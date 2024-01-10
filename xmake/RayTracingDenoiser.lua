package("RayTracingDenoiser")
    -- set_policy("package.install_always", true)
    add_deps("cmake")
    set_sourcedir(path.join(os.projectdir(), "ThirdParty", "RayTracingDenoiser"))

    on_install(function (package)
        local sourceDir = path.join(os.projectdir(), "ThirdParty", "RayTracingDenoiser");
        os.exec(path.join(sourceDir, "1-Deploy.bat"));

        local configs = {}
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
        table.insert(configs, "-DNRD_STATIC_LIBRARY=" .. (package:config("shared") and "ON" or "OFF"))
        import("package.tools.cmake").install(package, configs)

        os.exec(path.join(sourceDir, "3-Prepare NRD SDK.bat"));
    end)

    on_fetch(function (package) 
        local sourceDir = path.join(os.projectdir(), "ThirdParty", "RayTracingDenoiser");
        local ndkDir = path.join(sourceDir, "_NRD_SDK");
        local linkDir = path.join(ndkDir, "Lib", "Release")
        if package:debug() then
            linkDir = path.join(ndkDir, "Lib", "Debug")
        end

        package:addenv("PATH", package:installdir(linkDir))
        local result = {}
        result.links = { "NRD" }
        result.includedirs = { path.join(ndkDir, "Include"), path.join(ndkDir, "Integration") }
        result.linkdirs = { linkDir }
        return result
    end)
package_end()