#include "Timer.h"
#include <Windows.h>
using namespace std::chrono;

time_point<steady_clock> Timer::Now()
{
	return std::chrono::high_resolution_clock::now();
}

Timer::Timer(bool startTimer) :
	elapsedSeconds(0.0f),
	timeFlag(Timer::Now()),
	running(startTimer)
{
}

float Timer::Flush() const
{
	auto curTime = Timer::Now();
	float dt = duration<float>(curTime - this->timeFlag).count();
	this->timeFlag = curTime;
	return dt;
}

void Timer::Start()
{
	if (this->running)
		return;
	this->elapsedSeconds += this->Flush();
	this->running = true;
}

void Timer::Stop()
{
	if (!this->running)
		return;
	this->Flush();
	this->running = false;
}

float Timer::GetElapsedSeconds() const
{
	if (this->running)
	{
		this->elapsedSeconds += this->Flush();
	}
	return this->elapsedSeconds;
}

void Timer::Reset(bool startTimer)
{
	this->running = startTimer;
	this->Flush();
	this->elapsedSeconds = 0.0f;
}

void FpsTimer::Reset()
{
	timer.Reset();
	frameCount = 0;
}

void FpsTimer::Update(int& out)
{
	frameCount++;
	if (timer.GetElapsedSeconds() >= 1.0f)
	{
		out = frameCount;
		while (frameCount >= 60 && timer.GetElapsedSeconds() < 1.0f)
			Sleep(5);
		Reset();
	}
}
