#pragma once
#include "../Common.hpp"

namespace ER 
{
    enum class EPostureBarType
    {
        Entity,
        Boss
    };

    enum class EERDataType
    {
        Stagger,
        Stamina,
        STATUSES,
        Poison,
        Rot,
        Bleed,
        Blight,
        Frost,
        Sleep,
        Madness,
        MAX,
    };

    inline std::string to_string(const EERDataType erDataType)
    {
        switch (erDataType)
        {
        case EERDataType::Stagger:
            return "Stagger";
        case EERDataType::Stamina:
            return "Stamina";
        case EERDataType::Poison:
            return "Poison";
        case EERDataType::Rot:
            return "Rot";
        case EERDataType::Bleed:
            return "Bleed";
        case EERDataType::Blight:
            return "Blight";
        case EERDataType::Frost:
            return "Frost";
        case EERDataType::Sleep:
            return "Sleep";
        case EERDataType::Madness:
            return "Madness";
        default:
            return "";
        }
    }

    struct BarData
    {
        float value = 0.0f;
        float maxValue = 0.0f;

        void SetValue(float inValue, float inMaxValue) { value = inValue, maxValue = inMaxValue; };
        void SetValue(int inValue, int inMaxValue) { value = static_cast<float>(inValue), maxValue = static_cast<float>(inMaxValue); };
        void SetInverseValue(int inValue, int inMaxValue) { maxValue = static_cast<float>(inMaxValue); value = maxValue - static_cast<float>(inValue); };
        float GetRatio() { return value / maxValue; }
        bool IsValueMax() { return value == maxValue; }
    };

    struct PlayerPostureBarData
    {
        float maxStagger = 0.0f;
        float stagger = 0.0f;

        // Stagger reset
        float previousStagger = 1.0f; // set to 1, as 0 would conflict with checking for posture break
        bool  isResetStagger = false;
        float resetStaggerTimer = 0.0f;
        std::chrono::steady_clock::time_point lastTimePoint{};

        static inline bool  drawBar = true;
        static inline float barWidth = 286.0f;
        static inline float barHeight = 28.0f;
        static inline float resetStaggerTotalTime = 2.0f;
        static inline float screenX = 963.0f;
        static inline float screenY = 100.0f;

#ifdef DEBUGLOG
        void LogDebug();
#endif
    };

    struct BossPostureBarData
    {
        unsigned long long entityHandle = 0;
        int  displayId = -1;
        bool isStamina = false;
        bool isVisible = false;

        std::map<EERDataType, BarData> barDatas = make_map<EERDataType, BarData>();

        // Stagger reset
        float previousStagger = 1.0f; // set to 1, as 0 would conflict with checking for posture break
        bool  isResetStagger = false;
        float resetStaggerTimer = 0.0f;
        std::chrono::steady_clock::time_point lastTimePoint{};

        static inline bool  drawBars = true;
        static inline bool  useStaminaForNPC = true;
        static inline float barWidth = 998.0f;
        static inline float barHeight = 16.0f;
        static inline float resetStaggerTotalTime = 2.0f;
        static inline float firstBossScreenX = 963.0f;
        static inline float firstBossScreenY = 945.0f;
        static inline float nextBossBarDiffScreenY = 55.0f;

        // Status bars
        static inline bool  drawStatusBars = false;
        static inline float statusBarWidth = 1020.0f;
        static inline float statusBarHeight = 16.0f;
        static inline float firstStatusBarDiffScreenY = 16.0f;
        static inline float nextStatusBarDiffScreenY = 16.0f;

        static inline std::map<EERDataType, bool> drawBar = make_map<EERDataType, bool>();

#ifdef DEBUGLOG
        void LogDebug();
#endif
    };

    struct EntityPostureBarData
    {
        unsigned long long entityHandle = 0;
        bool  isStamina = false;
        bool  isVisible = false;
        float screenX = 0.0f;
        float screenY = 0.0f;
        float distanceModifier = 0.0f;

        // Position Fixing by movement velocity
        float previousScreenX = -1.0f;
        float previousScreenY = -1.0f;
        std::chrono::steady_clock::time_point gameUiUpdateTimePoint{};
        std::chrono::steady_clock::time_point gamePreviousUiUpdateTimePoint{};

        std::map<EERDataType, BarData> barDatas = make_map<EERDataType, BarData>();

        // Stagger reset
        float previousStagger = 1.0f; // set to 1, as 0 would conflict with checking for posture break
        bool  isResetStagger = false;
        float resetStaggerTimer = 0.0f;
        std::chrono::steady_clock::time_point lastTimePoint{};

        static inline bool   drawBars = true;
        static inline bool   useStaminaForNPC = true;
        static inline float  barWidth = 138.0f;
        static inline float  barHeight = 5.0f;
        static inline float  resetStaggerTotalTime = 2.0f;
        static inline float  offsetScreenX = -1.0f;
        static inline float  offsetScreenY = -10.0f;
        static inline float  leftScreenThreshold = 130.0f;
        static inline float  rightScreenThreshold = 1790.0f;
        static inline float  topScreenThreshold = 175.0f;
        static inline float  bottomScreenThreshold = 990.0f;
        static inline bool   usePositionFixing = true;
        static inline double positionFixingMultiplierX = 10.0f;
        static inline double positionFixingMultiplierY = 10.0f;

        // Status bars
        static inline bool  drawStatusBars = false;
        static inline float statusBarWidth = 143.0f;
        static inline float statusBarHeight = 14.0f;
        static inline float firstStatusBarDiffScreenY = 14.0f;
        static inline float nextStatusBarDiffScreenY = 14.0f;

        static inline std::map<EERDataType, bool> drawBar = make_map<EERDataType, bool>();

#ifdef DEBUGLOG
        void LogDebug();
#endif
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

    enum class EFillAlignment : uint_fast8_t
    {
        Left,
        Center,
        Right,
        Last
    };

    enum class EFillType : uint_fast8_t
    {
        FullToEmpty,
        EmptyToFull,
        Last
    };

    enum class EFillResizeType : uint_fast8_t
    {
        Clip,
        Scale,
        Last
    };

    struct BarStyle
    {
        static inline EFillAlignment fillAlignment = EFillAlignment::Left;
        static inline EFillType fillType = EFillType::FullToEmpty;
        static inline EFillResizeType fillResizeType = EFillResizeType::Clip;
        static inline ImVec4 staggerMaxColor = { 255, 255, 0, 255 };
        static inline ImVec4 staggerMinColor = { 255, 255, 0, 255 };
        static inline ImVec4 staminaMaxColor = { 80, 200, 104, 255 };
        static inline ImVec4 staminaMinColor = { 80, 200, 104, 255 };

        static inline ImVec4 poisonMaxColor =  { 100, 250, 85, 255 };
        static inline ImVec4 poisonMinColor =  { 100, 250, 85, 255 };
        static inline ImVec4 rotMaxColor =     { 255, 127, 0, 255 };
        static inline ImVec4 rotMinColor =     { 255, 127, 0, 255 };
        static inline ImVec4 bleedMaxColor =   { 255, 0, 0, 255 }; 
        static inline ImVec4 bleedMinColor =   { 255, 0, 0, 255 }; 
        static inline ImVec4 blightMaxColor =  { 200, 200, 0, 255 };
        static inline ImVec4 blightMinColor =  { 200, 200, 0, 255 };
        static inline ImVec4 frostMaxColor =   { 40, 225, 255, 255 };
        static inline ImVec4 frostMinColor =   { 40, 225, 255, 255 };
        static inline ImVec4 sleepMaxColor =   { 240, 0, 200, 255 };
        static inline ImVec4 sleepMinColor =   { 240, 0, 200, 255 };
        static inline ImVec4 madnessMaxColor = { 255, 255, 0, 255 };
        static inline ImVec4 madnessMinColor = { 255, 255, 0, 255 };
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
        ImColor getBarColor(EERDataType StatusEffectType, float fillRatio);
        std::pair<ImVec4, ImVec4> getMinMaxColor(EERDataType StatusEffectType);
        void drawBar(const EPostureBarType postureBarType, const EERDataType statusEffectType, const ImVec2& position, const ImVec2& size, float fillRatio);
        void drawBar(const TextureBar& textureBar, const ImColor& color, const ImVec2& position, const ImVec2& size, const std::pair<ImVec2 /* top-left */, ImVec2 /* bot-right */>& fillOffset, float fillRatio);
        void drawBar(const ImColor& color, const ImVec2& position, const ImVec2& size, float fillRatio);

        bool isMenuOpen();

        // Hooked func on update of in game UI bars
        static void updateUIBarStructs(uintptr_t moveMapStep, uintptr_t time);
        static inline void (*updateUIBarStructsOriginal)(uintptr_t, uintptr_t);

        std::optional<PlayerPostureBarData> playerPostureBar = std::nullopt;
        std::array<std::optional<EntityPostureBarData>, ENTITY_CHR_ARRAY_LEN> entityPostureBars = make_array<std::optional<EntityPostureBarData>, ENTITY_CHR_ARRAY_LEN>(std::nullopt);
        std::array<std::optional<BossPostureBarData>, BOSS_CHR_ARRAY_LEN> bossPostureBars = make_array<std::optional<BossPostureBarData>, BOSS_CHR_ARRAY_LEN>(std::nullopt);

        TextureBar entityBarTexture;
        TextureBar bossBarTexture;
        bool textureBarInit = false;
    };

    inline std::unique_ptr<PostureBarUI> g_postureUI;
};