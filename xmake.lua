local PROJECTION_DIR = os.curdir()
local RUNTIME_DIR = path.join(PROJECTION_DIR, "Runtime")
local THIRD_PARTY_DIR = path.join(PROJECTION_DIR, "ThirdParty")
local BINARY_DIR = path.join(PROJECTION_DIR, "Bin");

local isDebug = false
if is_mode("debug") then
    isDebug = true
    add_defines("MODE_DEBUG")
elseif is_mode("release") then
    add_defines("MODE_RELEASE")
else 
    isDebug = true
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
includes("xmake/pix.lua")
includes("xmake/RayTracingDenoiser.lua")
includes("xmake/FidelityFX.lua")

add_requires("fmt 9.1.0")
add_requires("spdlog v1.9.2") 
add_requires("glm")
add_requires("jsoncpp 1.9.5", {debug = isDebug, configs = {shared = false}})
add_requires("d3d12-memory-allocator v2.0.1")
add_requires("magic_enum v0.9.0")
add_requires("stb 2023.01.30")
add_requires("assimp v5.3.1", {configs = {shared = false}})
add_requires("imgui v1.90", {debug = isDebug, configs = {shared = false}})

-- local package
add_requires("dxc")
add_requires("stduuid", {debug = isDebug})
add_requires("renderdoc")
add_requires("pix")
add_requires("RayTracingDenoiser", {debug = isDebug, configs = {shared = false}})
add_requires("FidelityFX", {debug = isDebug, configs = {shared = false}})

target("RayTracing")
    add_headerfiles("**.natvis")

    set_languages("c++latest")
    set_warnings("all")
    set_kind("binary")
    add_headerfiles("Runtime/**.h")
    add_headerfiles("Runtime/**.hpp")
    add_headerfiles("Runtime/**.inc")

    add_headerfiles("Bin/Assets/Shaders/**.hlsl")
    add_headerfiles("Bin/Assets/Shaders/**.hlsli")

    add_files("Runtime/**.cpp")
    add_includedirs(RUNTIME_DIR)
    add_defines("PLATFORM_WIN")
    add_defines("_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING=1") 
    add_defines("_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS=1")

    add_packages("fmt")
    add_packages("spdlog")
    add_packages("jsoncpp")
    add_defines("GLM_FORCE_LEFT_HANDED=1")
    add_defines("GLM_FORCE_DEPTH_ZERO_TO_ONE=1")
    add_packages("glm")
    add_packages("d3d12-memory-allocator")
    add_packages("magic_enum")
    add_packages("stb")
    add_packages("assimp")
    add_packages("imgui")

    -- local packages
    add_packages("stduuid")
    add_headerfiles("ThirdParty/stduuid/include/**.h")

    add_packages("dxc")
    add_headerfiles("ThirdParty/dxc/inc/**.h")

    add_packages("renderdoc")
    add_headerfiles("ThirdParty/renderdoc/inc/**.h")

    add_defines("USE_PIX=1")
    add_packages("pix")
    add_headerfiles("ThirdParty/WinPixEventRuntime/Include/WinPixEventRuntime/**.h")

    add_packages("RayTracingDenoiser")
    add_headerfiles("ThirdParty/RayTracingDenoiser/Include/**.h")

    add_packages("FidelityFX")
    add_defines("FFX_FSR=1")
    add_headerfiles("ThirdParty/FidelityFX-SDK/sdk/include/**.h")

    set_targetdir(BINARY_DIR)

    add_syslinks("Advapi32")
    add_syslinks("d3dcompiler")
    add_syslinks("D3D12")
    add_syslinks("dxgi")
    add_syslinks("User32")
    add_syslinks("Shcore")
target_end()
