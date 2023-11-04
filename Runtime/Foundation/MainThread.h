#pragma once
#include <functional>
#include <mutex>
#include <source_location>
#include <string_view>
#include <thread>

class GameTimer;
class MainThread {
public:
	enum JobStatus {
		Finished,
		ExecuteInNextFrame
	};
	enum JobWorkTime {
		PreUpdate,
		OnUpdate,
		PostUpdate,
		PreRender,
		OnRender,
		PostRender,
	};
	using Job = std::function<JobStatus(GameTimer &gameTimer)>;
public:
	static bool IsMainThread();
	static void EnsureMainThread(std::string_view message = {}, std::source_location sl = std::source_location::current());
	static auto GetMainThreadId() -> std::thread::id;
	static void AddMainThreadJob(JobWorkTime jobWorkTime, Job job);
	static void ExecuteMainThreadJob(JobWorkTime jobWorkTime, GameTimer &gameTimer);
};