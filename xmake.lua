local PROJECTION_DIR = os.curdir()
local RUNTIME_DIR = path.join(PROJECTION_DIR, "Runtime")
local THIRD_PARTY_DIR = path.join(PROJECTION_DIR, "ThirdParty")
local BINARY_DIR = path.join(PROJECTION_DIR, "Bin");

if is_mode("debug") then
    add_defines("MODE_DEBUG")
elseif is_mode("release") then
    add_defines("MODE_RELEASE")
else 
    add_defines("MODE_RELWITHDEBINFO")
end

set_toolset("cc", "clang-cl")
set_toolset("cxx", "clang-cl")
add_cxxflags("-std:c++20")

-- add_cxxflags("-execution-charset:utf-8")
-- add_cxxflags("-source-charset:utf-8")

add_defines("__cpp_consteval")
add_defines("NOMINMAX", "UNICODE", "_UNICODE")
add_rules("mode.debug", "mode.releasedbg")
set_arch("x64")

includes("xmake/dxc.lua")
includes("xmake/stduuid.lua")
includes("xmake/renderdoc.lua")

add_requires("fmt 9.1.0")
add_requires("spdlog v1.9.2") 
add_requires("glm")
add_requires("stduuid", {debug = isDebug})
add_requires("jsoncpp 1.9.5", {debug = isDebug, configs = {shared = false}})
add_requires("d3d12-memory-allocator v2.0.1")

target("RayTracing")
    set_languages("c++latest")
    set_warnings("all")
    set_kind("binary")
    add_headerfiles("**.h")
    add_headerfiles("**.hpp")
    add_headerfiles("**.inc")
    add_files("Runtime/**.cpp")
    add_includedirs(RUNTIME_DIR)
    add_defines("PLATFORM_WIN")
    add_defines("_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING=1") add_defines("_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS=1")

    add_packages("fmt")
    add_packages("spdlog")
    add_packages("jsoncpp")
    add_packages("glm")
    add_packages("d3d12-memory-allocator")

    -- local packages
    add_packages("stduuid")


    set_targetdir(BINARY_DIR)

    add_syslinks("Advapi32")
    add_syslinks("d3dcompiler")
    add_syslinks("D3D12")
    add_syslinks("dxgi")
    add_syslinks("User32")

    -- link
    local dxcDir = path.join(THIRD_PARTY_DIR, "dxc")
    set_values("dxcDir", dxcDir)
    link_dxc_compiler(dxcDir)

        local renderdocLibDir = path.join(THIRD_PARTY_DIR, "renderdoc")
    set_values("renderdocLibDir", renderdocLibDir)
    link_renderdoc(renderdocLibDir)

    set_values("on_install_dxc", on_install_dxc)
    set_values("on_install_renderdoc", on_install_renderdoc)
    on_install(function (target)
        target:values("on_install_dxc")(target, target:values("dxcDir"))
        target:values("on_install_renderdoc")(target, target:values("renderdocLibDir"))
    end)
target_end()
