-- collab.lua — shared helper for BuildWithCollab projects
--
-- BuildWithCollab package registry — declared every time this file is
-- included. xmake dedups repository entries by URL, so re-declaring is
-- safe and ensures every consuming xmake.lua sees the registry.
add_repositories("BuildWithCollab https://github.com/BuildWithCollab/Packages")

-- Include-guarded below this line: a downstream collab repo's own xmake.lua
-- may re-`includes` this same file. Without the guard, every re-include
-- resets the file-local state (e.g. `_explicit_locals`) and prior
-- `use_collab_local()` calls vanish.
if _collab_lua_loaded then return end
_collab_lua_loaded = true
--
-- Usage in xmake.lua:
--   includes("xmake/collab.lua")
--
--   -- Top level (like add_requires)
--   add_collab_requires("collab.core", "collab.process")
--   add_collab_requires("collab.core", { configs = { shared = true } })
--
--   -- Inside a target (like add_packages)
--   target("myapp")
--       add_collab_packages("collab.core", "collab.process")
--       add_collab_packages("collab.core", { public = true })
--
--   -- Inside a target, local-only dep on a specific sub-target
--   -- (for when a collab package folder defines multiple targets and
--   --  you haven't yet decided what the shipped package name will be).
--   -- Errors loudly if the package isn't local — that's the feature:
--   -- the error is a reminder to pick a shipped-package name.
--   target("myapp")
--       add_collab_deps("agentic:agentic-client")
--       add_collab_deps("agentic:agentic-client", { public = true })
--
-- All collab package names use dots (the target/module name). The helpers
-- convert to dashes internally for folder lookups and xmake package names.
-- For add_collab_deps, only the LEFT side of the colon is a collab package
-- reference; the RIGHT side is the literal xmake target name.
--
-- Functions:
--   use_collab_local("collab.core")  — mark a package as local (alternative to env var)
--
-- Environment variables:
--   BUILDWITHCOLLAB_ROOT   — path to the folder containing all collab repos
--                              (e.g. C:\Code\mrowr\BuildWithCollab)
--   BUILDWITHCOLLAB_LOCAL  — comma-delimited list of package names to pull
--                              from local folders instead of the xmake registry
--                              (e.g. "collab-core,collab-process")
--                              (env var uses dashes — folder names)

-- dots → dashes (collab.core → collab-core)
local function _to_dash(name)
    return name:gsub("%.", "-")
end

-- If the last arg is a table, pop it as opts. Returns (names, opts).
local function _split_opts(args)
    local opts = {}
    if type(args[#args]) == "table" then
        opts = table.remove(args)
    end
    return args, opts
end

-- Packages marked local via use_collab_local() — merged with BUILDWITHCOLLAB_LOCAL
-- Stored as dash names internally (folder/package names)
local _explicit_locals = {}

-- Takes dotted names: use_collab_local("collab.core")
function use_collab_local(...)
    for _, name in ipairs({...}) do
        _explicit_locals[_to_dash(name)] = true
    end
end

-- Returns a set of dash names + the root path
local function _get_local_set()
    local root = os.getenv("BUILDWITHCOLLAB_ROOT")
    local local_csv = os.getenv("BUILDWITHCOLLAB_LOCAL")

    if not root or not os.isdir(root) then return {}, nil end

    -- Start with explicitly marked locals
    local set = {}
    for name, _ in pairs(_explicit_locals) do
        set[name] = true
    end

    -- Merge in env var entries (already dashes)
    if local_csv and #local_csv > 0 then
        for name in local_csv:gmatch("([^,]+)") do
            local trimmed = name:match("^%s*(.-)%s*$")
            if #trimmed > 0 then
                set[trimmed] = true
            end
        end
    end

    return set, root
end

-- Top level — like add_requires, but checks local first.
-- Takes dotted names + optional opts table (opts are passed to add_requires
-- when non-local, ignored for local includes):
--   add_collab_requires("collab.core", "collab.process")
--   add_collab_requires("collab.core", { configs = { shared = true } })
function add_collab_requires(...)
    local names, opts = _split_opts({...})
    local local_set, root = _get_local_set()

    local not_found = {}

    for _, name in ipairs(names) do
        local dash = _to_dash(name)
        if local_set[dash] then
            local xmake_file = path.join(root, dash, "xmake.lua")
            if os.isfile(xmake_file) then
                includes(xmake_file)
            else
                table.insert(not_found, dash)
            end
        else
            add_requires(dash, opts)
        end
    end

    if #not_found > 0 then
        os.raise("BUILDWITHCOLLAB_LOCAL lists packages not found in "
            .. root .. ":\n  " .. table.concat(not_found, ", ")
            .. "\n\nClone them or remove from BUILDWITHCOLLAB_LOCAL.")
    end
end

-- Inside a target — like add_packages, but uses add_deps for local
-- Takes dotted names + optional opts table:
--   add_collab_packages("collab.core")
--   add_collab_packages("collab.core", { public = true })
function add_collab_packages(...)
    local args, opts = _split_opts({...})
    local local_set, root = _get_local_set()

    for _, name in ipairs(args) do
        local dash = _to_dash(name)
        if local_set[dash] and root and os.isfile(path.join(root, dash, "xmake.lua")) then
            add_deps(name, opts)         -- dotted target name
        else
            add_packages(dash, opts)     -- dashed package name
        end
    end
end

-- Inside a target — local-dev-only scaffold for depending on a specific
-- sub-target of a collab package whose shipped name hasn't been decided yet.
--
-- Syntax: "pkg:target" (colon required, or it errors).
--   - "pkg" is the collab package reference (dotted, same as other helpers)
--   - "target" is the literal xmake target name inside that package
--
-- If "pkg" is local → add_deps("target", opts)
-- If "pkg" is NOT local → raise() with an instruction to replace this call
--   with add_collab_packages(...). That error IS the feature: it forces
--   you to pick the shipped package name before you can build non-locally.
--
-- Takes N args + optional opts table:
--   add_collab_deps("agentic:agentic-client")
--   add_collab_deps("agentic:agentic-client", "agentic:agentic-server")
--   add_collab_deps("agentic:agentic-client", { public = true })
function add_collab_deps(...)
    local args, opts = _split_opts({...})
    local local_set = _get_local_set()

    for _, name in ipairs(args) do
        local pkg, target = name:match("^([^:]+):([^:]+)$")
        if not pkg then
            os.raise("add_collab_deps requires \"pkg:target\" syntax, got: \""
                .. tostring(name) .. "\"")
        end
        local dash = _to_dash(pkg)
        if local_set[dash] then
            add_deps(target, opts)
        else
            os.raise("add_collab_deps(\"" .. name .. "\") — \"" .. pkg
                .. "\" is not a local package.\n\n"
                .. "This helper is for local dev only. Either mark \"" .. pkg
                .. "\" local (BUILDWITHCOLLAB_LOCAL or use_collab_local),\n"
                .. "or replace this call with add_collab_packages(\"" .. pkg
                .. "\") / add_collab_packages(\"" .. target .. "\")\n"
                .. "once you've decided the shipped package name.")
        end
    end
end
