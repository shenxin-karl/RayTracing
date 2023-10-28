package("pix")
    on_load(function (package) 
        package:set("installdir", path.join(os.projectdir(), "ThirdParty", "WinPixEventRuntime"));
    end)

    on_fetch(function (package)
        local binDir = nil
        if is_arch("x64", "x86_64") then
            binDir = path.join("bin", "x64")
        elseif is_arch("arm64") then
            binDir = path.join("bin", "ARM64")
        end

        package:addenv("PATH", package:installdir(binDir))
        local result = {}
        result.links = { "WinPixEventRuntime", "Shell32" }
        result.includedirs = { package:installdir("Include") }
        result.linkdirs = { package:installdir(binDir) }
        return result
    end)
package_end()