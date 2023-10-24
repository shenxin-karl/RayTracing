package("renderdoc")
    on_load(function (package) 
        package:set("installdir", path.join(os.projectdir(), "ThirdParty", "renderdoc"));
    end)

    on_fetch(function (package)
        package:addenv("PATH", package:installdir("bin"))
        local result = {}
        result.links = { "renderdoc" }
        result.includedirs = package:installdir("inc")
        result.linkdirs = package:installdir("bin")
        return result
    end)
package_end()