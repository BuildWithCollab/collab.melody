// 🎵 play_melody — load a melody JSON file and play it.
//
//   xmake run example.play_melody melodies/doorbell.json
//   xmake run example.play_melody melodies/chord.json --validate-only

#include <CLI/CLI.hpp>

#include <chrono>
#include <exception>
#include <filesystem>
#include <iostream>
#include <string>

import collab.melody;

int main(int argc, char** argv) {
    CLI::App app{"🎵 Play a collab.melody JSON file through your speakers"};

    std::string path;
    bool        validate_only = false;
    bool        verbose       = false;

    app.add_option("path", path, "Path to a .json melody file")->required();
    app.add_flag("--validate-only", validate_only,
                 "Parse + render but do not play (handy for CI / sanity checks)");
    app.add_flag("--verbose", verbose, "Print melody info before playing");

    CLI11_PARSE(app, argc, argv);

    try {
        auto melody = collab::melody::load_from_json_file(path);

        if (verbose) {
            std::cout << "🎼 melody: " << melody.name
                      << "  (" << melody.voices.size() << " voice"
                      << (melody.voices.size() == 1 ? "" : "s") << ")\n";
        }

        const auto rendered = collab::melody::render(melody);
        if (verbose) {
            const double duration_s = double(rendered.samples.size()) / rendered.sample_rate;
            std::cout << "🔊 rendered " << rendered.samples.size() << " samples ("
                      << duration_s << " s)\n";
        }

        if (validate_only) {
            std::cout << "✅ valid: " << melody.name << '\n';
            return 0;
        }

        if (!collab::melody::play_blocking(rendered)) {
            std::cerr << "💀 audio device unavailable\n";
            return 2;
        }
    } catch (const std::exception& e) {
        std::cerr << "💀 " << e.what() << '\n';
        return 1;
    }
    return 0;
}
