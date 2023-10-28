#include "MainThread.h"
#include "Exception.h"
#include <magic_enum.hpp>

static std::thread::id sMainThreadId = std::this_thread::get_id();
static std::mutex sQueueMutex[magic_enum::enum_count<MainThread::JobWorkTime>()];
static std::vector<MainThread::Job> sJobQueue[magic_enum::enum_count<MainThread::JobWorkTime>()];

bool MainThread::IsMainThread() {
    return std::this_thread::get_id() == sMainThreadId;
}

void MainThread::EnsureMainThread(std::string_view message, std::source_location sl) {
    if (message.empty()) {
        Exception::CondThrow(IsMainThread(),
            "{}({},{}) EnsureMainThread failed!",
            sl.file_name(),
            sl.line(),
            sl.column());
    } else {
        FormatAndLocation formatAndLocation{message, sl};
        Exception::CondThrow(IsMainThread(), formatAndLocation);
    }
}

auto MainThread::GetMainThreadId() -> std::thread::id {
    return sMainThreadId;
}

void MainThread::AddMainThreadJob(JobWorkTime jobWorkTime, Job job) {
    size_t index = magic_enum::enum_index(jobWorkTime).value();
    std::unique_lock lock(sQueueMutex[index]);
    sJobQueue[index].push_back(std::move(job));
}

void MainThread::ExecuteMainThreadJob(JobWorkTime jobWorkTime) {
    EnsureMainThread();
    size_t index = magic_enum::enum_index(jobWorkTime).value();
    std::unique_lock lock(sQueueMutex[index]);
    std::vector<Job> queue = std::move(sJobQueue[index]);
    sJobQueue[index].clear();
    lock.unlock();

    std::vector<Job> nextFrameExecuteJob;
    for (Job &job : queue) {
	    JobStatus status = job();
        if (status == JobStatus::ExecuteInNextFrame) {
	        nextFrameExecuteJob.push_back(std::move(job));
        }
    }

    lock.lock();
    sJobQueue[index].insert(sJobQueue[index].end(), nextFrameExecuteJob.begin(), nextFrameExecuteJob.end());
    lock.unlock();
}
