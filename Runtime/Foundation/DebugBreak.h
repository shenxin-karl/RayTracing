#pragma once

#if PLATFORM_APPLE || PLATFORM_LINUX || PLATFORM_PS4 || PLATFORM_ANDROID
EXPORT_ENGINEMODULE bool IsDebuggerPresent();
#endif

#if PLATFORM_WIN
// this header is included from almost everywhere; manually define the one function we need instead of dragging in the whole windows.h
extern "C" __declspec(dllimport) int __stdcall IsDebuggerPresent(void);
#endif

extern bool IsRunningNativeTests();

#if defined(__clang__)
    #if __has_builtin(__builtin_debugtrap)
        #define CLANG_DEBUG_BREAK __builtin_debugtrap
    #else
        #define CLANG_DEBUG_BREAK __builtin_trap
	#endif
#endif

#if PLATFORM_OSX
    #define DEBUG_BREAK     if (IsDebuggerPresent ()) CLANG_DEBUG_BREAK();
#elif PLATFORM_WIN
    #define DEBUG_BREAK     if (IsDebuggerPresent ()) __debugbreak()
#elif PLATFORM_PS4
    #include <libdbg.h>
    #define DEBUG_BREAK     if (IsDebuggerPresent ()) SCE_BREAK()
#elif ((PLATFORM_IOS || PLATFORM_TVOS) && !TARGET_IPHONE_SIMULATOR && !TARGET_TVOS_SIMULATOR)
// NOTE: On iOS we run unit tests through debugger, so don't check IsDebuggerPresent()
    #define DEBUG_BREAK     do { CLANG_DEBUG_BREAK(); } while (false)
#elif PLATFORM_ANDROID
    #define DEBUG_BREAK     do { if (IsDebuggerPresent()) { DUMP_CALLSTACK("DbgBreak: "); CLANG_DEBUG_BREAK(); } } while (false)
#elif PLATFORM_LINUX
    #include <signal.h>
    #define DEBUG_BREAK     if (IsDebuggerPresent ()) raise(SIGTRAP)
#elif !defined(DEBUG_BREAK)
    #define DEBUG_BREAK
#endif
