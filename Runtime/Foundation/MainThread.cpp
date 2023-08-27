#include "MainThread.h"
#include "Exception.h"

std::thread::id MainThread::sMainThreadId = std::this_thread::get_id();
std::vector<std::function<void()>> MainThread::sBeginFrameJobQueue{};
std::vector<std::function<void()>> MainThread::sEndFrameJobQueue{};
std::mutex MainThread::sBeginFrameJobMutex{};
std::mutex MainThread::sEndFrameJobMutex{};


bool MainThread::IsMainThread() {
	return std::this_thread::get_id() == sMainThreadId;
}

void MainThread::EnsureMainThread(std::string_view message, std::source_location sl) {
	if (message.empty()) {
	    Exception::CondThrow(IsMainThread(), 
			"{}({},{}) EnsureMainThread failed!", 
			sl.file_name(), 
			sl.line(), 
			sl.column()
		);
	} else {
		FormatAndLocation formatAndLocation { message, sl };
	    Exception::CondThrow(IsMainThread(), formatAndLocation);
	}
}

auto MainThread::GetMainThreadId() -> std::thread::id {
	return sMainThreadId;
}

void MainThread::AddBeginFrameJob(std::function<void()> job) {
	std::lock_guard lock(sBeginFrameJobMutex);
	sBeginFrameJobQueue.push_back(std::move(job));
}

void MainThread::AddEndFrameJob(std::function<void()> job) {
	std::lock_guard lock(sEndFrameJobMutex);
	sEndFrameJobQueue.push_back(std::move(job));
}

void MainThread::ExecuteBeginFrameJob() {
	EnsureMainThread();
	std::unique_lock lock(sBeginFrameJobMutex);
	std::vector<std::function<void()>> queue = std::move(sBeginFrameJobQueue);
	lock.unlock();
	for (auto &job : queue) {
		job();
	}
}

void MainThread::ExecuteEndFrameJob() {
	EnsureMainThread();
	std::unique_lock lock(sEndFrameJobMutex);
	std::vector<std::function<void()>> queue = std::move(sEndFrameJobQueue);
	lock.unlock();
	for (auto &job : queue) {
		job();
	}
}
