#pragma once

class CompileEnvInfo {
public:
	static consteval bool IsModeDebug() noexcept {
#if defined(MODE_DEBUG)
		return true;
#else
		return false;
#endif
	}

	static consteval bool IsModeRelease() noexcept {
#if defined(MODE_RELEASE)
		return true;
#else
		return false;
#endif
	}

	static constexpr bool IsModeRelWithDebInfo() noexcept {
#if defined(MODE_RELWITHDEBINFO)
		return true;
#else
		return false;
#endif
	}
};