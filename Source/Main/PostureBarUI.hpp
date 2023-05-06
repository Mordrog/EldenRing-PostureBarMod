#pragma once
#include "../Common.hpp"

namespace ER 
{
	struct BossPostureBarData
	{
		unsigned long long entityHandle = 0;
		int displayId = -1;
		bool isStamina = false;
		bool isVisible = false;
		float maxStagger = 0.0f;
		float stagger = 0.0f;

		// Stagger reset
		float previousStagger = 1.0f; // set to 1, as 0 would conflict with checking for posture break
		bool isResetStagger = false;
		float resetStaggerTimer = 0.0f;
		std::chrono::steady_clock::time_point lastTimePoint{};

		static inline bool  drawBars = true;
		static inline bool  useStaminaForNPC = true;
		static inline float barWidth = 998.0f;
		static inline float barHeight = 3.0f;
		static inline float resetStaggerTotalTime = 2.0f;
		static inline float firstBossScreenX = 963.0f;
		static inline float firstBossScreenY = 945.0f;
		static inline float nextBossBarDiffScreenY = 55.0f;
	};

	struct PostureBarData
	{
		unsigned long long entityHandle = 0;
		bool isStamina = false;
		float screenX = 0.0f;
		float screenY = 0.0f;
		float distanceModifier = 0.0f;
		bool isVisible = false;
		float maxStagger = 0.0f;
		float stagger = 0.0f;

		// Position Fixing by movement velocity
		float previousScreenX = -1.0f;
		float previousScreenY = -1.0f;
		std::chrono::steady_clock::time_point gameUiUpdateTimePoint{};
		std::chrono::steady_clock::time_point gamePreviousUiUpdateTimePoint{};

		// Stagger reset
		float previousStagger = 1.0f; // set to 1, as 0 would conflict with checking for posture break
		bool isResetStagger = false;
		float resetStaggerTimer = 0.0f;
		std::chrono::steady_clock::time_point lastTimePoint{};

		static inline bool  drawBars = true;
		static inline bool  useStaminaForNPC = true;
		static inline float barWidth = 138.0f;
		static inline float barHeight = 5.0f;
		static inline float resetStaggerTotalTime = 2.0f;
		static inline float offsetScreenX = -1.0f;
		static inline float offsetScreenY = -10.0f;
		static inline float leftScreenThreshold = 130.0f;
		static inline float rightScreenThreshold = 1790.0f;
		static inline float topScreenThreshold = 175.0f;
		static inline float bottomScreenThreshold = 990.0f;
		static inline bool  usePositionFixing = true;
		static inline double positionFixingMultiplierX = 10.0f;
		static inline double positionFixingMultiplierY = 10.0f;
	};

	struct ScreenParams
	{
		static inline float inGameCoordSizeX = 1920.0f;
		static inline float inGameCoordSizeY = 1080.0f;
		static inline float posX = 0.0f;
		static inline float posY = 0.0f;
		static inline float gameToViewportScaling = 1.0f;
		static inline bool  autoPositionSetup = true;
		static inline bool  autoGameToViewportScaling = true;
	};

	struct BarStyle
	{
		static inline ImVec4 staggerMaxColor = { 255, 255, 0, 255 };
		static inline ImVec4 staggerMinColor = { 255, 255, 0, 255 };
		static inline ImVec4 staminaMaxColor = { 80, 200, 104, 255 };
		static inline ImVec4 staminaMinColor = { 80, 200, 104, 255 };
	};

	typedef std::pair<ImVec2 /* top-left */, ImVec2 /* bot-right*/> FillTextureOffset;

	struct TextureData
	{
		ImTextureID texture = nullptr;
		float width = 0.0f;
		float height = 0.0f;

		static inline bool useTextures = true;
		static inline FillTextureOffset bossOffset = { {0.0f, 0.0f}, {0.0f, 0.0f} };
		static inline FillTextureOffset entityOffset = { {0.0f, 0.0f}, {0.0f, 0.0f} };
	};

	typedef std::pair<TextureData /* border */, TextureData /* fill */> TextureBar;

	class PostureBarUI
	{
	public:
		PostureBarUI()  noexcept = default;
		~PostureBarUI() noexcept = default;
		PostureBarUI(PostureBarUI const&) = delete;
		PostureBarUI(PostureBarUI&&) = delete;
		PostureBarUI& operator=(PostureBarUI const&) = delete;
		PostureBarUI& operator=(PostureBarUI&&) = delete;

		// Draws UI posture bars, use after starting imgui new frame
		void Draw();
		ImColor getBarColor(bool isStamina, float fillRatio);
		void drawBar(const TextureBar& textureBar, const ImColor& color, const ImVec2& position, const ImVec2& size, const std::pair<ImVec2 /* top-left */, ImVec2 /* bot-right */>& fillOffset, float fillRatio);
		void drawBar(const ImColor& color, const ImVec2& position, const ImVec2& size, float fillRatio);

		bool isMenuOpen();

		// Hooked func on update of in game UI bars
		static void updateUIBarStructs(uintptr_t moveMapStep, uintptr_t time);
		static inline void (*updateUIBarStructsOriginal)(uintptr_t, uintptr_t);

		std::array<std::optional<PostureBarData>, ENTITY_CHR_ARRAY_LEN> postureBars = make_array<std::optional<PostureBarData>, ENTITY_CHR_ARRAY_LEN>(std::nullopt);
		std::array<std::optional<BossPostureBarData>, BOSS_CHR_ARRAY_LEN> bossPostureBars = make_array<std::optional<BossPostureBarData>, BOSS_CHR_ARRAY_LEN>(std::nullopt);

		TextureBar entityBarTexture;
		TextureBar bossBarTexture;
		bool textureBarInit = false;
	};

	inline std::unique_ptr<PostureBarUI> g_postureUI;
};