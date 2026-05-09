// Round-trip every voice kind through JSON. Each voice's `kind` field
// is the discriminator used by Voice's variant from_json; verify the
// kind survives serialization, all per-kind fields round-trip, and
// from_json rejects malformed input.

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <def_type.hpp>
#include <nlohmann/json.hpp>

#include <stdexcept>
#include <string>
#include <variant>

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

TEST_CASE("Voice variant dispatches via kind on from_json", "[serialization][variant]") {
    SECTION("kind=tone resolves to ToneVoice") {
        nlohmann::json j = {{"kind", "tone"}, {"freq_hz", 660.0}, {"duration_ms", 200}};
        Voice v;
        from_json(j, v);
        REQUIRE(std::holds_alternative<ToneVoice>(v));
        CHECK(std::get<ToneVoice>(v).freq_hz == 660.0);
    }
    SECTION("kind=piano resolves to PianoVoice") {
        nlohmann::json j = {{"kind", "piano"}, {"freq_hz", 523.25}, {"tau_ms", 800}};
        Voice v;
        from_json(j, v);
        REQUIRE(std::holds_alternative<PianoVoice>(v));
        CHECK(std::get<PianoVoice>(v).tau_ms == 800);
    }
    SECTION("kind=glide resolves to GlideVoice") {
        nlohmann::json j = {{"kind", "glide"}, {"from_hz", 100.0}, {"to_hz", 200.0}};
        Voice v;
        from_json(j, v);
        REQUIRE(std::holds_alternative<GlideVoice>(v));
    }
    SECTION("missing kind throws") {
        nlohmann::json j = {{"freq_hz", 660.0}};
        Voice v;
        REQUIRE_THROWS_AS(from_json(j, v), std::logic_error);
    }
    SECTION("unknown kind throws") {
        nlohmann::json j = {{"kind", "bogus"}, {"freq_hz", 660.0}};
        Voice v;
        REQUIRE_THROWS_WITH(
            from_json(j, v),
            Catch::Matchers::ContainsSubstring("bogus")
        );
    }
    SECTION("non-object voice throws") {
        nlohmann::json j = "not an object";
        Voice v;
        REQUIRE_THROWS_AS(from_json(j, v), std::logic_error);
    }
}

TEST_CASE("Voice variant round-trips via to_json/from_json", "[serialization][variant]") {
    Voice original = PianoVoice{
        .start_ms  = 100,
        .freq_hz   = 659.25,
        .attack_ms = 6,
        .tau_ms    = 700,
    };

    nlohmann::json j;
    to_json(j, original);
    REQUIRE(j["kind"] == "piano");

    Voice restored;
    from_json(j, restored);
    REQUIRE(std::holds_alternative<PianoVoice>(restored));
    CHECK(std::get<PianoVoice>(restored).freq_hz == 659.25);
    CHECK(std::get<PianoVoice>(restored).tau_ms  == 700);
}

TEST_CASE("Wave enum serializes by name", "[serialization][enum]") {
    ToneVoice t{ .wave = Wave::sawtooth };
    const auto j = def_type::to_json(t);
    REQUIRE(j["wave"] == "sawtooth");

    const auto back = def_type::from_json<ToneVoice>(j);
    CHECK(back.wave == Wave::sawtooth);
}

TEST_CASE("Defaults preserved when JSON omits fields", "[serialization][defaults]") {
    nlohmann::json minimal = {{"kind", "tone"}};
    const auto t = def_type::from_json<ToneVoice>(minimal);
    CHECK(t.freq_hz     == 440.0);    // default
    CHECK(t.duration_ms == 500);       // default
    CHECK(t.wave        == Wave::sine);// default
}
