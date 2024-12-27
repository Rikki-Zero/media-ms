add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate", {outputdir = ".vscode"})

add_includedirs("src","sqlite3")

target("sqlite3")
    set_kind("shared")
    set_languages("c11")
    set_optimize("fastest")
    set_warnings("none")
    add_files("sqlite3/**.c")

target("media-ms")
    set_kind("binary")
    set_languages("c++17")
    set_optimize("fastest")
    add_deps("sqlite3")
    add_files("src/**.cpp")