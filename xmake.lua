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

includes("xmake/*.lua")

add_requires("def_type") -- For serialization of the melody instructions
add_requires("miniaudio") -- Is this required for sure or only by the app that plays the sounds?

if get_config("build_examples") then
    add_requires("cli11")
end

if get_config("build_tests") then
    add_requires("catch2")
end

includes("lib/collab.platform/xmake.lua")

if get_config("build_examples") then
    includes("examples/xmake.lua")
end
