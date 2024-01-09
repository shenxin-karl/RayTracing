package("NRI")
    set_policy("package.install_always", true)
    add_deps("cmake")
    set_sourcedir(path.join(os.projectdir(), "ThirdParty", "NRI"))

    on_install(function (package)
        local sourceDir = path.join(os.projectdir(), "ThirdParty", "NRI");
        os.exec(path.join(sourceDir, "1-Deploy.bat"));

        local configs = {}
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
        table.insert(configs, "-DNRI_STATIC_LIBRARY=" .. (package:config("shared") and "ON" or "OFF"))
        import("package.tools.cmake").install(package, configs)

        os.exec(path.join(sourceDir, "3-Prepare NRI SDK.bat"));
    end)

    on_fetch(function (package) 
        local sourceDir = path.join(os.projectdir(), "ThirdParty", "NRI");
        local ndkDir = path.join(sourceDir, "_NRI_SDK");
        local linkDir = path.join(ndkDir, "Lib", "Release")
        if package:debug() then
            linkDir = path.join(ndkDir, "Lib", "Debug")
        end

        package:addenv("PATH", package:installdir(linkDir))
        local result = {}
        result.links = { "NRI" }
        result.includedirs = { path.join(ndkDir, "Include") }
        result.linkdirs = { linkDir }
        return result
    end)
package_end()