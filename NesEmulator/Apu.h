#pragma once
#include <memory>
#include <mutex>
#include <vector>
#include "Audio.h"
#include "SaveStateUtil.h"

class FrameCounter;
class PulseChannel;
class TriangleChannel;
class NoiseChannel;
class Nes;

class Apu
{
public:
	enum class Channel { Pulse1, Pulse2, Triangle, Noise };
	Apu(Nes& nes);
	Apu(Nes& nes, Snapshot& bytes);
	Snapshot SaveState() const;
	void Reset();
	void Clock();
	uint8_t ReadFromCpu(uint16_t cpuAddress, bool readonly = false);
	void WriteFromCpu(uint16_t cpuAddress, uint8_t value);
	bool GetIrq() const;

	void GetSamples(float* outBuffer);
	void SetEmulationSpeed(float speed);
private:
	float SampleChannelsAndMix();
	static constexpr int MAX_QUEUED_AUDIO_BUFFERS = 8;
	friend class FrameCounter;

	Nes& nes;
	bool enabled = false;
	bool evenFrame = true;
	float realTime = 0.0f;
	float channelVolumes[4];
	int clockNumber = 0;
	std::shared_ptr<FrameCounter> frameCounter;
	std::shared_ptr<PulseChannel> pulseChannel1;
	std::shared_ptr<PulseChannel> pulseChannel2;
	std::shared_ptr<TriangleChannel> triangleChannel;
	std::shared_ptr<NoiseChannel> noiseChannel;

	mutable std::mutex mtx;
	std::atomic_int buffersFull = 0;
	std::vector<float> finalAudioBuffer;
	std::vector<float> audioBuffer;
	std::atomic<float> emulationSpeed;

	float lastSample = 0.0f;
	bool wasStarved = true;
};
