
package("dxc")
    on_load(function (package) 
        package:set("installdir", path.join(os.projectdir(), "ThirdParty", "dxc"));
    end)

    on_fetch(function (package)
        local binDir = nil
        if is_arch("x86") then
            binDir = path.join("bin", "x86")
        elseif is_arch("x64", "x86_64") then
            binDir = path.join("bin", "x64")
        elseif is_arch("arm64") then
            binDir = path.join("bin", "arm64")
        end
        package:addenv("PATH", package:installdir(binDir))
        local result = {}
        result.links = {}--{ "dxcompiler", "dxil" }
        result.includedirs = package:installdir("inc")
        result.linkdirs = package:installdir(binDir)
        return result
    end)
package_end()