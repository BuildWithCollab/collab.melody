// 🎉 melody_demo — load every melody in a folder and play them in order.
//
//   xmake run example.melody_demo                 # plays everything in melodies/
//   xmake run example.melody_demo --dir my-sounds # different folder
//   xmake run example.melody_demo --filter chord  # only matching names
//   xmake run example.melody_demo --list          # print without playing

#include <CLI/CLI.hpp>

#include <algorithm>
#include <chrono>
#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>

import collab.melody;

int main(int argc, char** argv) {
    CLI::App app{"🎉 Tour every melody in a folder"};

    std::string dir         = "melodies";
    std::string filter      = "";
    bool        list_only   = false;
    int         pause_ms    = 350;

    app.add_option("--dir",     dir,     "Folder containing .json melodies")->capture_default_str();
    app.add_option("--filter",  filter,  "Only play melodies whose name contains this substring");
    app.add_flag(  "--list",    list_only, "Print the melodies without playing them");
    app.add_option("--pause",   pause_ms,  "Milliseconds of silence between melodies")
        ->capture_default_str();

    CLI11_PARSE(app, argc, argv);

    auto melodies = collab::melody::load_all_from_directory(dir,
        [](const std::filesystem::path& p, std::string_view err) {
            std::cerr << "⚠️  failed to load " << p << ": " << err << '\n';
        });

    if (melodies.empty()) {
        std::cerr << "💀 no melodies found in " << dir << '\n';
        return 1;
    }

    std::cout << "🎼 loaded " << melodies.size() << " melodie"
              << (melodies.size() == 1 ? "" : "s") << " from " << dir << "\n\n";

    collab::melody::Player player;

    for (const auto& [name, melody] : melodies) {
        if (!filter.empty() && name.find(filter) == std::string::npos) {
            continue;
        }

        std::cout << "▶️  " << name;
        if (!melody.name.empty() && melody.name != name) {
            std::cout << "  (\"" << melody.name << "\")";
        }
        std::cout << "  — " << melody.voices.size() << " voice"
                  << (melody.voices.size() == 1 ? "" : "s") << '\n';

        if (list_only) continue;

        if (!player.play(melody)) {
            std::cerr << "💀 audio device unavailable\n";
            return 2;
        }
        player.wait();
        std::this_thread::sleep_for(std::chrono::milliseconds(pause_ms));
    }
    return 0;
}
