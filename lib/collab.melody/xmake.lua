target("collab.melody")
    set_kind("static")
    add_files("src/**.cppm", { public = true })
    add_files("src/**.cpp")
    add_collab_packages("def_type", { public = true })
    add_packages("miniaudio")
target_end()

if get_config("build_tests") then
    includes("tests/xmake.lua")
end
