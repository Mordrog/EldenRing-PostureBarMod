#include "Common.hpp"

using namespace ER;

namespace ER
{
	enum class EDebugTestState : int8_t
	{
		BossBarOffset = 0,
		EntityBarOffset = 1,
		GameScreenOffset = 2,
		PosFixingMultiplier = 3,
	};

	template<typename T>
	constexpr T cycleNumber(T cur, T min, T max) { if (cur > max) return min; else if (cur < min) return max; else return cur; };

	static EDebugTestState cycleState(EDebugTestState oldState, int8_t mov) { return EDebugTestState(cycleNumber(((int8_t)oldState + mov), 0, 3)); }

	inline HMODULE g_Module{};
	inline std::atomic_bool g_Running = TRUE;
	inline std::atomic_bool g_KillSwitch = FALSE;
	inline bool offsetTesting = FALSE;
	inline float offsetSpeed = 0.5f;
	inline float barHeightMultiplier = 1.0f;
	inline EDebugTestState debugState = EDebugTestState::GameScreenOffset;

	inline std::string dllPath = "";

	inline bool hideBarsOnMenu = true;
}

void MainThread();