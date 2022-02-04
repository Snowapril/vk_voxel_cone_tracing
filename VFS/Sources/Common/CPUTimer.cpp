#include <Common/pch.h>
#include <Common/CPUTimer.h>

namespace vfs
{
	CPUTimer::CPUTimer() noexcept
	{
		reset();
	}

	void CPUTimer::reset(void) noexcept
	{
		_startTime = std::chrono::steady_clock::now();
	}

	float CPUTimer::elapsedSeconds(void) const noexcept
	{
		std::chrono::steady_clock::time_point nowTime = std::chrono::steady_clock::now();
		return std::chrono::duration<float, std::chrono::seconds::period>(nowTime - _startTime).count();
	}

	float CPUTimer::elapsedMilliSeconds(void) const noexcept
	{
		std::chrono::steady_clock::time_point nowTime = std::chrono::steady_clock::now();
		return std::chrono::duration<float, std::chrono::milliseconds::period>(nowTime - _startTime).count();
	}
};