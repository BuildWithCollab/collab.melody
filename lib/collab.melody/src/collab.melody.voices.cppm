// Wave shape, voice kinds, and the `Voice` variant used by `Melody`.
//
// Each voice is a plain C++ struct with sensible defaults — designated
// initializers in C++, plain JSON for tweakers. Each struct's `kind`
// member defaults to its label and is included in the serialized JSON.
//
// Voice is a std::variant; we provide nlohmann ADL hooks (to_json /
// from_json) that switch on the `kind` field for polymorphic JSON.
// from_json on the variant reads `kind` and dispatches to the right
// alternative; to_json visits and delegates to def_type::to_json for
// the underlying struct (which serializes the `kind` field too).

module;

#include <def_type.hpp>
#include <nlohmann/json.hpp>

#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

export module collab.melody.voices;

import std;

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

// The variant. ToneVoice is first so default-construction picks a
// benign all-defaulted alternative.
using Voice = std::variant<
    ToneVoice,
    GlideVoice,
    PianoVoice,
    TremoloVoice,
    VibratoVoice,
    DecayVoice,
    SilenceVoice
>;

// Helper for the std::visit(overloaded{...}, variant) idiom.
template <class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };

// ─── nlohmann ADL hooks for the Voice variant ──────────────────────
//
// def_type's value_to_json/value_from_json fall through to nlohmann's
// ADL hooks for non-reflected types, so these get picked up when a
// Melody (which contains std::vector<Voice>) round-trips. The `kind`
// field on each underlying struct is what makes from_json deterministic.

inline void to_json(nlohmann::json& j, const Voice& voice) {
    std::visit([&](const auto& v) { j = def_type::to_json(v); }, voice);
}

inline void from_json(const nlohmann::json& j, Voice& voice) {
    if (!j.is_object()) {
        throw std::logic_error("collab.melody: voice JSON must be an object");
    }
    if (!j.contains("kind") || !j["kind"].is_string()) {
        throw std::logic_error("collab.melody: voice JSON missing string `kind` field");
    }
    const auto kind = j["kind"].get<std::string>();

    if (kind == "tone")    { voice = def_type::from_json<ToneVoice>(j);    return; }
    if (kind == "glide")   { voice = def_type::from_json<GlideVoice>(j);   return; }
    if (kind == "piano")   { voice = def_type::from_json<PianoVoice>(j);   return; }
    if (kind == "tremolo") { voice = def_type::from_json<TremoloVoice>(j); return; }
    if (kind == "vibrato") { voice = def_type::from_json<VibratoVoice>(j); return; }
    if (kind == "decay")   { voice = def_type::from_json<DecayVoice>(j);   return; }
    if (kind == "silence") { voice = def_type::from_json<SilenceVoice>(j); return; }

    throw std::logic_error("collab.melody: unknown voice kind '" + kind + "'");
}

}  // namespace collab::melody
