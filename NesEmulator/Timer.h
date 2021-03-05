#pragma once
#include <chrono>

class Timer final
{
public:
	Timer(bool startTimer = true);
	void Start();
	void Stop();
	float GetElapsedSeconds() const; // DO NOT CALL MORE THAN 1000 TIMES PER SECOND
	void Reset(bool startTimer = true);
	static std::chrono::time_point<std::chrono::steady_clock> Now();
private:
	float Flush() const;
	bool running;
	mutable std::chrono::time_point<std::chrono::steady_clock> timeFlag;
	mutable float elapsedSeconds;
};

class FpsTimer
{
public:
	void Reset();
	void Update(int& out);
private:
	int frameCount = 0;
	Timer timer;
};