set_project("collab.melody")

add_rules("mode.release")
set_defaultmode("release")

set_languages("c++23")
set_warnings("all", "extra")
set_policy("build.c++.modules", true)

option("build_tests")
    set_default(true)
    set_showmenu(true)
    set_description("Build test targets")
option_end()

option("build_examples")
    set_default(true)
    set_showmenu(true)
    set_description("Build example binaries")
option_end()

includes("xmake/collab.lua")

-- def_type drives the JSON serialization of melody/voice structs.
-- miniaudio is only required by the player module. We keep it as a hard
-- dep because playback is the obvious use case for this library; if
-- offline-only consumers ever appear, the player can be split into its
-- own sibling library (collab.melody.player) to drop the dep.
add_collab_requires("def_type")
add_requires("miniaudio")

if get_config("build_examples") then
    add_requires("cli11")
end

if get_config("build_tests") then
    add_requires("catch2")
end

includes("lib/collab.melody/xmake.lua")

if get_config("build_examples") then
    includes("examples/xmake.lua")
end
