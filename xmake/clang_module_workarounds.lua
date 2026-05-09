-- clang_module_workarounds.lua — linker fixes for Clang/LLVM C++20 modules
--
-- Problem: Clang emits module initializer symbols as S-type (section) symbols.
-- llvm-ar does not index S-type symbols in the archive TOC (__.SYMDEF).
-- When a shared library (plugin) links against a static archive containing
-- module interface units, the linker cannot resolve the module initializer
-- via -l because it never appears in the TOC.
--
-- Fix: link the module interface .o files directly into the plugin so the
-- initializer symbols are available without going through the archive.
--
-- Usage (inside a plugin target, after add_deps):
--
--   link_module_objects("collab.tts.kokoro", "providers/kokoro/src")
--

function link_module_objects(static_target_name, src_subdir)
    on_config(function (target)
        if not target:has_tool("cxx", "clang") then return end
        local m = get_config("mode") or "release"
        local objdir = path.join("build/.objs/" .. static_target_name,
                                 target:plat(), target:arch(), m, src_subdir)
        for _, f in ipairs(os.files(path.join(objdir, "*.cppm.o"))) do
            target:add("shflags", f, { force = true })
        end
    end)
end
