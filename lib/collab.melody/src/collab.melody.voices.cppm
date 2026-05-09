// Wave shape and voice kinds used by `Melody`.

module;

#include <def_type.hpp>

#include <string>
#include <vector>

export module collab.melody.voices;

export namespace collab::melody {

enum class Wave {
    sine,
    square,
    triangle,
    sawtooth,
};

// Single-frequency tone with linear edge fades (anti-click).
struct ToneVoice {
    std::string kind        = "tone";
    int         start_ms    = 0;
    double      freq_hz     = 440.0;
    int         duration_ms = 500;
    Wave        wave        = Wave::sine;
    int         fade_ms     = 8;
    double      volume      = 0.6;
};

// Smooth linear pitch slide — boop, phew, sweep alarms.
struct GlideVoice {
    std::string kind        = "glide";
    int         start_ms    = 0;
    double      from_hz     = 220.0;
    double      to_hz       = 880.0;
    int         duration_ms = 800;
    Wave        wave        = Wave::sine;
    int         fade_ms     = 8;
    double      volume      = 0.6;
};

// Piano-ish voice — fast attack swell, exponential decay tail, light
// harmonic stack for body. The harmonics array is the per-partial
// amplitude relative to the fundamental: leave it as the default
// `[1.0, 0.4, 0.2]` for a balanced piano-ish sound, or supply
// `[1.0]` for a glassy bell, or `[1.0, 0.8, 0.6, 0.4]` for a
// brighter / brassier character.
struct PianoVoice {
    std::string         kind        = "piano";
    int                 start_ms    = 0;
    double              freq_hz     = 440.0;
    int                 attack_ms   = 5;
    int                 tau_ms      = 500;
    double              volume      = 0.4;
    int                 duration_ms = 0;   // 0 = derive from attack + ~5*tau
    std::vector<double> harmonics   = {1.0, 0.4, 0.2};
};

// Steady tone with amplitude LFO ("pulsing").
struct TremoloVoice {
    std::string kind        = "tremolo";
    int         start_ms    = 0;
    double      freq_hz     = 440.0;
    int         duration_ms = 1500;
    double      lfo_hz      = 6.0;
    double      depth       = 0.7;
    int         fade_ms     = 8;
    double      volume      = 0.6;
};

// Steady tone with frequency LFO ("wobble").
struct VibratoVoice {
    std::string kind            = "vibrato";
    int         start_ms        = 0;
    double      freq_hz         = 440.0;
    int         duration_ms     = 1500;
    double      lfo_hz          = 6.0;
    double      depth_semitones = 0.5;
    int         fade_ms         = 8;
    double      volume          = 0.6;
};

// Pure sine with exponential decay only (no harmonics, no swell) —
// the classic chime / bell shape.
struct DecayVoice {
    std::string kind        = "decay";
    int         start_ms    = 0;
    double      freq_hz     = 880.0;
    int         duration_ms = 1200;
    int         tau_ms      = 200;
    int         fade_in_ms  = 2;
    double      volume      = 0.7;
};

// Explicit silence — useful when authoring sequences in C++.
// Has no audible effect when rendered (timeline gaps already produce
// silence), but is preserved so the JSON can document intent.
struct SilenceVoice {
    std::string kind        = "silence";
    int         start_ms    = 0;
    int         duration_ms = 100;
};

// The variant. def_type::oneof_by_field uses each voice struct's
// `kind` field as the discriminator. JSON round-trip is handled by
// def_type — no custom hooks.
using Voice = def_type::oneof_by_field<"kind",
    def_type::oneof_type<ToneVoice,    "tone">,
    def_type::oneof_type<GlideVoice,   "glide">,
    def_type::oneof_type<PianoVoice,   "piano">,
    def_type::oneof_type<TremoloVoice, "tremolo">,
    def_type::oneof_type<VibratoVoice, "vibrato">,
    def_type::oneof_type<DecayVoice,   "decay">,
    def_type::oneof_type<SilenceVoice, "silence">
>;

}  // namespace collab::melody
