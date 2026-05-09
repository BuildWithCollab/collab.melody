// Round-trip a Melody (which contains std::vector<Voice>) through JSON.
// Verifies the full pipeline: voice ADL hooks fire correctly when the
// vector is walked by def_type, and the per-voice kind dispatch lands
// on the right alternatives during from_json.

#include <catch2/catch_test_macros.hpp>

#include <def_type.hpp>
#include <nlohmann/json.hpp>

#include <variant>

import collab.melody;

using namespace collab::melody;

TEST_CASE("Empty Melody round-trips", "[melody][serialization]") {
    Melody m{ .name = "empty" };
    const auto j        = def_type::to_json(m);
    const auto restored = def_type::from_json<Melody>(j);
    CHECK(restored.name == "empty");
    CHECK(restored.voices.empty());
}

TEST_CASE("Multi-voice Melody round-trips with mixed kinds", "[melody][serialization]") {
    Melody original{
        .name = "doorbell",
        .voices = {
            DecayVoice{ .start_ms =   0, .freq_hz = 660.0, .duration_ms = 600, .tau_ms = 250 },
            DecayVoice{ .start_ms = 200, .freq_hz = 520.0, .duration_ms = 800, .tau_ms = 350 },
            ToneVoice { .start_ms = 800, .freq_hz = 440.0, .duration_ms = 100 },
        }
    };

    const auto j = def_type::to_json(original);
    REQUIRE(j["voices"].is_array());
    REQUIRE(j["voices"].size() == 3);
    CHECK(j["voices"][0]["kind"] == "decay");
    CHECK(j["voices"][1]["kind"] == "decay");
    CHECK(j["voices"][2]["kind"] == "tone");

    const auto restored = def_type::from_json<Melody>(j);
    REQUIRE(restored.name == "doorbell");
    REQUIRE(restored.voices.size() == 3);

    REQUIRE(std::holds_alternative<DecayVoice>(restored.voices[0]));
    CHECK(std::get<DecayVoice>(restored.voices[0]).freq_hz == 660.0);

    REQUIRE(std::holds_alternative<DecayVoice>(restored.voices[1]));
    CHECK(std::get<DecayVoice>(restored.voices[1]).start_ms == 200);

    REQUIRE(std::holds_alternative<ToneVoice>(restored.voices[2]));
    CHECK(std::get<ToneVoice>(restored.voices[2]).freq_hz == 440.0);
}

TEST_CASE("Melody round-trips through string", "[melody][serialization]") {
    const std::string json_text = R"({
        "name": "chord",
        "duration_ms": 0,
        "voices": [
            { "kind": "piano", "start_ms":   0, "freq_hz": 523.25, "attack_ms":  3, "tau_ms": 600 },
            { "kind": "piano", "start_ms": 180, "freq_hz": 659.25, "attack_ms":  6, "tau_ms": 600 },
            { "kind": "piano", "start_ms": 360, "freq_hz": 783.99, "attack_ms": 12, "tau_ms": 600 }
        ]
    })";

    const auto m = def_type::from_json<Melody>(nlohmann::json::parse(json_text));
    REQUIRE(m.name == "chord");
    REQUIRE(m.voices.size() == 3);
    for (const auto& v : m.voices) {
        REQUIRE(std::holds_alternative<PianoVoice>(v));
    }
    CHECK(std::get<PianoVoice>(m.voices[2]).attack_ms == 12);
}

TEST_CASE("Melody preserves all voice kinds in one melody", "[melody][serialization]") {
    Melody original{
        .name = "everything",
        .voices = {
            ToneVoice{},
            GlideVoice{},
            PianoVoice{},
            TremoloVoice{},
            VibratoVoice{},
            DecayVoice{},
            SilenceVoice{},
        }
    };

    const auto restored = def_type::from_json<Melody>(def_type::to_json(original));
    REQUIRE(restored.voices.size() == 7);
    CHECK(std::holds_alternative<ToneVoice>   (restored.voices[0]));
    CHECK(std::holds_alternative<GlideVoice>  (restored.voices[1]));
    CHECK(std::holds_alternative<PianoVoice>  (restored.voices[2]));
    CHECK(std::holds_alternative<TremoloVoice>(restored.voices[3]));
    CHECK(std::holds_alternative<VibratoVoice>(restored.voices[4]));
    CHECK(std::holds_alternative<DecayVoice>  (restored.voices[5]));
    CHECK(std::holds_alternative<SilenceVoice>(restored.voices[6]));
}
