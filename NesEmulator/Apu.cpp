#include "Apu.h"
#include "Ppu.h"
#include "Nes.h"

//#define MIX_USING_LINEAR_APPROXIMATION

#define BIT(n) (1u<<n)

#ifdef _DEBUG
void assert(bool b)
{
	if (!b)
		throw "invalid apu op";
}
#else
#define assert(b)
#endif

template <class... Args>
constexpr uint32_t BITS(unsigned arg, Args... args)
{
	return BITS(args...) | BIT(arg);
}

template <>
constexpr uint32_t BITS(unsigned int arg)
{
	return BIT(arg);
}
constexpr auto Z = BITS(31);

template <typename T>
T Clamp(T value, T min, T max)
{
	return value < min ? min : value > max ? max : value;
}

template <typename T, typename U>
FORCEINLINE void SetBits(T& target, U value)
{
	target |= value;
}

template <typename T, typename U>
FORCEINLINE void ClearBits(T& target, U value)
{
	target &= ~value;
}

template <typename T, typename U>
FORCEINLINE T ReadBits(T& target, U value)
{
	return target & value;
}

template <typename T, typename U>
FORCEINLINE bool TestBits(T& target, U value)
{
	return ReadBits(target, value) != 0;
}

class LengthCounter;

// Divider outputs a clock periodically.
// Note that the term 'period' in this code really means 'period reload value', P,
// where the actual output clock period is P + 1.
class Divider
{
public:
	Divider() : m_period(0), m_counter(0) {}

	size_t GetPeriod() const { return m_period; }
	size_t GetCounter() const { return m_counter; }

	void SetPeriod(size_t period)
	{
		m_period = period;
	}

	void ResetCounter()
	{
		m_counter = m_period;
	}

	bool Clock()
	{
		// We count down from P to 0 inclusive, clocking out every P + 1 input clocks.
		if (m_counter-- == 0)
		{
			ResetCounter();
			return true;
		}
		return false;
	}

	Divider(Snapshot& bytes)
	{
		LoadBytes(bytes, m_period);
		LoadBytes(bytes, m_counter);
	}

	Snapshot SaveState() const
	{
		Snapshot bytes;
		SaveBytes(bytes, m_period);
		SaveBytes(bytes, m_counter);
		return bytes;
	}

private:
	size_t m_period;
	size_t m_counter;
};

// When LengthCounter reaches 0, corresponding channel is silenced
// http://wiki.nesdev.com/w/index.php/APU_Length_Counter
class LengthCounter
{
public:
	LengthCounter() : m_enabled(false), m_halt(false), m_counter(0) {}

	void SetEnabled(bool enabled)
	{
		m_enabled = enabled;

		// Disabling resets counter to 0, and it stays that way until enabled again
		if (!m_enabled)
			m_counter = 0;
	}

	void SetHalt(bool halt)
	{
		m_halt = halt;
	}

	void LoadCounterFromLUT(uint8_t index)
	{
		if (!m_enabled)
			return;

		static uint8_t lut[] =
		{
			10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14,
			12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
		};
		static_assert(ARRAYSIZE(lut) == 32, "Invalid");
		assert(index < ARRAYSIZE(lut));

		m_counter = lut[index];
	}

	// Clocked by FrameCounter
	void Clock()
	{
		if (m_halt) // Halting locks counter at current value
			return;

		if (m_counter > 0) // Once it reaches 0, it stops, and channel is silenced
			--m_counter;
	}

	size_t GetValue() const
	{
		return m_counter;
	}

	bool SilenceChannel() const
	{
		return m_counter == 0;
	}

	LengthCounter(Snapshot& bytes)
	{
		LoadBytes(bytes, m_enabled);
		LoadBytes(bytes, m_halt);
		LoadBytes(bytes, m_counter);
	}

	Snapshot SaveState() const
	{
		Snapshot bytes;
		SaveBytes(bytes, m_enabled);
		SaveBytes(bytes, m_halt);
		SaveBytes(bytes, m_counter);
		return bytes;
	}

private:
	bool m_enabled;
	bool m_halt;
	size_t m_counter;
};

// Controls volume in 2 ways: decreasing saw with optional looping, or constant volume
// Input: Clocked by Frame Sequencer
// Output: 4-bit volume value (0-15)
// http://wiki.nesdev.com/w/index.php/APU_Envelope
class VolumeEnvelope
{
public:
	VolumeEnvelope()
		: m_restart(true)
		, m_loop(false)
		, m_counter(0)
		, m_constantVolumeMode(false)
		, m_constantVolume(0)
	{
	}

	void Restart() { m_restart = true; }
	void SetLoop(bool loop) { m_loop = loop; }
	void SetConstantVolumeMode(bool mode) { m_constantVolumeMode = mode; }

	void SetConstantVolume(uint16_t value)
	{
		assert(value < 16);
		m_constantVolume = value & 0b1111;
		m_divider.SetPeriod(m_constantVolume); // Constant volume doubles up as divider reload value
	}

	size_t GetVolume() const
	{
		size_t result = m_constantVolumeMode ? m_constantVolume : m_counter;
		assert(result < 16);
		return result;
	}

	// Clocked by FrameCounter
	void Clock()
	{
		if (m_restart)
		{
			m_restart = false;
			m_counter = 15;
			m_divider.ResetCounter();
		}
		else
		{
			if (m_divider.Clock())
			{
				if (m_counter > 0)
				{
					--m_counter;
				}
				else if (m_loop)
				{
					m_counter = 15;
				}
			}
		}
	}

	VolumeEnvelope(Snapshot& bytes)
	{
		LoadBytes(bytes, m_restart);
		LoadBytes(bytes, m_loop);
		LoadBytes(bytes, m_counter);
		LoadBytes(bytes, m_constantVolumeMode);
		LoadBytes(bytes, m_constantVolume);
		m_divider = Divider(bytes);
	}

	Snapshot SaveState() const
	{
		Snapshot bytes;
		SaveBytes(bytes, m_restart);
		SaveBytes(bytes, m_loop);
		SaveBytes(bytes, m_counter);
		SaveBytes(bytes, m_constantVolumeMode);
		SaveBytes(bytes, m_constantVolume);
		AppendVector(bytes, m_divider.SaveState());
		return bytes;
	}

private:
	bool m_restart;
	bool m_loop;
	Divider m_divider;
	size_t m_counter; // Saw envelope volume value (if not constant volume mode)
	bool m_constantVolumeMode;
	size_t m_constantVolume; // Also reload value for divider
};

// Produces the square wave based on one of 4 duty cycles
// http://wiki.nesdev.com/w/index.php/APU_Pulse
class PulseWaveGenerator
{
public:
	PulseWaveGenerator() : m_duty(0), m_step(0) {}

	void Restart()
	{
		m_step = 0;
	}

	void SetDuty(uint8_t duty)
	{
		assert(duty < 4);
		m_duty = duty;
	}

	// Clocked by an ApuTimer, outputs bit (0 or 1)
	void Clock()
	{
		m_step = (m_step + 1) % 8;
	}

	size_t GetValue() const
	{
		static uint8_t sequences[4][8] =
		{
			{ 0, 1, 0, 0, 0, 0, 0, 0 }, // 12.5%
			{ 0, 1, 1, 0, 0, 0, 0, 0 }, // 25%
			{ 0, 1, 1, 1, 1, 0, 0, 0 }, // 50%
			{ 1, 0, 0, 1, 1, 1, 1, 1 }  // 25% negated
		};

		const uint8_t value = sequences[m_duty][m_step];
		return value;
	}

	PulseWaveGenerator(Snapshot& bytes)
	{
		LoadBytes(bytes, m_duty);
		LoadBytes(bytes, m_step);
	}

	Snapshot SaveState() const
	{
		Snapshot bytes;
		SaveBytes(bytes, m_duty);
		SaveBytes(bytes, m_step);
		return bytes;
	}

private:
	uint8_t m_duty; // 2 bits
	uint8_t m_step; // 0-7
};

// A timer is used in each of the five channels to control the sound frequency. It contains a divider which is
// clocked by the CPU clock. The triangle channel's timer is clocked on every CPU cycle, but the pulse, noise,
// and DMC timers are clocked only on every second CPU cycle and thus produce only even periods.
// http://wiki.nesdev.com/w/index.php/APU_Misc#Glossary
class ApuTimer
{
public:
	ApuTimer() : m_minPeriod(0) {}

	void Reset()
	{
		m_divider.ResetCounter();
	}

	size_t GetPeriod() const { return m_divider.GetPeriod(); }

	void SetPeriod(size_t period)
	{
		m_divider.SetPeriod(period);
	}

	void SetPeriodLow8(uint8_t value)
	{
		size_t period = m_divider.GetPeriod();
		period = (period & BITS(8, 9, 10)) | value; // Keep high 3 bits
		SetPeriod(period);
	}

	void SetPeriodHigh3(size_t value)
	{
		assert(value < BIT(3));
		size_t period = m_divider.GetPeriod();
		period = (value << 8) | (period & 0xFF); // Keep low 8 bits
		m_divider.SetPeriod(period);

		m_divider.ResetCounter();
	}

	void SetMinPeriod(size_t minPeriod)
	{
		m_minPeriod = minPeriod;
	}

	// Clocked by CPU clock every cycle (triangle channel) or second cycle (pulse/noise channels)
	// Returns true when output chip should be clocked
	bool Clock()
	{
		// Avoid popping and weird noises from ultra sonic frequencies
		if (m_divider.GetPeriod() < m_minPeriod)
			return false;

		if (m_divider.Clock())
		{
			return true;
		}
		return false;
	}

	ApuTimer(Snapshot& bytes)
	{
		LoadBytes(bytes, m_minPeriod);
		m_divider = Divider(bytes);
	}

	Snapshot SaveState() const
	{
		Snapshot bytes;
		SaveBytes(bytes, m_minPeriod);
		AppendVector(bytes, m_divider.SaveState());
		return bytes;
	}

private:
	Divider m_divider;
	size_t m_minPeriod;
};

// Periodically adjusts the period of the ApuTimer, sweeping the frequency high or low over time
// http://wiki.nesdev.com/w/index.php/APU_Sweep
class SweepUnit
{
public:
	SweepUnit()
		: m_subtractExtra(0)
		, m_enabled(false)
		, m_negate(false)
		, m_reload(false)
		, m_silenceChannel(false)
		, m_shiftCount(0)
		, m_targetPeriod(0)
	{
	}

	void SetSubtractExtra()
	{
		m_subtractExtra = 1;
	}

	void SetEnabled(bool enabled) { m_enabled = enabled; }
	void SetNegate(bool negate) { m_negate = negate; }

	void SetPeriod(size_t period, ApuTimer& timer)
	{
		assert(period < 8); // 3 bits
		m_divider.SetPeriod(period); // Don't reset counter

		// From wiki: The adder computes the next target period immediately after the period is updated by $400x writes
		// or by the frame counter.
		ComputeTargetPeriod(timer);
	}

	void SetShiftCount(uint8_t shiftCount)
	{
		assert(shiftCount < BIT(3));
		m_shiftCount = shiftCount;
	}

	void Restart() { m_reload = true; }

	// Clocked by FrameCounter
	void Clock(ApuTimer& timer)
	{
		ComputeTargetPeriod(timer);

		if (m_reload)
		{
			// From nesdev wiki: "If the divider's counter was zero before the reload and the sweep is enabled,
			// the pulse's period is also adjusted". What this effectively means is: if the divider would have
			// clocked and reset as usual, adjust the timer period.
			if (m_enabled && m_divider.Clock())
			{
				AdjustTimerPeriod(timer);
			}

			m_divider.ResetCounter();

			m_reload = false;
		}
		else
		{
			// From the nesdev wiki, it looks like the divider is always decremented, but only
			// reset to its period if the sweep is enabled.
			if (m_divider.GetCounter() > 0)
			{
				m_divider.Clock();
			}
			else if (m_enabled && m_divider.Clock())
			{
				AdjustTimerPeriod(timer);
			}
		}
	}

	bool SilenceChannel() const
	{
		return m_silenceChannel;
	}

	SweepUnit(Snapshot& bytes)
	{
		LoadBytes(bytes, m_subtractExtra);
		LoadBytes(bytes, m_enabled);
		LoadBytes(bytes, m_negate);
		LoadBytes(bytes, m_reload);
		LoadBytes(bytes, m_silenceChannel);
		LoadBytes(bytes, m_shiftCount);
		LoadBytes(bytes, m_targetPeriod);
		m_divider = Divider(bytes);
	}

	Snapshot SaveState() const
	{
		Snapshot bytes;
		SaveBytes(bytes, m_subtractExtra);
		SaveBytes(bytes, m_enabled);
		SaveBytes(bytes, m_negate);
		SaveBytes(bytes, m_reload);
		SaveBytes(bytes, m_silenceChannel);
		SaveBytes(bytes, m_shiftCount);
		SaveBytes(bytes, m_targetPeriod);
		AppendVector(bytes, m_divider.SaveState());
		return bytes;
	}

private:
	void ComputeTargetPeriod(ApuTimer& timer)
	{
		assert(m_shiftCount < 8); // 3 bits

		const size_t currPeriod = timer.GetPeriod();
		const size_t shiftedPeriod = currPeriod >> m_shiftCount;

		if (m_negate)
		{
			// Pulse 1's adder's carry is hardwired, so the subtraction adds the one's complement
			// instead of the expected two's complement (as pulse 2 does)
			m_targetPeriod = currPeriod - (shiftedPeriod - m_subtractExtra);
		}
		else
		{
			m_targetPeriod = currPeriod + shiftedPeriod;
		}

		// Channel will be silenced under certain conditions even if Sweep unit is disabled
		m_silenceChannel = (currPeriod < 8 || m_targetPeriod > 0x7FF);
	}

	void AdjustTimerPeriod(ApuTimer& timer)
	{
		// If channel is not silenced, it means we're in range
		if (m_enabled && m_shiftCount > 0 && !m_silenceChannel)
		{
			timer.SetPeriod(m_targetPeriod);
		}
	}

private:
	size_t m_subtractExtra;
	bool m_enabled;
	bool m_negate;
	bool m_reload;
	bool m_silenceChannel; // This is the Sweep -> Gate connection, if true channel is silenced
	uint8_t m_shiftCount; // [0,7]
	Divider m_divider;
	size_t m_targetPeriod; // Target period for the timer; is computed continuously in real hardware
};

// Concrete base class for audio channels
class AudioChannel
{
public:
	AudioChannel() = default;
	LengthCounter& GetLengthCounter() { return m_lengthCounter; }

	AudioChannel(Snapshot& bytes)
	{
		m_timer = ApuTimer(bytes);
		m_lengthCounter = LengthCounter(bytes);
	}

	Snapshot SaveState() const
	{
		Snapshot bytes;
		AppendVector(bytes, m_timer.SaveState());
		AppendVector(bytes, m_lengthCounter.SaveState());
		return bytes;
	}

protected:
	ApuTimer m_timer;
	LengthCounter m_lengthCounter;
};

// http://wiki.nesdev.com/w/index.php/APU_Pulse
class PulseChannel : public AudioChannel
{
public:
	PulseChannel(uint8_t pulseChannelNumber)
	{
		assert(pulseChannelNumber < 2);
		if (pulseChannelNumber == 0)
			m_sweepUnit.SetSubtractExtra();
	}

	void ClockQuarterFrameChips()
	{
		m_volumeEnvelope.Clock();
	}

	void ClockHalfFrameChips()
	{
		m_lengthCounter.Clock();
		m_sweepUnit.Clock(m_timer);
	}

	void ClockTimer()
	{
		if (m_timer.Clock())
		{
			m_pulseWaveGenerator.Clock();
		}
	}

	void HandleCpuWrite(uint16_t cpuAddress, uint8_t value)
	{
		switch (ReadBits(cpuAddress, BITS(0, 1)))
		{
		case 0:
			m_pulseWaveGenerator.SetDuty(ReadBits(value, BITS(6, 7)) >> 6);
			m_lengthCounter.SetHalt(TestBits(value, BIT(5)));
			m_volumeEnvelope.SetLoop(TestBits(value, BIT(5))); // Same bit for length counter halt and envelope loop
			m_volumeEnvelope.SetConstantVolumeMode(TestBits(value, BIT(4)));
			m_volumeEnvelope.SetConstantVolume(ReadBits(value, BITS(0, 1, 2, 3)));
			break;

		case 1: // Sweep unit setup
			m_sweepUnit.SetEnabled(TestBits(value, BIT(7)));
			m_sweepUnit.SetPeriod(ReadBits(value, BITS(4, 5, 6)) >> 4, m_timer);
			m_sweepUnit.SetNegate(TestBits(value, BIT(3)));
			m_sweepUnit.SetShiftCount(ReadBits(value, BITS(0, 1, 2)));
			m_sweepUnit.Restart(); // Side effect
			break;

		case 2:
			m_timer.SetPeriodLow8(value);
			break;

		case 3:
			m_timer.SetPeriodHigh3(ReadBits(value, BITS(0, 1, 2)));
			m_lengthCounter.LoadCounterFromLUT(ReadBits(value, BITS(3, 4, 5, 6, 7)) >> 3);

			// Side effects...
			m_volumeEnvelope.Restart();
			m_pulseWaveGenerator.Restart(); //@TODO: for pulse channels the phase is reset - IS THIS RIGHT?
			break;

		default:
			assert(false);
			break;
		}
	}

	size_t GetValue() const
	{
		if (m_sweepUnit.SilenceChannel())
			return 0;

		if (m_lengthCounter.SilenceChannel())
			return 0;

		auto value = m_volumeEnvelope.GetVolume() * m_pulseWaveGenerator.GetValue();

		assert(value < 16);
		return value;
	}

	PulseChannel(Snapshot& bytes) :
		AudioChannel(bytes)
	{
		m_volumeEnvelope = VolumeEnvelope(bytes);
		m_sweepUnit = SweepUnit(bytes);
		m_pulseWaveGenerator = PulseWaveGenerator(bytes);
	}

	Snapshot SaveState() const
	{
		Snapshot bytes;
		AppendVector(bytes, AudioChannel::SaveState());
		AppendVector(bytes, m_volumeEnvelope.SaveState());
		AppendVector(bytes, m_sweepUnit.SaveState());
		AppendVector(bytes, m_pulseWaveGenerator.SaveState());
		return bytes;
	}

private:
	VolumeEnvelope m_volumeEnvelope;
	SweepUnit m_sweepUnit;
	PulseWaveGenerator m_pulseWaveGenerator;
};

// A counter used by TriangleChannel clocked twice as often as the LengthCounter.
// Is called "linear" because it is fed the period directly rather than an index
// into a look up table like the LengthCounter.
class LinearCounter
{
public:
	LinearCounter() : m_reload(true), m_control(true) {}

	void Restart() { m_reload = true; }

	// If control is false, counter will keep reloading to input period.
	// One way to disable Triangle channel is to set control to false and
	// period to 0 (via $4008), and then restart the LinearCounter (via $400B)
	void SetControlAndPeriod(bool control, size_t period)
	{
		m_control = control;
		assert(period < BIT(7));
		m_divider.SetPeriod(period);
	}

	// Clocked by FrameCounter every CPU cycle
	void Clock()
	{
		if (m_reload)
		{
			m_divider.ResetCounter();
		}
		else if (m_divider.GetCounter() > 0)
		{
			m_divider.Clock();
		}

		if (!m_control)
		{
			m_reload = false;
		}
	}

	// If zero, sequencer is not clocked
	size_t GetValue() const
	{
		return m_divider.GetCounter();
	}

	bool SilenceChannel() const
	{
		return GetValue() == 0;
	}

	LinearCounter(Snapshot& bytes)
	{
		LoadBytes(bytes, m_reload);
		LoadBytes(bytes, m_control);
		m_divider = Divider(bytes);
	}

	Snapshot SaveState() const
	{
		Snapshot bytes;
		SaveBytes(bytes, m_reload);
		SaveBytes(bytes, m_control);
		AppendVector(bytes, m_divider.SaveState());
		return bytes;
	}

private:
	bool m_reload;
	bool m_control;
	Divider m_divider;
};

class TriangleWaveGenerator
{
public:
	TriangleWaveGenerator() : m_step(0) {}

	void Clock()
	{
		m_step = (m_step + 1) % 32;
	}

	size_t GetValue() const
	{
		static size_t sequence[] =
		{
			15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
			0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
		};
		assert(m_step < 32);
		return sequence[m_step];
	}

	TriangleWaveGenerator(Snapshot& bytes)
	{
		LoadBytes(bytes, m_step);
	}

	Snapshot SaveState() const
	{
		Snapshot bytes;
		SaveBytes(bytes, m_step);
		return bytes;
	}

private:
	uint8_t m_step;
};

class TriangleChannel : public AudioChannel
{
public:
	TriangleChannel()
	{
		m_timer.SetMinPeriod(2); // Avoid popping from ultrasonic frequencies
	}

	void ClockQuarterFrameChips()
	{
		m_linearCounter.Clock();
	}

	void ClockHalfFrameChips()
	{
		m_lengthCounter.Clock();
	}

	void ClockTimer()
	{
		if (m_timer.Clock())
		{
			if (m_linearCounter.GetValue() > 0 && m_lengthCounter.GetValue() > 0)
			{
				m_triangleWaveGenerator.Clock();
			}
		}
	}

	void HandleCpuWrite(uint16_t cpuAddress, uint8_t value)
	{
		switch (cpuAddress)
		{
		case 0x4008:
			m_lengthCounter.SetHalt(TestBits(value, BIT(7)));
			m_linearCounter.SetControlAndPeriod(TestBits(value, BIT(7)), ReadBits(value, BITS(0, 1, 2, 3, 4, 5, 6)));
			break;

		case 0x400A:
			m_timer.SetPeriodLow8(value);
			break;

		case 0x400B:
			m_timer.SetPeriodHigh3(ReadBits(value, BITS(0, 1, 2)));
			m_linearCounter.Restart(); // Side effect
			m_lengthCounter.LoadCounterFromLUT(value >> 3);
			break;

		default:
			assert(false);
			break;
		};
	}

	size_t GetValue() const
	{
		return m_triangleWaveGenerator.GetValue();
	}

	TriangleChannel(Snapshot& bytes) :
		AudioChannel(bytes)
	{
		m_linearCounter = LinearCounter(bytes);
		m_triangleWaveGenerator = TriangleWaveGenerator(bytes);
	}

	Snapshot SaveState() const
	{
		Snapshot bytes;
		AppendVector(bytes, AudioChannel::SaveState());
		AppendVector(bytes, m_linearCounter.SaveState());
		AppendVector(bytes, m_triangleWaveGenerator.SaveState());
		return bytes;
	}

	LinearCounter m_linearCounter;
	TriangleWaveGenerator m_triangleWaveGenerator;
};

class LinearFeedbackShiftRegister
{
public:
	LinearFeedbackShiftRegister() : m_register(1), m_mode(false) {}

	// Clocked by noise channel timer
	void Clock()
	{
		uint16_t bit0 = ReadBits(m_register, BIT(0));

		uint16_t whichBitN = m_mode ? 6 : 1;
		uint16_t bitN = ReadBits(m_register, BIT(whichBitN)) >> whichBitN;

		uint16_t feedback = bit0 ^ bitN;
		assert(feedback < 2);

		m_register = (m_register >> 1) | (feedback << 14);
		assert(m_register < BIT(15));
	}

	bool SilenceChannel() const
	{
		// If bit 0 is set, silence
		return TestBits(m_register, BIT(0));
	}

	LinearFeedbackShiftRegister(Snapshot& bytes)
	{
		LoadBytes(bytes, m_register);
		LoadBytes(bytes, m_mode);
	}

	Snapshot SaveState() const
	{
		Snapshot bytes;
		SaveBytes(bytes, m_register);
		SaveBytes(bytes, m_mode);
		return bytes;
	}

	uint16_t m_register;
	bool m_mode;
};

class NoiseChannel : public AudioChannel
{
public:
	NoiseChannel()
	{
		m_volumeEnvelope.SetLoop(true); // Always looping
	}

	void ClockQuarterFrameChips()
	{
		m_volumeEnvelope.Clock();
	}

	void ClockHalfFrameChips()
	{
		m_lengthCounter.Clock();
	}

	void ClockTimer()
	{
		if (m_timer.Clock())
		{
			m_shiftRegister.Clock();
		}
	}

	size_t GetValue() const
	{
		if (m_shiftRegister.SilenceChannel() || m_lengthCounter.SilenceChannel())
			return 0;

		return m_volumeEnvelope.GetVolume();
	}

	void HandleCpuWrite(uint16_t cpuAddress, uint8_t value)
	{
		switch (cpuAddress)
		{
		case 0x400C:
			m_lengthCounter.SetHalt(TestBits(value, BIT(5)));
			m_volumeEnvelope.SetConstantVolumeMode(TestBits(value, BIT(4)));
			m_volumeEnvelope.SetConstantVolume(ReadBits(value, BITS(0, 1, 2, 3)));
			break;

		case 0x400E:
			m_shiftRegister.m_mode = TestBits(value, BIT(7));
			SetNoiseTimerPeriod(ReadBits(value, BITS(0, 1, 2, 3)));
			break;

		case 0x400F:
			m_lengthCounter.LoadCounterFromLUT(value >> 3);
			m_volumeEnvelope.Restart();
			break;

		default:
			assert(false);
			break;
		};
	}

	NoiseChannel(Snapshot& bytes) :
		AudioChannel(bytes)
	{
		m_volumeEnvelope = VolumeEnvelope(bytes);
		m_shiftRegister = LinearFeedbackShiftRegister(bytes);
	}

	Snapshot SaveState() const
	{
		Snapshot bytes;
		AppendVector(bytes, AudioChannel::SaveState());
		AppendVector(bytes, m_volumeEnvelope.SaveState());
		AppendVector(bytes, m_shiftRegister.SaveState());
		return bytes;
	}

private:
	void SetNoiseTimerPeriod(size_t lutIndex)
	{
		static size_t ntscPeriods[] = { 4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068 };
		static_assert(ARRAYSIZE(ntscPeriods) == 16, "Size error");
		//size_t palPeriods[] = { 4, 8, 14, 30, 60, 88, 118, 148, 188, 236, 354, 472, 708, 944, 1890, 3778 };
		//static_assert(ARRAYSIZE(palPeriods) == 16, "Size error");

		assert(lutIndex < ARRAYSIZE(ntscPeriods));

		// The LUT contains the effective period for the channel, but the timer is clocked
		// every second CPU cycle so we divide by 2, and the divider's input is the period
		// reload value so we subtract by 1.
		const size_t periodReloadValue = (ntscPeriods[lutIndex] / 2) - 1;
		m_timer.SetPeriod(periodReloadValue);
	}

	VolumeEnvelope m_volumeEnvelope;
	LinearFeedbackShiftRegister m_shiftRegister;
};

// aka Frame Sequencer
// http://wiki.nesdev.com/w/index.php/APU_Frame_Counter
class FrameCounter
{
public:
	FrameCounter(Apu& apu)
		: m_apu(&apu)
		, m_cpuCycles(0)
		, m_numSteps(4)
		, m_inhibitInterrupt(true)
	{
	}

	//void Serialize(class Serializer& serializer)
	//{
	//	SERIALIZE(m_cpuCycles);
	//	SERIALIZE(m_numSteps);
	//	SERIALIZE(m_inhibitInterrupt);
	//}

	void SetMode(uint8_t mode)
	{
		assert(mode < 2);
		if (mode == 0)
		{
			m_numSteps = 4;
		}
		else
		{
			m_numSteps = 5;

			//@TODO: This should happen in 3 or 4 CPU cycles
			ClockQuarterFrameChips();
			ClockHalfFrameChips();
		}

		// Always restart sequence
		//@TODO: This should happen in 3 or 4 CPU cycles
		m_cpuCycles = 0;
	}

	void HandleCpuWrite(uint16_t cpuAddress, uint8_t value)
	{
		(void)cpuAddress;
		assert(cpuAddress == 0x4017);

		SetMode(ReadBits(value, BIT(7)) >> 7);

		m_inhibitInterrupt = TestBits(value, BIT(6));
		if (m_inhibitInterrupt)
			irq = false;
	}

	// Clock every CPU cycle
	void Clock()
	{
		bool resetCycles = false;

#define APU_TO_CPU_CYCLE(cpuCycle) static_cast<size_t>(cpuCycle * 2)

		switch (m_cpuCycles)
		{
		case APU_TO_CPU_CYCLE(3728.5):
			ClockQuarterFrameChips();
			break;

		case APU_TO_CPU_CYCLE(7456.5):
			ClockQuarterFrameChips();
			ClockHalfFrameChips();
			break;

		case APU_TO_CPU_CYCLE(11185.5):
			ClockQuarterFrameChips();
			break;

		case APU_TO_CPU_CYCLE(14914):
			if (m_numSteps == 4)
			{
				//@TODO: set interrupt flag if !inhibit
				Interupt();
			}
			break;

		case APU_TO_CPU_CYCLE(14914.5):
			if (m_numSteps == 4)
			{
				//@TODO: set interrupt flag if !inhibit
				Interupt();
				ClockQuarterFrameChips();
				ClockHalfFrameChips();
			}
			break;

		case APU_TO_CPU_CYCLE(14915):
			if (m_numSteps == 4)
			{
				//@TODO: set interrupt flag if !inhibit
				Interupt();
				resetCycles = true;
			}
			break;

		case APU_TO_CPU_CYCLE(18640.5):
			assert(m_numSteps == 5);
			{
				ClockQuarterFrameChips();
				ClockHalfFrameChips();
			}
			break;

		case APU_TO_CPU_CYCLE(18641):
			assert(m_numSteps == 5);
			{
				resetCycles = true;
			}
			break;
		}

		m_cpuCycles = resetCycles ? 0 : m_cpuCycles + 1;

#undef APU_TO_CPU_CYCLE
	}

	bool GetIrq() const
	{
		return irq;
	}

	void ClearIrq()
	{
		irq = false;
	}

	FrameCounter(Apu& apu, Snapshot& bytes) :
		m_apu(&apu)
	{
		LoadBytes(bytes, m_cpuCycles);
		LoadBytes(bytes, m_numSteps);
		LoadBytes(bytes, m_inhibitInterrupt);
		LoadBytes(bytes, irq);
	}

	Snapshot SaveState() const
	{
		Snapshot bytes;
		SaveBytes(bytes, m_cpuCycles);
		SaveBytes(bytes, m_numSteps);
		SaveBytes(bytes, m_inhibitInterrupt);
		SaveBytes(bytes, irq);
		return bytes;
	}

private:
	void ClockQuarterFrameChips()
	{
		m_apu->pulseChannel1->ClockQuarterFrameChips();
		m_apu->pulseChannel2->ClockQuarterFrameChips();
		m_apu->triangleChannel->ClockQuarterFrameChips();
		m_apu->noiseChannel->ClockQuarterFrameChips();
	}

	void ClockHalfFrameChips()
	{
		m_apu->pulseChannel1->ClockHalfFrameChips();
		m_apu->pulseChannel2->ClockHalfFrameChips();
		m_apu->triangleChannel->ClockHalfFrameChips();
		m_apu->noiseChannel->ClockHalfFrameChips();
	}

	void Interupt()
	{
		if (!m_inhibitInterrupt)
			irq = true;
	}

	bool irq = false;
	Apu* m_apu;
	size_t m_cpuCycles;
	size_t m_numSteps;
	bool m_inhibitInterrupt;
};

Apu::Apu(Nes& nes) :
	nes(nes)
{
	SetEmulationSpeed(1.0f);
	for (size_t i = 0; i < std::size(channelVolumes); i++)
		channelVolumes[i] = 1.0f;

	frameCounter = std::make_shared<FrameCounter>(*this);

	pulseChannel1 = std::make_shared<PulseChannel>(0);
	pulseChannel2 = std::make_shared<PulseChannel>(1);
	triangleChannel = std::make_shared<TriangleChannel>();
	noiseChannel = std::make_shared<NoiseChannel>();
}

Apu::Apu(Nes& nes, Snapshot& bytes) :
	nes(nes)
{
	LoadBytes(bytes, enabled);
	LoadBytes(bytes, evenFrame);
	LoadBytes(bytes, realTime);
	LoadBytes(bytes, channelVolumes, std::size(channelVolumes));
	LoadBytes(bytes, clockNumber);
	frameCounter = std::make_shared<FrameCounter>(*this, bytes);
	pulseChannel1 = std::make_shared<PulseChannel>(bytes);
	pulseChannel2 = std::make_shared<PulseChannel>(bytes);
	triangleChannel = std::make_shared<TriangleChannel>(bytes);
	noiseChannel = std::make_shared<NoiseChannel>(bytes);

	int nOut;
	LoadBytes(bytes, nOut);
	buffersFull = nOut;

	size_t len;
	LoadBytes(bytes, len);
	if (len > MAX_QUEUED_AUDIO_BUFFERS * Audio::SAMPLES_PER_BLOCK || len % Audio::SAMPLES_PER_BLOCK != 0)
		throw EmuFileException("invalid file");
	finalAudioBuffer.resize(len);
	LoadBytes(bytes, finalAudioBuffer.data(), finalAudioBuffer.size());

	LoadBytes(bytes, len);
	if (len >= Audio::SAMPLES_PER_BLOCK)
		throw EmuFileException("invalid file");
	audioBuffer.resize(len);
	LoadBytes(bytes, audioBuffer.data(), audioBuffer.size());
}

Snapshot Apu::SaveState() const
{
	std::unique_lock<std::mutex> lock(mtx);

	Snapshot bytes;
	SaveBytes(bytes, enabled);
	SaveBytes(bytes, evenFrame);
	SaveBytes(bytes, realTime);
	SaveBytes(bytes, channelVolumes, std::size(channelVolumes));
	SaveBytes(bytes, clockNumber);
	AppendVector(bytes, frameCounter->SaveState());
	AppendVector(bytes, pulseChannel1->SaveState());
	AppendVector(bytes, pulseChannel2->SaveState());
	AppendVector(bytes, triangleChannel->SaveState());
	AppendVector(bytes, noiseChannel->SaveState());

	SaveBytes<int>(bytes, buffersFull);
	SaveBytes(bytes, finalAudioBuffer.size());
	SaveBytes(bytes, finalAudioBuffer.data(), finalAudioBuffer.size());
	SaveBytes(bytes, audioBuffer.size());
	SaveBytes(bytes, audioBuffer.data(), audioBuffer.size());
	return bytes;
}

void Apu::Reset()
{
	evenFrame = true;
	realTime = 0;
	WriteFromCpu(0x4017, 0);
	WriteFromCpu(0x4015, 0);
	for (uint16_t addr = 0x4000; addr <= 0x400F; ++addr)
		WriteFromCpu(addr, 0);
}

void Apu::Clock()
{
	if (clockNumber == 3)
	{
		clockNumber = 0;
		frameCounter->Clock();
		triangleChannel->ClockTimer();
		if (evenFrame)
		{
			pulseChannel1->ClockTimer();
			pulseChannel2->ClockTimer();
			noiseChannel->ClockTimer();
		}
		evenFrame = !evenFrame;
	}
	clockNumber++;

	if (emulationSpeed)
		realTime += 1.0f / ((Ppu::DOT_COUNT * Ppu::SCANLINE_COUNT - 0.5f) * 60.0f) / emulationSpeed;
	while (realTime >= 1.0f / Audio::SAMPLE_RATE)
	{
		realTime -= 1.0f / Audio::SAMPLE_RATE;

		float sample = SampleChannelsAndMix();
		audioBuffer.push_back(sample);

		if (audioBuffer.size() >= Audio::SAMPLES_PER_BLOCK)
		{
			std::unique_lock<std::mutex> lock(mtx);
			buffersFull++;
			finalAudioBuffer.insert(finalAudioBuffer.end(), audioBuffer.begin(), audioBuffer.end());
			audioBuffer.clear();
			if (buffersFull > MAX_QUEUED_AUDIO_BUFFERS)
			{
				buffersFull--;
				finalAudioBuffer.erase(finalAudioBuffer.begin(), finalAudioBuffer.begin() + Audio::SAMPLES_PER_BLOCK);
			}
		}
	}
}

void Apu::GetSamples(float* outBuffer)
{
	constexpr auto BUF_LEN = Audio::SAMPLES_PER_BLOCK;
	if (wasStarved && buffersFull > 0)
	{
		for (int i = 0; i < BUF_LEN; i++)
			outBuffer[i] += finalAudioBuffer[BUF_LEN - 1] * ((float)i / BUF_LEN);
		lastSample = 0.0f;
		wasStarved = false;
	}
	else if (buffersFull == 0)
	{
		for (int i = 0; i < BUF_LEN; i++)
			outBuffer[i] += lastSample * (1.0f - (float)i / BUF_LEN);
		lastSample = 0.0f;
		wasStarved = true;
	}
	else
	{
		std::unique_lock<std::mutex> lock(mtx);
		if (!nes.mute)
		{
			for (int i = 0; i < BUF_LEN; i++)
				outBuffer[i] += finalAudioBuffer[i];
			lastSample = finalAudioBuffer[BUF_LEN - 1];
		}
		else
			lastSample = 0.0f;

		finalAudioBuffer.erase(finalAudioBuffer.begin(), finalAudioBuffer.begin() + BUF_LEN);
		buffersFull--;
	}
}

void Apu::SetEmulationSpeed(float speed)
{
	emulationSpeed = speed;
}

bool Apu::GetIrq() const
{
	return frameCounter->GetIrq();
}

uint8_t Apu::ReadFromCpu(uint16_t addr, bool readonly)
{
	uint8_t res = 0;
	if (addr == 0x4015)
	{
		//@TODO: set bits 7,6,4: DMC interrupt (I), frame interrupt (F), DMC active (D)
		//@TODO: Reading this register clears the frame interrupt flag (but not the DMC interrupt flag).
		frameCounter->ClearIrq();
		if (pulseChannel1->GetLengthCounter().GetValue() > 0)
			res |= 0b0001;
		if (pulseChannel2->GetLengthCounter().GetValue() > 0)
			res |= 0b0010;
		if (triangleChannel->GetLengthCounter().GetValue() > 0)
			res |= 0b0100;
		if (noiseChannel->GetLengthCounter().GetValue() > 0)
			res |= 0b1000;
	}
	return res;
}

void Apu::WriteFromCpu(uint16_t addr, uint8_t data)
{
	switch (addr)
	{
	case 0x4000:
	case 0x4001:
	case 0x4002:
	case 0x4003:
		pulseChannel1->HandleCpuWrite(addr, data);
		break;

	case 0x4004:
	case 0x4005:
	case 0x4006:
	case 0x4007:
		pulseChannel2->HandleCpuWrite(addr, data);
		break;

	case 0x4008:
	case 0x400A:
	case 0x400B:
		triangleChannel->HandleCpuWrite(addr, data);
		break;

	case 0x400C:
	case 0x400E:
	case 0x400F:
		noiseChannel->HandleCpuWrite(addr, data);
		break;

		/////////////////////
		// Misc
		/////////////////////
	case 0x4015:
		pulseChannel1->GetLengthCounter().SetEnabled(TestBits(data, BIT(0)));
		pulseChannel2->GetLengthCounter().SetEnabled(TestBits(data, BIT(1)));
		triangleChannel->GetLengthCounter().SetEnabled(TestBits(data, BIT(2)));
		noiseChannel->GetLengthCounter().SetEnabled(TestBits(data, BIT(3)));
		enabled = data & 0b1111;
		//@TODO: DMC Enable bit 4
		break;

	case 0x4017:
		frameCounter->HandleCpuWrite(addr, data);
		break;
	}
}

float Apu::SampleChannelsAndMix()
{
	if (!enabled)
		return 0.0f;

	// Sample all channels
	size_t pulse1 =   static_cast<size_t>(pulseChannel1->GetValue()   * channelVolumes[(int)Channel::Pulse1]);
	size_t pulse2 =   static_cast<size_t>(pulseChannel2->GetValue()   * channelVolumes[(int)Channel::Pulse2]);
	size_t triangle = static_cast<size_t>(triangleChannel->GetValue() * channelVolumes[(int)Channel::Triangle]);
	size_t noise =    static_cast<size_t>(noiseChannel->GetValue()    * channelVolumes[(int)Channel::Noise]);
	size_t dmc =      static_cast<size_t>(0.0f);

	// Mix samples
#ifdef MIX_USING_LINEAR_APPROXIMATION
	// Linear approximation (less accurate)
	float pulseOut = 0.00752f * (pulse1 + pulse2);
	float tndOut = 0.00851f * triangle + 0.00494f * noise + 0.00335f * dmc;
#else
	// Lookup Table (accurate)			
	static std::vector<float> pulseTable = []
	{
		std::vector<float> res;
		res.push_back(0.0f);
		for (size_t i = 1; i < 31; ++i)
			res.push_back(95.52f / (8128.0f / i + 100.0f));
		return res;
	}();
	static std::vector<float> tndTable = []
	{
		std::vector<float> res;
		res.push_back(0.0f);
		for (size_t i = 1; i < 203; ++i)
			res.push_back(163.67f / (24329.0f / i + 100.0f));
		return res;
	}();
	float pulseOut = pulseTable[pulse1 + pulse2];
	float tndOut = tndTable[3 * triangle + 2 * noise + dmc];
#endif

	return pulseOut + tndOut;
}
