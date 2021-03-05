#include "Audio.h"
#pragma comment(lib, "winmm.lib")

AudioException::AudioException(const std::string& msg) :
	msg(msg)
{
}

const char* AudioException::what() const noexcept
{
	return msg.c_str();
}

Audio::Audio(SynthFunc synth, FilterFunc filter)
{
	this->synth = synth;
	this->filter = filter;
	if (!InitialiseAudio())
		throw AudioException("failed to initialise audio");
}

Audio::~Audio()
{
	DestroyAudio();
}

bool Audio::InitialiseAudio()
{
	thrdActive = false;
	blocksFree = BLOCK_COUNT;

	WAVEFORMATEX wf;
	wf.wFormatTag = WAVE_FORMAT_PCM;
	wf.nSamplesPerSec = SAMPLE_RATE;
	wf.wBitsPerSample = BITS_PER_SAMPLE;
	wf.nChannels = CHANNELS;
	wf.nBlockAlign = (wf.wBitsPerSample / 8) * wf.nChannels;
	wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;
	wf.cbSize = 0;

	if (waveOutOpen(&device, WAVE_MAPPER, &wf, (DWORD_PTR)Audio::WaveOutProc, (DWORD_PTR)this, CALLBACK_FUNCTION) != S_OK)
	{
		DestroyAudio();
		return false;
	}

	blocks.resize(BLOCK_COUNT * SAMPLES_PER_BLOCK);
	waveHeaders.resize(BLOCK_COUNT);

	for (int i = 0; i < BLOCK_COUNT; i++)
	{
		waveHeaders[i].dwBufferLength = SAMPLES_PER_BLOCK * sizeof(Sample);
		waveHeaders[i].lpData = (LPSTR)(blocks.data() + (i * SAMPLES_PER_BLOCK));
	}

	thrdActive = true;
	thrd = std::thread(&Audio::Run, this);
	std::unique_lock<std::mutex> lock(mtx);
	cv.notify_one();
	return true;
}

void Audio::DestroyAudio()
{
	thrdActive = false;
	if (thrd.joinable())
		thrd.join();
}

void CALLBACK Audio::WaveOutProc(HWAVEOUT device, UINT msg, DWORD_PTR inst, DWORD_PTR param1, DWORD_PTR param2)
{
	auto audio = (Audio*)inst;
	switch (msg)
	{
	case WOM_DONE:
	{
		audio->blocksFree++;
		std::unique_lock<std::mutex> lock(audio->mtx);
		audio->cv.notify_one();
		break;
	}
	case WOM_CLOSE:
		audio->deviceClosed = true;
		break;
	case WOM_OPEN:
		audio->deviceClosed = false;
		break;
	}
}

void Audio::Run()
{
	int curBlock = 0;
	std::vector<float> tmpBuffer(SAMPLES_PER_BLOCK);
	while (thrdActive)
	{
		// Wait for block to become available
		if (blocksFree == 0)
		{
			std::unique_lock<std::mutex> lock(mtx);
			while (blocksFree == 0)
				cv.wait(lock);
		}
		blocksFree--;

		// Prepare block
		if (waveHeaders[curBlock].dwFlags & WHDR_PREPARED)
			waveOutUnprepareHeader(device, &waveHeaders[curBlock], sizeof(WAVEHDR));

		synth(tmpBuffer.data());
		if (filter)
			filter(tmpBuffer.data());

		auto Clamp = [](float val)
		{
			if (val < -1.0f)
				return -1.0f;
			else if (val >= 1.0f)
				return 1.0f;
			else
				return val;
		};
		for (size_t i = 0; i < SAMPLES_PER_BLOCK; i++)
			blocks[curBlock * SAMPLES_PER_BLOCK + i] = (Sample)(Clamp(tmpBuffer[i]) * std::numeric_limits<Sample>::max());

		// Send block to audio device
		waveOutPrepareHeader(device, &waveHeaders[curBlock], sizeof(WAVEHDR));
		waveOutWrite(device, &waveHeaders[curBlock], sizeof(WAVEHDR));
		curBlock = (curBlock + 1) % BLOCK_COUNT;
	}
	waveOutReset(device);
	waveOutClose(device);
	while (!deviceClosed) {}
}
