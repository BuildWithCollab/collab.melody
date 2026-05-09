// Round-trip every voice kind through JSON. Verifies the `kind`
// discriminator survives serialization and all per-kind fields
// round-trip cleanly via def_type.

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <def_type.hpp>

#include <stdexcept>
#include <string>

import collab.melody;

using namespace collab::melody;

TEST_CASE("ToneVoice round-trips through JSON", "[serialization][tone]") {
    ToneVoice original{
        .start_ms    = 100,
        .freq_hz     = 880.0,
        .duration_ms = 250,
        .wave        = Wave::triangle,
        .fade_ms     = 12,
        .volume      = 0.42
    };

    const auto j = def_type::to_json(original);
    REQUIRE(j["kind"] == "tone");
    REQUIRE(j["start_ms"] == 100);
    REQUIRE(j["freq_hz"] == 880.0);
    REQUIRE(j["wave"] == "triangle");

    const auto restored = def_type::from_json<ToneVoice>(j);
    CHECK(restored.kind        == "tone");
    CHECK(restored.start_ms    == original.start_ms);
    CHECK(restored.freq_hz     == original.freq_hz);
    CHECK(restored.duration_ms == original.duration_ms);
    CHECK(restored.wave        == original.wave);
    CHECK(restored.fade_ms     == original.fade_ms);
    CHECK(restored.volume      == original.volume);
}

TEST_CASE("GlideVoice round-trips with from/to frequencies", "[serialization][glide]") {
    GlideVoice original{
        .start_ms    = 0,
        .from_hz     = 220.0,
        .to_hz       = 1760.0,
        .duration_ms = 1200,
        .wave        = Wave::square,
    };

    const auto j        = def_type::to_json(original);
    const auto restored = def_type::from_json<GlideVoice>(j);

    CHECK(restored.kind    == "glide");
    CHECK(restored.from_hz == 220.0);
    CHECK(restored.to_hz   == 1760.0);
    CHECK(restored.wave    == Wave::square);
}

TEST_CASE("PianoVoice round-trips harmonics array", "[serialization][piano]") {
    PianoVoice original{
        .start_ms  = 50,
        .freq_hz   = 523.25,
        .attack_ms = 8,
        .tau_ms    = 750,
        .volume    = 0.35,
        .harmonics = {1.0, 0.5, 0.3, 0.15}
    };

    const auto j = def_type::to_json(original);
    REQUIRE(j["harmonics"].is_array());
    REQUIRE(j["harmonics"].size() == 4);

    const auto restored = def_type::from_json<PianoVoice>(j);
    CHECK(restored.kind      == "piano");
    CHECK(restored.harmonics == original.harmonics);
    CHECK(restored.attack_ms == 8);
    CHECK(restored.tau_ms    == 750);
}

TEST_CASE("TremoloVoice round-trips LFO knobs", "[serialization][tremolo]") {
    TremoloVoice original{
        .freq_hz     = 660.0,
        .duration_ms = 800,
        .lfo_hz      = 4.5,
        .depth       = 0.6,
    };
    const auto restored = def_type::from_json<TremoloVoice>(def_type::to_json(original));
    CHECK(restored.kind   == "tremolo");
    CHECK(restored.lfo_hz == 4.5);
    CHECK(restored.depth  == 0.6);
}

TEST_CASE("VibratoVoice round-trips depth in semitones", "[serialization][vibrato]") {
    VibratoVoice original{
        .freq_hz         = 440.0,
        .duration_ms     = 1000,
        .depth_semitones = 1.5,
    };
    const auto restored = def_type::from_json<VibratoVoice>(def_type::to_json(original));
    CHECK(restored.kind            == "vibrato");
    CHECK(restored.depth_semitones == 1.5);
}

TEST_CASE("DecayVoice round-trips bell envelope", "[serialization][decay]") {
    DecayVoice original{
        .freq_hz     = 1000.0,
        .duration_ms = 600,
        .tau_ms      = 150,
    };
    const auto restored = def_type::from_json<DecayVoice>(def_type::to_json(original));
    CHECK(restored.kind        == "decay");
    CHECK(restored.tau_ms      == 150);
    CHECK(restored.duration_ms == 600);
}

TEST_CASE("SilenceVoice round-trips", "[serialization][silence]") {
    SilenceVoice original{ .start_ms = 200, .duration_ms = 500 };
    const auto restored = def_type::from_json<SilenceVoice>(def_type::to_json(original));
    CHECK(restored.kind        == "silence");
    CHECK(restored.start_ms    == 200);
    CHECK(restored.duration_ms == 500);
}

TEST_CASE("Wave enum serializes by name", "[serialization][enum]") {
    ToneVoice t{ .wave = Wave::sawtooth };
    const auto j = def_type::to_json(t);
    REQUIRE(j["wave"] == "sawtooth");

    const auto back = def_type::from_json<ToneVoice>(j);
    CHECK(back.wave == Wave::sawtooth);
}

TEST_CASE("Defaults preserved when JSON omits fields", "[serialization][defaults]") {
    const auto t = def_type::from_json<ToneVoice>(std::string(R"({"kind": "tone"})"));
    CHECK(t.freq_hz     == 440.0);    // default
    CHECK(t.duration_ms == 500);       // default
    CHECK(t.wave        == Wave::sine);// default
}
