// Round-trip a Melody (with mixed voice kinds) through JSON. Verifies
// def_type::oneof_by_field dispatches each voice to the right alt
// based on the `kind` discriminator, and that all per-voice fields
// survive the round-trip.

#include <catch2/catch_test_macros.hpp>

#include <def_type.hpp>

import collab.melody;

using namespace collab::melody;

TEST_CASE("Empty Melody round-trips", "[melody][serialization]") {
    Melody m{ .name = "empty" };
    const auto j        = def_type::to_json(m);
    const auto restored = def_type::from_json<Melody>(j);
    CHECK(restored.name == "empty");
    CHECK(restored.voices.empty());
}

TEST_CASE("Multi-voice Melody serializes kinds in order", "[melody][serialization]") {
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

    const auto restored = def_type::from_json<Melody>(def_type::to_json(original));
    REQUIRE(restored.voices.size() == 3);

    REQUIRE(restored.voices[0].is<DecayVoice>());
    CHECK(restored.voices[0].as<DecayVoice>()->freq_hz == 660.0);

    REQUIRE(restored.voices[1].is<DecayVoice>());
    CHECK(restored.voices[1].as<DecayVoice>()->start_ms == 200);

    REQUIRE(restored.voices[2].is<ToneVoice>());
    CHECK(restored.voices[2].as<ToneVoice>()->freq_hz == 440.0);
}

TEST_CASE("Melody round-trips through string", "[melody][serialization]") {
    const auto m = def_type::from_json<Melody>(std::string(R"({
        "name": "chord",
        "voices": [
            { "kind": "piano", "start_ms":   0, "freq_hz": 523.25, "attack_ms":  3, "tau_ms": 600 },
            { "kind": "piano", "start_ms": 180, "freq_hz": 659.25, "attack_ms":  6, "tau_ms": 600 },
            { "kind": "piano", "start_ms": 360, "freq_hz": 783.99, "attack_ms": 12, "tau_ms": 600 }
        ]
    })"));

    REQUIRE(m.name == "chord");
    REQUIRE(m.voices.size() == 3);
    for (const auto& v : m.voices) {
        REQUIRE(v.is<PianoVoice>());
    }
    CHECK(m.voices[2].as<PianoVoice>()->attack_ms == 12);
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
    CHECK(restored.voices[0].is<ToneVoice>());
    CHECK(restored.voices[1].is<GlideVoice>());
    CHECK(restored.voices[2].is<PianoVoice>());
    CHECK(restored.voices[3].is<TremoloVoice>());
    CHECK(restored.voices[4].is<VibratoVoice>());
    CHECK(restored.voices[5].is<DecayVoice>());
    CHECK(restored.voices[6].is<SilenceVoice>());
}
