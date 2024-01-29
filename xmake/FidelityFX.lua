package("FidelityFX")
    set_policy("package.install_always", true)
    add_deps("cmake")
    set_sourcedir(path.join(os.projectdir(), "ThirdParty", "FidelityFX-SDK", "sdk"))

    on_install(function (package)
        local sourceDir = path.join(os.projectdir(), "ThirdParty", "FidelityFX-SDK", "sdk");

        local configs = {}
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
        table.insert(configs, "-DNRD_STATIC_LIBRARY=" .. (package:config("shared") and "ON" or "OFF"))
        table.insert(configs, "-DFFX_API_CUSTOM=1")
        table.insert(configs, "-DFFX_API_DX12=1")
        table.insert(configs, "-DFFX_FSR2=1")           
        table.insert(configs, "-DBIN_OUTPUT="..path.join(package:buildir(), "bin"))
        import("package.tools.cmake").install(package, configs)
        -- copy lib

        local destDir = package:config("shared") and package:installdir("bin") or package:installdir("lib")
        local outputDir = path.join(package:buildir(), "bin", "ffx_sdk")
        os.mkdir(destDir)
        os.trycp(path.join(outputDir, "**.lib"), destDir)
        os.trycp(path.join(outputDir, "**.dll"), destDir)
        os.trycp(path.join(outputDir, "**.pdb"), destDir)
        print("outputDir: "..outputDir)
        print("destDir: "..destDir)
    end)


    on_fetch(function (package) 
        local linkDir = package:config("shared") and package:installdir("bin") or package:installdir("lib")
        local includeDir = path.join(os.projectdir(), "ThirdParty", "FidelityFX-SDK", "sdk", "include");
        package:addenv("PATH", package:installdir(linkDir))
        local result = {}
        result.links = {}
        for _, filepath in ipairs(os.files(path.join(linkDir, "*.lib"))) do
            table.insert(result.links, path.basename(filepath))
        end
        result.includedirs = { includeDir }
        result.linkdirs = { linkDir }
        return result
    end)
package_end()
