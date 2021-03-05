#pragma once
#include <condition_variable>
#include <Windows.h>
#include <atomic>
#include <thread>
#include <vector>
#undef min
#undef max

using Sample = int16_t;
class Audio
{
public:
	using SynthFunc = void(*)(float* outBlock);
	using FilterFunc = void(*)(float* block);
	static constexpr int BLOCK_COUNT = 8;
	static constexpr int SAMPLES_PER_BLOCK = 512;
	static constexpr int SAMPLE_RATE = 44100;
	static constexpr int BITS_PER_SAMPLE = sizeof(Sample) * 8;
	static constexpr int CHANNELS = 1;

	Audio(SynthFunc synth, FilterFunc filter = nullptr);
	~Audio();
private:
	static void CALLBACK WaveOutProc(HWAVEOUT device, UINT msg, DWORD_PTR inst, DWORD_PTR param1, DWORD_PTR param2);
	bool InitialiseAudio();
	void DestroyAudio();
	std::vector<Sample> blocks;
	std::vector<WAVEHDR> waveHeaders;
	HWAVEOUT device;
	std::atomic_int blocksFree;
	std::condition_variable cv;
	std::mutex mtx;

	void Run();
	std::thread thrd;
	std::atomic_bool thrdActive;
	std::atomic_bool deviceClosed;
	SynthFunc synth;
	FilterFunc filter;
};

class AudioException : public std::exception
{
public:
	AudioException(const std::string& msg = "an audio error occurred");
	const char* what() const noexcept override;
private:
	std::string msg;
};
