// Render a `Melody` to mono 16-bit signed PCM at 44.1 kHz.
//
// All voices share one timeline: each voice's `start_ms` places it on
// the timeline, and voices overlap freely. Mixing accumulates into an
// int32 buffer and clips down to int16 once at the end, so summing
// many voices doesn't smear per-voice clipping into the result.
//
// This is the interface only — implementations live in
// collab.melody.render.cpp so the heavy synthesis math and the
// def_type template machinery don't end up in this BMI.

module;

#include <cstdint>
#include <vector>

export module collab.melody.render;

import std;
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
