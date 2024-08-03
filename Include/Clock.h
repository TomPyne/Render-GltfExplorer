#pragma once

#include <chrono>

class Clock
{	
	using Duration = std::chrono::high_resolution_clock::duration;
	using HighResolutionClock = std::chrono::high_resolution_clock;
	using TimePoint = std::chrono::high_resolution_clock::time_point;

public:
	Clock()
		: DeltaTime(0)
		, TotalTime(0)
	{
		LastFrameTime = HighResolutionClock::now();
	}

	void Tick()
	{
		TimePoint CurrentTime = HighResolutionClock::now();
		DeltaTime = CurrentTime - LastFrameTime;
		TotalTime += DeltaTime;
		LastFrameTime = CurrentTime;
	}

	float GetDeltaSeconds() const { return static_cast<float>(DeltaTime.count() * 1e-9); }

private:
	TimePoint LastFrameTime;
	Duration DeltaTime;
	Duration TotalTime;
};