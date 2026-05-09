// File IO — write a melody to disk, read it back, verify identity.
// Uses a tmpdir so we don't litter the working directory.

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <filesystem>
#include <fstream>
#include <random>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

import collab.melody;

using namespace collab::melody;

namespace {

std::filesystem::path make_tmpdir() {
    auto dir = std::filesystem::temp_directory_path() /
               ("collab.melody.test." + std::to_string(std::random_device{}()));
    std::filesystem::create_directories(dir);
    return dir;
}

}  // namespace

TEST_CASE("save_to_json_file then load_from_json_file round-trips", "[io]") {
    const auto tmp  = make_tmpdir();
    const auto path = tmp / "test.json";

    Melody original{
        .name = "io-test",
        .voices = {
            DecayVoice{ .freq_hz = 880.0, .duration_ms = 500, .tau_ms = 200 },
            ToneVoice { .start_ms = 200, .freq_hz = 440.0, .duration_ms = 200 },
        }
    };

    save_to_json_file(original, path);
    REQUIRE(std::filesystem::exists(path));

    const auto loaded = load_from_json_file(path);
    CHECK(loaded.name == original.name);
    REQUIRE(loaded.voices.size() == original.voices.size());

    std::filesystem::remove_all(tmp);
}

TEST_CASE("load_from_json_file throws for missing file", "[io]") {
    REQUIRE_THROWS_WITH(
        load_from_json_file("does_not_exist.json"),
        Catch::Matchers::ContainsSubstring("does_not_exist")
    );
}

TEST_CASE("load_from_json_string throws for malformed JSON", "[io]") {
    REQUIRE_THROWS_AS(
        load_from_json_string("{ this is not valid json"),
        std::runtime_error
    );
}

TEST_CASE("load_from_json_string parses minimal melody", "[io]") {
    const std::string json = R"({"name": "tiny", "voices": []})";
    const auto m = load_from_json_string(json);
    CHECK(m.name == "tiny");
    CHECK(m.voices.empty());
}

TEST_CASE("to_json_string is pretty-printed by default", "[io]") {
    Melody m{ .name = "p" };
    const auto s = to_json_string(m);
    CHECK(s.find('\n') != std::string::npos);
    CHECK(s.find("\"name\"") != std::string::npos);
}

TEST_CASE("to_json_string with indent=-1 is compact", "[io]") {
    Melody m{ .name = "c" };
    const auto s = to_json_string(m, -1);
    CHECK(s.find('\n') == std::string::npos);
}

TEST_CASE("load_all_from_directory loads every .json file", "[io]") {
    const auto tmp = make_tmpdir();

    save_to_json_file(Melody{ .name = "alpha" }, tmp / "alpha.json");
    save_to_json_file(Melody{ .name = "beta"  }, tmp / "beta.json");
    save_to_json_file(Melody{ .name = "gamma" }, tmp / "gamma.json");

    // Drop a non-JSON file to verify it's ignored.
    std::ofstream(tmp / "not_a_melody.txt") << "ignore me";

    const auto melodies = load_all_from_directory(tmp);
    CHECK(melodies.size() == 3);
    CHECK(melodies.contains("alpha"));
    CHECK(melodies.contains("beta"));
    CHECK(melodies.contains("gamma"));

    std::filesystem::remove_all(tmp);
}

TEST_CASE("load_all_from_directory reports parse errors via callback", "[io]") {
    const auto tmp = make_tmpdir();

    save_to_json_file(Melody{ .name = "ok" }, tmp / "ok.json");
    std::ofstream(tmp / "broken.json") << "not json at all";

    int errors_seen = 0;
    std::filesystem::path bad_path;
    const auto melodies = load_all_from_directory(tmp,
        [&](const std::filesystem::path& p, std::string_view) {
            ++errors_seen;
            bad_path = p;
        });

    CHECK(errors_seen == 1);
    CHECK(bad_path.filename() == "broken.json");
    CHECK(melodies.size() == 1);
    CHECK(melodies.contains("ok"));

    std::filesystem::remove_all(tmp);
}

TEST_CASE("load_all_from_directory on missing dir returns empty", "[io]") {
    const auto melodies = load_all_from_directory("really_does_not_exist_xyz");
    CHECK(melodies.empty());
}
