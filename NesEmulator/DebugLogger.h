#pragma once
#include <fstream>
#include "Timer.h"
#include <exception>

class DebugLogger
{
public:
	DebugLogger(const std::string& filename) :
		file(filename, std::ios::app)
	{
		if (!file.is_open())
			throw std::exception();
	}

	void Log()
	{
		file << '[' << timer.GetElapsedSeconds() << "]" << std::endl;
	}

	template <class Arg> void Log(Arg&& obj);
	template <class Arg, class... Args> void Log(Arg&& obj, Args&&... objs);
private:
	std::ofstream file;
	Timer timer;
	bool root = true;
};

template <class Arg>
void DebugLogger::Log(Arg&& obj)
{
	if (root)
		file << '[' << timer.GetElapsedSeconds() << "]\t";
	file << '\t' << obj << std::endl;
	root = true;
}

template <class Arg, class... Args>
void DebugLogger::Log(Arg&& obj, Args&&... objs)
{
	if (root)
		file << '[' << timer.GetElapsedSeconds() << "]\t";
	root = false;
	file << '\t' << obj;
	Log(objs...);
}
