package("RayTracingDenoiser")
    set_policy("package.install_always", true)
    add_deps("cmake")
    set_sourcedir(path.join(os.projectdir(), "ThirdParty", "RayTracingDenoiser"))

    on_load(function (package)
        local shaderDir = path.join(os.projectdir(), "ThirdParty", "RayTracingDenoiser", "Shaders", "Include");
        os.trycp(path.join(shaderDir, "NRD.hlsli"), path.join(os.projectdir(), "Bin", "Assets", "Shaders"))
        os.trycp(path.join(shaderDir, "NRDEncoding.hlsli"), path.join(os.projectdir(), "Bin", "Assets", "Shaders"))
    end)

    on_install(function (package)
        local sourceDir = path.join(os.projectdir(), "ThirdParty", "RayTracingDenoiser");
        os.exec(path.join(sourceDir, "4-Clean.bat"));
        os.exec(path.join(sourceDir, "1-Deploy.bat"));

        local configs = {}
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
        table.insert(configs, "-DNRD_STATIC_LIBRARY=" .. (package:config("shared") and "ON" or "OFF"))
        table.insert(configs, "-DNRD_NORMAL_ENCODING=0")
        import("package.tools.cmake").install(package, configs)

        local binDir = path.join(os.projectdir(), "ThirdParty", "RayTracingDenoiser", "_Bin");
        local libDir = path.join(binDir, "Release")
        if package:debug() then
            libDir = path.join(binDir, "Debug")
        end

        local destDir = package:config("shared") and package:installdir("bin") or package:installdir("lib")
        os.trycp(path.join(libDir, "**.lib"), destDir)
        os.trycp(path.join(libDir, "**.dll"), destDir)
        os.trycp(path.join(libDir, "**.pdb"), destDir)
    end)

    on_fetch(function (package) 
        local linkDir = package:config("shared") and package:installdir("bin") or package:installdir("lib")
        local includeDir = path.join(os.projectdir(), "ThirdParty", "RayTracingDenoiser", "Include");
        package:addenv("PATH", package:installdir(linkDir))
        local result = {}
        result.links = { "NRD" }
        result.includedirs = { includeDir }
        result.linkdirs = { linkDir }
        return result
    end)
package_end()