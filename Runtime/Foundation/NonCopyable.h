#pragma once

struct NonCopyable {
	NonCopyable() = default;
	NonCopyable(NonCopyable &&) = default;
	NonCopyable &operator=(NonCopyable &&) = default;
	~NonCopyable() = default;
	NonCopyable(const NonCopyable &) = delete;
	NonCopyable &operator=(const NonCopyable &) = delete;
};
