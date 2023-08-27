#pragma once
#include "Foundation/NamespeceAlias.h"

class GameTimer {
public:
	GameTimer();	
	void Reset();
	void Start();
	void Stop();
	void StartNewFrame();
	auto GetTotalTime() const -> float;
	auto GetDeltaTime() const -> float;
	auto GetFPS() const -> std::uint32_t;
	auto GetMSPF() const -> float;
	bool IsStopped() const;
	bool IsStarted() const;
private:
	using time_point = std::chrono::steady_clock::time_point;
	time_point		_baseTime;
	time_point		_prevTime;
	time_point		_stoppedTime;
	float			_deltaTime;
	float			_pausedTime;
	float			_totalTime;
	std::uint32_t	_prevFrameTimes;
	std::uint32_t	_currFameTimes;
	std::time_t		_nextTime;
	bool			_stopped;
	bool			_newSeconds;
};

