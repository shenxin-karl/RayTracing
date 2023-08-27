#pragma once
#include <functional>
#include <mutex>
#include <source_location>
#include <string_view>
#include <thread>

class MainThread {
public:
	static bool IsMainThread();
	static void EnsureMainThread(std::string_view message = {}, std::source_location sl = std::source_location::current());
	static auto GetMainThreadId() -> std::thread::id;
	static void AddBeginFrameJob(std::function<void()> job);
	static void AddEndFrameJob(std::function<void()> job);
	static void ExecuteBeginFrameJob();
	static void ExecuteEndFrameJob();
private:
	static std::thread::id sMainThreadId;
	static std::vector<std::function<void()>> sBeginFrameJobQueue;
	static std::vector<std::function<void()>> sEndFrameJobQueue;
	static std::mutex sBeginFrameJobMutex;
	static std::mutex sEndFrameJobMutex;
};