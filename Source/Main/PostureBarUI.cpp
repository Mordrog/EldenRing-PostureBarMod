#include "PostureBarUI.hpp"
#include "Logger.hpp"
#include "Hooking.hpp"
#include "D3DRenderer.hpp"
#include "../PostureBarMod.hpp"

#ifndef __UINT32_MAX__
#define __UINT32_MAX__ (4294967295)
#endif

#ifndef __UINT64_MAX__
#define __UINT64_MAX__ (18446744073709551615)
#endif

namespace ER 
{
    float getTimeDifference(const LARGE_INTEGER& start, const LARGE_INTEGER& end)
    {
        static LARGE_INTEGER Frequency{};
        if (Frequency.QuadPart <= 0)
            QueryPerformanceFrequency(&Frequency);

        return (((end.QuadPart - start.QuadPart) * 1000) / Frequency.QuadPart) * 0.001f;
    }

    ImVec2 positionFixOffset(const PostureBarData& postureBar, const std::chrono::steady_clock::time_point& directXtimePoint)
    {
        float gameUIDelta = std::chrono::duration_cast<std::chrono::duration<float>>(postureBar.gameUiUpdateTimePoint - postureBar.gamePreviousUiUpdateTimePoint).count();
        float gameToDirectXDelta = std::chrono::duration_cast<std::chrono::duration<float>>(directXtimePoint - postureBar.gameUiUpdateTimePoint).count();
        float velX = 0.0f;
        float velY = 0.0f;
        if (gameUIDelta > 0)
        {
            velX = ((postureBar.screenX - postureBar.previousScreenX) * (float)PostureBarData::positionFixingMultiplierX) / gameUIDelta;
            velY = ((postureBar.screenY - postureBar.previousScreenY) * (float)PostureBarData::positionFixingMultiplierY) / gameUIDelta;
        }

        return ImVec2(velX * gameToDirectXDelta, velY * gameToDirectXDelta);
    }

    void PostureBarUI::Draw()
    {
        if (ScreenParams::autoGameToViewportScaling || ScreenParams::autoPositionSetup)
        {
            auto viewportSize = ImGui::GetMainViewport()->Size;
            float viewportScaleX = viewportSize.x / ScreenParams::inGameCoordSizeX;
            float viewportScaleY = viewportSize.y / ScreenParams::inGameCoordSizeY;

            if (ScreenParams::autoGameToViewportScaling)
            {
                ScreenParams::gameToViewportScaling = std::min(viewportScaleX, viewportScaleY);
                ScreenParams::autoGameToViewportScaling = false;
                Logger::log("Set ScreenParams::gameToViewportScaling to: " + std::to_string(ScreenParams::gameToViewportScaling));
            }

            if (ScreenParams::autoPositionSetup)
            {
                ScreenParams::posX = (viewportSize.x - std::ceilf(ScreenParams::inGameCoordSizeX * ScreenParams::gameToViewportScaling)) * 0.5f;
                ScreenParams::posY = (viewportSize.y - std::ceilf(ScreenParams::inGameCoordSizeY * ScreenParams::gameToViewportScaling)) * 0.5f;
                ScreenParams::autoPositionSetup = false;
                Logger::log("Set ScreenParams::posX to: " + std::to_string(ScreenParams::posX));
                Logger::log("Set ScreenParams::posY to: " + std::to_string(ScreenParams::posY));
            }
        }

        if (offsetTesting)
        {
            auto debugInfo = [](EDebugTestState state)
            {
                switch (state)
                {
                    using enum EDebugTestState;

                case BossBarOffset:
                    return std::tuple{ "Boss bar offset", BossPostureBarData::firstBossScreenX, BossPostureBarData::firstBossScreenY };
                case EntityBarOffset:
                    return std::tuple{ "Entity bar offset", PostureBarData::offsetScreenX, PostureBarData::offsetScreenY };
                case GameScreenOffset:
                    return std::tuple{ "Screen offset (white border should match game viewport)", ScreenParams::posX, ScreenParams::posY };
                case PosFixingMultiplier:
                    return std::tuple{ "Position fixing (set lower if bar are getting ahead, set bigger if they are dragging)", (float)PostureBarData::positionFixingMultiplierX, (float)PostureBarData::positionFixingMultiplierY };
                default:
                    return std::tuple{ "", -1.f, -1.f };
                }
            };

            auto viewportSize = ImGui::GetMainViewport()->Size;
            float adjustedViewportX = std::ceilf(ScreenParams::inGameCoordSizeX * ScreenParams::gameToViewportScaling);
            float adjustedViewportY = std::ceilf(ScreenParams::inGameCoordSizeY * ScreenParams::gameToViewportScaling);

            auto [text, x, y] = debugInfo(debugState);
            ImGui::GetBackgroundDrawList()->AddText(ImVec2(20, 15), ImColor(255, 255, 255, 255), "Offset Testing, turn off in .ini file if not needed");
            ImGui::GetBackgroundDrawList()->AddText(ImVec2(20, 35), ImColor(255, 255, 255, 255), "Press insert key to save current offset to .ini file. Press page up/down key to swap tested offset (boss bar, entity bar, screen offset, position fixing properties)");
            ImGui::GetBackgroundDrawList()->AddText(ImVec2(20, 55), ImColor(255, 255, 255, 255), "Press keyboard arrows to change values: X (left/right) Y(down/up)");
            ImGui::GetBackgroundDrawList()->AddText(ImVec2(20, 75), ImColor(255, 255, 255, 255), std::string("Window viewport: " + std::to_string((int)viewportSize.x) + "x" + std::to_string((int)viewportSize.y)).c_str());
            ImGui::GetBackgroundDrawList()->AddText(ImVec2(20, 95), ImColor(255, 255, 255, 255), std::string("Adjusted Window viewport (white border): " + std::to_string((int)adjustedViewportX) + "x" + std::to_string((int)adjustedViewportY)).c_str());
            ImGui::GetBackgroundDrawList()->AddText(ImVec2(20, 115), ImColor(255, 255, 255, 255), std::string(std::string("Game to Shown viewport Scale: ") + std::to_string(ScreenParams::gameToViewportScaling)).c_str());
            ImGui::GetBackgroundDrawList()->AddText(ImVec2(20, 135), ImColor(255, 255, 255, 255), std::string(std::string("Currently testing: ") + text).c_str());
            ImGui::GetBackgroundDrawList()->AddText(ImVec2(20, 155), ImColor(255, 255, 255, 255), std::string("X: " + std::to_string(x)).c_str());
            ImGui::GetBackgroundDrawList()->AddText(ImVec2(130, 155), ImColor(255, 255, 255, 255), std::string("Y: " + std::to_string(y)).c_str());
            ImGui::GetBackgroundDrawList()->AddRect(ImVec2(ScreenParams::posX, ScreenParams::posY), ImVec2(ScreenParams::posX + adjustedViewportX, ScreenParams::posY + adjustedViewportY), ImColor(255, 255, 255,255), 0, 0, 1.0f);
        }

        if (isMenuOpen())
        {
            Logger::log("Menu is open - not rendering bars", LogLevel::Debug);
            return;
        }

        if (BossPostureBarData::drawBars)
            for (IndexType i = 0; i < BOSS_CHR_ARRAY_LEN; i++)
                if (auto bossPostureBar = bossPostureBars[i]; bossPostureBar && bossPostureBar->isVisible)
                {
                    auto&& timePoint = std::chrono::steady_clock::now();

                    float staggerRatio = bossPostureBar->stagger / bossPostureBar->maxStagger;

                    float height = BossPostureBarData::barHeight * ScreenParams::gameToViewportScaling;
                    float width = BossPostureBarData::barWidth * ScreenParams::gameToViewportScaling;
                    ImVec2 viewportPosition = ImVec2(BossPostureBarData::firstBossScreenX, BossPostureBarData::firstBossScreenY) * ScreenParams::gameToViewportScaling;

                    // apply screen position offset
                    viewportPosition.x += ScreenParams::posX;
                    viewportPosition.y += ScreenParams::posY;

                    // apply offset if bar is for second and third boss
                    viewportPosition.y -= ((float)i * BossPostureBarData::nextBossBarDiffScreenY) * ScreenParams::gameToViewportScaling;

                    // apply offset so x is in middle of bar 
                    viewportPosition.x -= (width * 0.5f);

                    if (bossPostureBar->isResetStagger)
                    {
                        bossPostureBar->resetStaggerTimer -= std::chrono::duration_cast<std::chrono::duration<float>>(timePoint - bossPostureBar->lastTimePoint).count();
                        bossPostureBar->lastTimePoint = timePoint;

                        if (bossPostureBar->resetStaggerTimer <= 0.0f)
                            bossPostureBar->isResetStagger = false;
                        else
                        {
                            float timeRatio = 1.0f - bossPostureBar->resetStaggerTimer / BossPostureBarData::resetStaggerTotalTime;

                            auto barColor = getBarColor(bossPostureBar->isStamina, staggerRatio);

                            if (textureBarInit)
                                drawBar(bossBarTexture, barColor, viewportPosition, ImVec2(width, height), TextureData::bossOffset, timeRatio);
                            else
                                drawBar(barColor, viewportPosition, ImVec2(width, height), timeRatio);

                            continue;
                        }
                    }

                    auto barColor = getBarColor(bossPostureBar->isStamina, staggerRatio);

                    if (textureBarInit)
                        drawBar(bossBarTexture, barColor, viewportPosition, ImVec2(width, height), TextureData::bossOffset, staggerRatio);
                    else
                        drawBar(barColor, viewportPosition, ImVec2(width, height), staggerRatio);
                }

        if (PostureBarData::drawBars)
            for (IndexType i = 0; i < ENTITY_CHR_ARRAY_LEN; i++)
                if (auto postureBar = postureBars[i]; postureBar && postureBar->isVisible)
                {
                    auto&& timePoint = std::chrono::steady_clock::now();

                    float staggerRatio = postureBar->stagger / postureBar->maxStagger;
                    float distanceModifier = postureBar->distanceModifier;

                    ImVec2 gamePosition(postureBar->screenX, postureBar->screenY);

                    // apply fix offset from predicting previous movement
                    if (PostureBarData::usePositionFixing)
                        gamePosition -= positionFixOffset(*postureBar, timePoint);

                    // apply threshold to in game position
                    gamePosition.x = std::clamp(gamePosition.x, PostureBarData::leftScreenThreshold, PostureBarData::rightScreenThreshold);
                    gamePosition.y = std::clamp(gamePosition.y, PostureBarData::topScreenThreshold, PostureBarData::bottomScreenThreshold);

                    // transform in game position into viewport position
                    float height = PostureBarData::barHeight * ScreenParams::gameToViewportScaling;
                    float width = PostureBarData::barWidth * distanceModifier * ScreenParams::gameToViewportScaling;
                    ImVec2 viewportPosition = gamePosition * ScreenParams::gameToViewportScaling;

                    // apply offset so x is in middle of bar
                    viewportPosition.x -= (width * 0.5f);

                    // apply screen position offset
                    viewportPosition.x += ScreenParams::posX;
                    viewportPosition.y += ScreenParams::posY;

                    // apply user specified bar offset
                    viewportPosition.x += PostureBarData::offsetScreenX * ScreenParams::gameToViewportScaling;
                    viewportPosition.y += PostureBarData::offsetScreenY * ScreenParams::gameToViewportScaling;

                    if (postureBar->isResetStagger)
                    {
                        postureBar->resetStaggerTimer -= std::chrono::duration_cast<std::chrono::duration<float>>(timePoint - postureBar->lastTimePoint).count();
                        postureBar->lastTimePoint = timePoint;

                        if (postureBar->resetStaggerTimer <= 0.0f)
                            postureBar->isResetStagger = false;
                        else
                        {
                            float timeRatio = 1.0f - postureBar->resetStaggerTimer / PostureBarData::resetStaggerTotalTime;

                            auto barColor = getBarColor(postureBar->isStamina, timeRatio);

                            if (textureBarInit)
                                drawBar(entityBarTexture, barColor, viewportPosition, ImVec2(width, height), TextureData::entityOffset, timeRatio);
                            else
                                drawBar(barColor, viewportPosition, ImVec2(width, height), timeRatio);

                            continue;
                        }
                    }

                    auto barColor = getBarColor(postureBar->isStamina, staggerRatio);

                    if (textureBarInit)
                        drawBar(entityBarTexture, barColor, viewportPosition, ImVec2(width, height), TextureData::entityOffset, staggerRatio);
                    else
                        drawBar(barColor, viewportPosition, ImVec2(width, height), staggerRatio);
                }
    }

    ImColor PostureBarUI::getBarColor(bool isStamina, float fillRatio)
    {
        const auto& colorFrom = !isStamina ? BarStyle::staggerMaxColor : BarStyle::staminaMaxColor;
        const auto& colorTo = !isStamina ? BarStyle::staggerMinColor : BarStyle::staminaMinColor;

        // so full stagger is "from", and empty is "to"
        fillRatio = 1.0f - fillRatio;
        int r = (int)std::lerp(colorFrom.x, colorTo.x, fillRatio);
        int g = (int)std::lerp(colorFrom.y, colorTo.y, fillRatio);
        int b = (int)std::lerp(colorFrom.z, colorTo.z, fillRatio);
        int a = (int)std::lerp(colorFrom.w, colorTo.w, fillRatio);

        return ImColor(r, g, b, a);
    }

    void PostureBarUI::drawBar(const TextureBar& textureBar, const ImColor& color, const ImVec2& position, const ImVec2& size, const std::pair<ImVec2 /* top-left */, ImVec2 /* bot-right */>& fillOffset, float fillRatio)
    {
        auto&& [topLeftFillOffset, botRightFillOffset] = fillOffset;
        auto&& [barTextureBorder, barTextureFill] = textureBar;
        ImVec2 fillScale(size.x / (float)barTextureFill.width, size.y / (float)barTextureFill.height);
        ImVec2 borderScale(size.x / (float)barTextureBorder.width, size.y / (float)barTextureBorder.height);

        ImVec2 fillTopLeftScaled = topLeftFillOffset * fillScale;
        ImVec2 fillBotRightScaled = (ImVec2((float)barTextureFill.width, (float)barTextureFill.height) + botRightFillOffset) * fillScale;

        // Add Fill texture with clip to stagger ratio
        ImGui::GetBackgroundDrawList()->PushClipRect(position + fillTopLeftScaled, position + fillBotRightScaled * ImVec2(fillRatio, 1.0f));
        ImGui::GetBackgroundDrawList()->AddImage(barTextureFill.texture, position + fillTopLeftScaled, position + fillBotRightScaled, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), color);
        ImGui::GetBackgroundDrawList()->PopClipRect();
        // Add Border texture
        ImGui::GetBackgroundDrawList()->AddImage(barTextureBorder.texture, position, position + ImVec2((float)barTextureBorder.width, (float)barTextureBorder.height) * borderScale);
    }

    void PostureBarUI::drawBar(const ImColor& color, const ImVec2& position, const ImVec2& size, float fillRatio)
    {
        ImGui::GetBackgroundDrawList()->AddRectFilled(position, position + size * ImVec2(fillRatio, 1.0f), color);
    }

    bool PostureBarUI::isMenuOpen()
    {
        auto isLoad = RPM<bool>(g_Hooking->isLoading);
        auto menuState = RPM<uint8_t>(g_Hooking->menuState);

        // 18 seems to be a "gameplay" state
        return isLoad || menuState != 18;
    }

    void PostureBarUI::updateUIBarStructs(uintptr_t moveMapStep, uintptr_t time)
    {
        updateUIBarStructsOriginal(moveMapStep, time);
#ifdef _DEBUG
        try
        {
#endif // _DEBUG
        Logger::log("Entered updateUIBarStructs", LogLevel::Debug);

        auto&& worldChar = (GameData::WorldChrMan*)RPM<uintptr_t>(g_Hooking->worldChrSignature);
        auto&& feMan = (GameData::CSFeManImp*)RPM<uintptr_t>(g_Hooking->CSFeManSignature);

        static bool called = false;
        if (!worldChar || !feMan || !g_Hooking->GetChrInsFromHandleFunc)
        {
            if (!called)
                Logger::log("Failed to load in game structures", LogLevel::Warning);
            called = true;
            return;
        }
        called = false;

        for (IndexType i = 0; i < BOSS_CHR_ARRAY_LEN; i++)
            if (auto&& entityHandle = feMan->bossHpBars[i].bossHandle; entityHandle != __UINT64_MAX__)
            {
                auto&& chrIns = g_Hooking->GetChrInsFromHandleFunc(worldChar, &entityHandle);
                auto&& previousBossStaggerBar = g_postureUI->bossPostureBars[i];

                if (!chrIns || chrIns->chrModulelBag->staggerModule->staggerMax <= 0.0f)
                {
                    g_postureUI->bossPostureBars[i] = std::nullopt;
                    continue;
                }

                auto&& timePoint = std::chrono::steady_clock::now();

                BossPostureBarData bossStagerBarData;

                bossStagerBarData.entityHandle = entityHandle;
                bossStagerBarData.displayId = feMan->bossHpBars[i].displayId;
                bossStagerBarData.isStamina = BossPostureBarData::useStaminaForNPC && chrIns->modelNumber == 0;
                bossStagerBarData.maxStagger = !bossStagerBarData.isStamina ? chrIns->chrModulelBag->staggerModule->staggerMax : chrIns->chrModulelBag->statModule->staminaMax;
                bossStagerBarData.stagger = !bossStagerBarData.isStamina ? chrIns->chrModulelBag->staggerModule->stagger : chrIns->chrModulelBag->statModule->stamina;
                bossStagerBarData.isVisible = chrIns->chrModulelBag->statModule->health > 0;

                if (previousBossStaggerBar && previousBossStaggerBar->entityHandle == entityHandle)
                {
                    if (!bossStagerBarData.isStamina && BossPostureBarData::resetStaggerTotalTime > 0.0f && previousBossStaggerBar->previousStagger <= 0.0f && bossStagerBarData.stagger == bossStagerBarData.maxStagger)
                    {
                        bossStagerBarData.isResetStagger = true;
                        bossStagerBarData.resetStaggerTimer = BossPostureBarData::resetStaggerTotalTime;
                        bossStagerBarData.lastTimePoint = timePoint;
                    }
                    else
                    {
                        bossStagerBarData.isResetStagger = previousBossStaggerBar->isResetStagger;
                        bossStagerBarData.resetStaggerTimer = previousBossStaggerBar->resetStaggerTimer;
                        bossStagerBarData.lastTimePoint = previousBossStaggerBar->lastTimePoint;
                    }

                    bossStagerBarData.previousStagger = bossStagerBarData.stagger;
                }

                g_postureUI->bossPostureBars[i] = bossStagerBarData;
            }
            else
                g_postureUI->bossPostureBars[i] = std::nullopt;


        for (IndexType i = 0; i < ENTITY_CHR_ARRAY_LEN; i++)
            if (auto&& entityHandle = feMan->entityHpBars[i].entityHandle; entityHandle != __UINT64_MAX__)
            {
                auto&& chrIns = g_Hooking->GetChrInsFromHandleFunc(worldChar, &entityHandle);
                auto&& previousStaggerBar = g_postureUI->postureBars[i];

                if (!chrIns || chrIns->chrModulelBag->staggerModule->staggerMax <= 0.0f)
                {
                    g_postureUI->postureBars[i] = std::nullopt;
                    continue;
                }

                auto&& timePoint = std::chrono::steady_clock::now();

                PostureBarData staggerBarData;

                staggerBarData.entityHandle = entityHandle;
                staggerBarData.isStamina = PostureBarData::useStaminaForNPC && chrIns->modelNumber == 0;
                staggerBarData.maxStagger = !staggerBarData.isStamina ? chrIns->chrModulelBag->staggerModule->staggerMax : chrIns->chrModulelBag->statModule->staminaMax;
                staggerBarData.stagger = !staggerBarData.isStamina ? chrIns->chrModulelBag->staggerModule->stagger : chrIns->chrModulelBag->statModule->stamina;
                staggerBarData.screenX = feMan->entityHpBars[i].screenPosX;
                staggerBarData.screenY = feMan->entityHpBars[i].screenPosY;
                staggerBarData.distanceModifier = feMan->entityHpBars[i].mod;
                staggerBarData.isVisible = feMan->entityHpBars[i].isVisible && chrIns->chrModulelBag->statModule->health > 0;

                staggerBarData.gameUiUpdateTimePoint = timePoint;
                staggerBarData.gamePreviousUiUpdateTimePoint = timePoint;
                staggerBarData.previousScreenX = staggerBarData.screenX;
                staggerBarData.previousScreenY = staggerBarData.screenY;


                if (previousStaggerBar && previousStaggerBar->entityHandle == entityHandle)
                {
                    if (!staggerBarData.isStamina && BossPostureBarData::resetStaggerTotalTime > 0.0f && previousStaggerBar->previousStagger <= 0.0f && staggerBarData.stagger == staggerBarData.maxStagger)
                    {
                        staggerBarData.isResetStagger = true;
                        staggerBarData.resetStaggerTimer = PostureBarData::resetStaggerTotalTime;
                        staggerBarData.lastTimePoint = timePoint;
                    }
                    else
                    {
                        staggerBarData.isResetStagger = previousStaggerBar->isResetStagger;
                        staggerBarData.resetStaggerTimer = previousStaggerBar->resetStaggerTimer;
                        staggerBarData.lastTimePoint = previousStaggerBar->lastTimePoint;
                    }

                    staggerBarData.gamePreviousUiUpdateTimePoint = previousStaggerBar->gameUiUpdateTimePoint;
                    staggerBarData.previousScreenX = previousStaggerBar->screenX;
                    staggerBarData.previousScreenY = previousStaggerBar->screenY;
                    staggerBarData.previousStagger = staggerBarData.stagger;
                }

                g_postureUI->postureBars[i] = staggerBarData;
            }
            else
            {
                g_postureUI->postureBars[i] = std::nullopt;
            }

#ifdef _DEBUG
        }
        catch (const std::exception& e)
        {
            Logger::useLogger = true;
            Logger::log(e.what(), LogLevel::Error);
            throw;
        }
        catch (...)
        {
            Logger::useLogger = true;
            Logger::log("Unknown exception during PostureBarUI::updateUIBarStructs", LogLevel::Error);
            throw;
        }
#endif // _DEBUG
    }
}