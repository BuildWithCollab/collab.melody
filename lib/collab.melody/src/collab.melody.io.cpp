// Implementation of collab.melody.io.
//
// def_type's templates are instantiated here (in the module impl
// unit) rather than in the .cppm so the heavy template machinery
// stays out of the BMI — keeps GCC's module serializer happy.

module;

#include <def_type.hpp>

#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

module collab.melody.io;

import collab.melody.melody;

namespace collab::melody {

Melody load_from_json_string(std::string_view json) {
    return def_type::from_json<Melody>(std::string(json));
}

Melody load_from_json_file(const std::filesystem::path& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("collab.melody: cannot open file: " + path.string());
    }
    std::stringstream buf;
    buf << in.rdbuf();
    return load_from_json_string(buf.str());
}

std::string to_json_string(const Melody& melody, int indent) {
    return def_type::to_json_string(melody, indent);
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

std::map<std::string, Melody> load_all_from_directory(
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
