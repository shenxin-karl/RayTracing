#include "GameTimer.h"

GameTimer::GameTimer() {
    Reset();
}

void GameTimer::Reset() {
    _baseTime = stdchrono::steady_clock::now();
    _prevTime = _baseTime;
    _stopped = false;
    _deltaTime = 0.f;
    _totalTime = 0.f;
    _pausedTime = 0.f;
    _prevFrameTimes = 30;
    _currFameTimes = 0;
    _nextTime = std::chrono::system_clock::to_time_t(stdchrono::system_clock::now()) + 1;
    _newSeconds = false;
    _frameCount = 0;
}

void GameTimer::Start() {
    if (!_stopped)
        return;

    _stopped = false;
    auto currentTime = stdchrono::steady_clock::now();
    _prevTime = currentTime;
    stdchrono::duration<float> diff = currentTime - _stoppedTime;
    _pausedTime += diff.count();
}

void GameTimer::Stop() {
    if (_stopped)
        return;

    _deltaTime = 0.f;
    _stopped = true;
    _stoppedTime = stdchrono::steady_clock::now();
}

void GameTimer::StartNewFrame() {
    if (_stopped)
        return;

    auto currentTime = stdchrono::steady_clock::now();
    stdchrono::duration<float> diff = currentTime - _prevTime;
    _prevTime = currentTime;
    _deltaTime = diff.count();
    diff = currentTime - _baseTime;
    _totalTime = diff.count() - _pausedTime;
    ++_currFameTimes;
    ++_frameCount;
    time_t sysTime = stdchrono::system_clock::to_time_t(stdchrono::system_clock::now());
    _newSeconds = false;
    if (sysTime >= _nextTime) {
        _nextTime = sysTime + 1;
        _prevFrameTimes = _currFameTimes;
        _currFameTimes = 0;
        _newSeconds = true;
    }
}

float GameTimer::GetTotalTime() const {
    return _totalTime;
}

float GameTimer::GetDeltaTime() const {
    return _deltaTime;
}

std::uint32_t GameTimer::GetFPS() const {
    return _prevFrameTimes;
}

float GameTimer::GetMSPF() const {
    return 1000.f / static_cast<float>(GetFPS());
}

bool GameTimer::IsStopped() const {
    return _stopped;
}

bool GameTimer::IsStarted() const {
    return !_stopped;
}

auto GameTimer::GetFrameCount() const -> uint64_t {
    return _frameCount;
}

auto GameTimer::Get() -> GameTimer & {
    static GameTimer instance;
    return instance;
}
