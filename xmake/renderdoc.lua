function link_renderdoc(libDir)
    add_sysincludedirs(path.join(libDir, "inc", "app"), {public=true})
end

function on_install_renderdoc(target, libDir)
    os.cp(path.join(binDir, "bin", "renderdoc.dll", target:installdir()))
    os.cp(path.join(binDir, "bin", "renderdoc.lib", target:installdir()))
    os.cp(path.join(binDir, "bin", "renderdoc.pdb", target:installdir()))
end

