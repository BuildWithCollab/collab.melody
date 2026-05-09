// Render a `Melody` to mono 16-bit signed PCM at 44.1 kHz.
//
// All voices share one timeline: each voice's `start_ms` places it on
// the timeline, and voices overlap freely. Mixing accumulates into an
// int32 buffer and clips down to int16 once at the end, so summing
// many voices doesn't smear per-voice clipping into the result.

module;

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <vector>

export module collab.melody.render;

import collab.melody.voices;
import collab.melody.melody;

export namespace collab::melody {

constexpr int kSampleRate = 44100;

struct RenderedAudio {
    std::vector<std::int16_t> samples;
    int                       sample_rate = kSampleRate;
};

// Returns the audible length of `voice` in milliseconds. For voices
// with a fixed duration_ms field, that's the answer. PianoVoice has
// auto-derive logic when duration_ms == 0.
[[nodiscard]] int voice_duration_ms(const Voice& voice);

// Total duration the buffer needs to be: max(start_ms + duration_ms)
// over all voices, or melody.duration_ms if non-zero.
[[nodiscard]] int derive_duration_ms(const Melody& melody);

[[nodiscard]] RenderedAudio render(const Melody& melody);

}  // namespace collab::melody

// ─── Implementation ────────────────────────────────────────────────

namespace collab::melody {

namespace {

constexpr double kPi = std::numbers::pi_v<double>;

inline int ms_to_samples(int ms) { return (kSampleRate * ms) / 1000; }

inline std::int16_t to_pcm_clipped(double s) {
    return static_cast<std::int16_t>(std::clamp(s, -1.0, 1.0) * 32767.0);
}

inline double oscillator(Wave w, double phase) {
    switch (w) {
        case Wave::sine:     return std::sin(2.0 * kPi * phase);
        case Wave::square:   return phase < 0.5 ? 1.0 : -1.0;
        case Wave::triangle: return phase < 0.5 ? (4.0 * phase - 1.0)
                                                : (3.0 - 4.0 * phase);
        case Wave::sawtooth: return 2.0 * phase - 1.0;
    }
    return 0.0;
}

// 0..1 multiplier providing linear fade-in for the first `fade_samples`
// and fade-out for the last `fade_samples`. Eliminates edge clicks.
inline double edge_envelope(int sample_index, int total_samples, int fade_samples) {
    if (fade_samples <= 0) return 1.0;
    if (sample_index < fade_samples)
        return double(sample_index) / fade_samples;
    if (sample_index >= total_samples - fade_samples)
        return double(total_samples - 1 - sample_index) / fade_samples;
    return 1.0;
}

// All mix_* helpers add into `buf` at `start_sample`, clamping each
// per-sample contribution before adding so int32 overflow is impossible
// even with extreme polyphony.
inline void mix_add(std::vector<std::int32_t>& buf, int idx, double s) {
    if (idx >= 0 && idx < static_cast<int>(buf.size())) {
        buf[idx] += static_cast<std::int32_t>(std::clamp(s, -1.0, 1.0) * 32767.0);
    }
}

void mix_tone(std::vector<std::int32_t>& buf, const ToneVoice& v) {
    const int start = ms_to_samples(v.start_ms);
    const int n     = ms_to_samples(v.duration_ms);
    const int fade  = ms_to_samples(v.fade_ms);
    if (n <= 0) return;

    double phase = 0.0;
    const double inc = v.freq_hz / kSampleRate;

    for (int i = 0; i < n; ++i) {
        const double env = edge_envelope(i, n, fade);
        mix_add(buf, start + i, oscillator(v.wave, phase) * v.volume * env);
        phase += inc;
        if (phase >= 1.0) phase -= 1.0;
    }
}

void mix_glide(std::vector<std::int32_t>& buf, const GlideVoice& v) {
    const int start = ms_to_samples(v.start_ms);
    const int n     = ms_to_samples(v.duration_ms);
    const int fade  = ms_to_samples(v.fade_ms);
    if (n <= 0) return;

    double phase = 0.0;
    for (int i = 0; i < n; ++i) {
        const double t    = double(i) / n;
        const double freq = v.from_hz + (v.to_hz - v.from_hz) * t;
        const double env  = edge_envelope(i, n, fade);
        mix_add(buf, start + i, oscillator(v.wave, phase) * v.volume * env);
        phase += freq / kSampleRate;
        if (phase >= 1.0) phase -= 1.0;
    }
}

void mix_piano(std::vector<std::int32_t>& buf, const PianoVoice& v) {
    const int start          = ms_to_samples(v.start_ms);
    const int attack_samples = std::max(1, ms_to_samples(v.attack_ms));
    const double tau_samples = ms_to_samples(v.tau_ms);

    int duration_ms = v.duration_ms;
    if (duration_ms <= 0) {
        duration_ms = std::max(50, v.attack_ms + v.tau_ms * 5);
    }
    const int n = ms_to_samples(duration_ms);
    if (n <= 0) return;

    // Normalize harmonics so summed peak stays around 1.0.
    double weight_sum = 0.0;
    for (double h : v.harmonics) weight_sum += std::abs(h);
    if (weight_sum <= 0.0) return;
    const double norm = 1.0 / weight_sum;

    std::vector<double> phase(v.harmonics.size(), 0.0);
    std::vector<double> incs(v.harmonics.size());
    for (std::size_t k = 0; k < v.harmonics.size(); ++k) {
        incs[k] = (v.freq_hz * static_cast<double>(k + 1)) / kSampleRate;
    }

    for (int i = 0; i < n; ++i) {
        double amp;
        if (i < attack_samples) {
            amp = double(i) / attack_samples;
        } else {
            amp = std::exp(-double(i - attack_samples) / tau_samples);
        }

        double partials = 0.0;
        for (std::size_t k = 0; k < v.harmonics.size(); ++k) {
            partials += v.harmonics[k] * std::sin(2.0 * kPi * phase[k]);
            phase[k] += incs[k];
            if (phase[k] >= 1.0) phase[k] -= 1.0;
        }

        mix_add(buf, start + i, partials * norm * v.volume * amp);
    }
}

void mix_tremolo(std::vector<std::int32_t>& buf, const TremoloVoice& v) {
    const int start = ms_to_samples(v.start_ms);
    const int n     = ms_to_samples(v.duration_ms);
    const int fade  = ms_to_samples(v.fade_ms);
    if (n <= 0) return;

    double phase = 0.0;
    const double inc = v.freq_hz / kSampleRate;

    for (int i = 0; i < n; ++i) {
        const double t   = double(i) / kSampleRate;
        const double lfo = 1.0 - v.depth * 0.5
                         * (1.0 - std::cos(2.0 * kPi * v.lfo_hz * t));
        const double env = edge_envelope(i, n, fade);
        mix_add(buf, start + i, std::sin(2.0 * kPi * phase) * v.volume * lfo * env);
        phase += inc;
        if (phase >= 1.0) phase -= 1.0;
    }
}

void mix_vibrato(std::vector<std::int32_t>& buf, const VibratoVoice& v) {
    const int start = ms_to_samples(v.start_ms);
    const int n     = ms_to_samples(v.duration_ms);
    const int fade  = ms_to_samples(v.fade_ms);
    if (n <= 0) return;

    double phase = 0.0;
    for (int i = 0; i < n; ++i) {
        const double t        = double(i) / kSampleRate;
        const double mult     = std::pow(2.0,
                                  (v.depth_semitones * std::sin(2.0 * kPi * v.lfo_hz * t)) / 12.0);
        const double env      = edge_envelope(i, n, fade);
        mix_add(buf, start + i, std::sin(2.0 * kPi * phase) * v.volume * env);
        phase += (v.freq_hz * mult) / kSampleRate;
        if (phase >= 1.0) phase -= 1.0;
    }
}

void mix_decay(std::vector<std::int32_t>& buf, const DecayVoice& v) {
    const int    start           = ms_to_samples(v.start_ms);
    const int    n               = ms_to_samples(v.duration_ms);
    const int    fade_in_samples = ms_to_samples(v.fade_in_ms);
    const double tau_samples     = ms_to_samples(v.tau_ms);
    if (n <= 0) return;

    double phase = 0.0;
    const double inc = v.freq_hz / kSampleRate;

    for (int i = 0; i < n; ++i) {
        const double attack = (i < fade_in_samples) ? double(i) / std::max(1, fade_in_samples) : 1.0;
        const double decay  = std::exp(-double(i) / tau_samples);
        mix_add(buf, start + i, std::sin(2.0 * kPi * phase) * v.volume * attack * decay);
        phase += inc;
        if (phase >= 1.0) phase -= 1.0;
    }
}

[[nodiscard]] std::vector<std::int16_t> finalize_mix(const std::vector<std::int32_t>& mix) {
    std::vector<std::int16_t> out;
    out.reserve(mix.size());
    for (std::int32_t s : mix) {
        out.push_back(static_cast<std::int16_t>(std::clamp<std::int32_t>(s, -32767, 32767)));
    }
    return out;
}

}  // anonymous namespace

[[nodiscard]] int voice_duration_ms(const Voice& voice) {
    return voice.match(
        [](const ToneVoice&    v) { return v.duration_ms; },
        [](const GlideVoice&   v) { return v.duration_ms; },
        [](const PianoVoice&   v) {
            if (v.duration_ms > 0) return v.duration_ms;
            return std::max(50, v.attack_ms + v.tau_ms * 5);
        },
        [](const TremoloVoice& v) { return v.duration_ms; },
        [](const VibratoVoice& v) { return v.duration_ms; },
        [](const DecayVoice&   v) { return v.duration_ms; },
        [](const SilenceVoice& v) { return v.duration_ms; }
    );
}

namespace {
[[nodiscard]] int voice_start_ms(const Voice& voice) {
    return voice.match([](const auto& v) { return v.start_ms; });
}
}  // anonymous namespace

[[nodiscard]] int derive_duration_ms(const Melody& melody) {
    if (melody.duration_ms > 0) return melody.duration_ms;
    int max_end = 0;
    for (const auto& v : melody.voices) {
        const int end = voice_start_ms(v) + voice_duration_ms(v);
        if (end > max_end) max_end = end;
    }
    return max_end;
}

[[nodiscard]] RenderedAudio render(const Melody& melody) {
    const int total_ms = derive_duration_ms(melody);
    if (total_ms <= 0) {
        return RenderedAudio{ {}, kSampleRate };
    }

    std::vector<std::int32_t> mix(ms_to_samples(total_ms), 0);

    for (const auto& voice : melody.voices) {
        voice.match(
            [&](const ToneVoice&    v) { mix_tone(mix,    v); },
            [&](const GlideVoice&   v) { mix_glide(mix,   v); },
            [&](const PianoVoice&   v) { mix_piano(mix,   v); },
            [&](const TremoloVoice& v) { mix_tremolo(mix, v); },
            [&](const VibratoVoice& v) { mix_vibrato(mix, v); },
            [&](const DecayVoice&   v) { mix_decay(mix,   v); },
            [](const SilenceVoice&) { /* gap on the timeline */ }
        );
    }

    return RenderedAudio{ finalize_mix(mix), kSampleRate };
}

}  // namespace collab::melody
