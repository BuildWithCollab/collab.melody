// Load melodies from / save melodies to JSON.
//
// The actual struct ↔ JSON marshalling is handled by def_type's
// reflection — these helpers just wrap file IO around it and surface
// useful errors for the common "user typo'd a JSON file" case.

module;

#include <def_type.hpp>
#include <nlohmann/json.hpp>

export module collab.melody.io;

import std;
import collab.melody.melody;

export namespace collab::melody {

// Load a single melody from a JSON file. Throws std::runtime_error
// (with a descriptive message including the path) if the file can't
// be opened, isn't valid JSON, or doesn't match the Melody schema.
[[nodiscard]] Melody load_from_json_file(const std::filesystem::path& path);

// Parse a melody from an already-loaded JSON string. Same error
// behavior as load_from_json_file.
[[nodiscard]] Melody load_from_json_string(std::string_view json);

// Serialize a melody to a JSON string. `indent = -1` is compact;
// `indent >= 0` is pretty-printed with that many spaces.
[[nodiscard]] std::string to_json_string(const Melody& melody, int indent = 2);

// Serialize a melody to a JSON file (pretty-printed).
void save_to_json_file(const Melody& melody, const std::filesystem::path& path);

// Load every *.json file in `dir` (non-recursive), keyed by file
// stem (e.g. "doorbell.json" → key "doorbell"). Files that fail to
// parse are surfaced via the error_callback (which can be empty if
// you want failures silently skipped).
[[nodiscard]] std::map<std::string, Melody> load_all_from_directory(
    const std::filesystem::path& dir,
    std::function<void(const std::filesystem::path&, std::string_view error)> error_callback = {}
);

}  // namespace collab::melody

// ─── Implementation ────────────────────────────────────────────────

namespace collab::melody {

[[nodiscard]] Melody load_from_json_string(std::string_view json) {
    try {
        auto parsed = nlohmann::json::parse(json);
        return def_type::from_json<Melody>(parsed);
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("collab.melody: JSON parse failed: ") + e.what());
    }
}

[[nodiscard]] Melody load_from_json_file(const std::filesystem::path& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("collab.melody: cannot open file: " + path.string());
    }
    std::stringstream buf;
    buf << in.rdbuf();
    try {
        return load_from_json_string(buf.str());
    } catch (const std::exception& e) {
        // Re-wrap with the path for a more useful error.
        throw std::runtime_error(
            std::string("collab.melody: failed to load ") + path.string() + ": " + e.what());
    }
}

[[nodiscard]] std::string to_json_string(const Melody& melody, int indent) {
    auto j = def_type::to_json(melody);
    return j.dump(indent);
}

void save_to_json_file(const Melody& melody, const std::filesystem::path& path) {
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path());
    }
    std::ofstream out(path);
    if (!out) {
        throw std::runtime_error("collab.melody: cannot write to file: " + path.string());
    }
    out << to_json_string(melody, 2);
}

[[nodiscard]] std::map<std::string, Melody> load_all_from_directory(
    const std::filesystem::path& dir,
    std::function<void(const std::filesystem::path&, std::string_view)> error_callback)
{
    std::map<std::string, Melody> out;
    if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir)) {
        return out;
    }

    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        const auto& path = entry.path();
        if (path.extension() != ".json") continue;

        try {
            auto m = load_from_json_file(path);
            out.emplace(path.stem().string(), std::move(m));
        } catch (const std::exception& e) {
            if (error_callback) error_callback(path, e.what());
        }
    }
    return out;
}

}  // namespace collab::melody
