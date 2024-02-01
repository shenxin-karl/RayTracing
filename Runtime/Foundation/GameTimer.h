#pragma once
#include "Foundation/NamespeceAlias.h"

class GameTimer {
public:
	GameTimer();	
	void Reset();
	void Start();
	void Stop();
	void StartNewFrame();
	auto GetTotalTimeMS() const -> float;
	auto GetDeltaTimeMS() const -> float;
	auto GetTotalTimeS() const -> float;
	auto GetDeltaTimeS() const -> float;
	auto GetFPS() const -> std::uint32_t;
	auto GetMSPF() const -> float;
	bool IsStopped() const;
	bool IsStarted() const;
	auto GetFrameCount() const -> uint64_t;
	static auto Get() -> GameTimer &;
private:
	using time_point = std::chrono::steady_clock::time_point;
	// clang-format off
	time_point					_baseTime;
	time_point					_prevTime;
	time_point					_stoppedTime;
	time_point					_pausedTime;
	stdchrono::duration<float>	_deltaTime;
	stdchrono::duration<float>	_totalTime;
	std::uint32_t				_prevFrameTimes;
	std::uint32_t				_currFameTimes;
	std::time_t					_nextTime;
	bool						_stopped;
	bool						_newSeconds;
	uint64_t					_frameCount;
	// clang-format on
};

