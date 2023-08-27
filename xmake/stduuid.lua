
package("stduuid")
    -- tag: v1.2.3
    add_deps("cmake")
    set_kind("library", {headeronly = true})
    set_sourcedir(path.join(os.projectdir(), "ThirdParty", "stduuid"))
    on_install(function (package)
        local configs = {}
        if is_mode("debug") then
            table.insert(configs, "-DCMAKE_BUILD_TYPE=Debug")
        elseif is_mode("release") then
            table.insert(configs, "-DCMAKE_BUILD_TYPE=Release")
        else
            table.insert(configs, "-DCMAKE_BUILD_TYPE=RelWithDebInfo")
        end
        import("package.tools.cmake").install(package, configs)
    end)
package_end()