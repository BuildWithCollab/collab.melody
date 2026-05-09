// File IO for melodies. Pure interface — implementations live in
// collab.melody.io.cpp so def_type's heavy template instantiations
// don't end up in this module's BMI.

module;

#include <filesystem>
#include <functional>
#include <map>
#include <string>
#include <string_view>

export module collab.melody.io;

import std;
import collab.melody.melody;

export namespace collab::melody {

[[nodiscard]] Melody load_from_json_file(const std::filesystem::path& path);
[[nodiscard]] Melody load_from_json_string(std::string_view json);

[[nodiscard]] std::string to_json_string(const Melody& melody, int indent = 2);
void                      save_to_json_file(const Melody& melody, const std::filesystem::path& path);

// Load every *.json file in `dir` (non-recursive), keyed by file stem.
// Failures per file are reported via error_callback; other files still load.
[[nodiscard]] std::map<std::string, Melody> load_all_from_directory(
    const std::filesystem::path& dir,
    std::function<void(const std::filesystem::path&, std::string_view error)> error_callback = {}
);

}  // namespace collab::melody
