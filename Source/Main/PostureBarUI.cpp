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
    ImVec2 positionFixOffset(const EntityPostureBarData& postureBar, const std::chrono::steady_clock::time_point& directXtimePoint)
    {
        float gameUIDelta = std::chrono::duration_cast<std::chrono::duration<float>>(postureBar.gameUiUpdateTimePoint - postureBar.gamePreviousUiUpdateTimePoint).count();
        float gameToDirectXDelta = std::chrono::duration_cast<std::chrono::duration<float>>(directXtimePoint - postureBar.gameUiUpdateTimePoint).count();
        float velX = 0.0f;
        float velY = 0.0f;
        if (gameUIDelta > 0)
        {
            velX = ((postureBar.screenX - postureBar.previousScreenX) * (float)EntityPostureBarData::positionFixingMultiplierX) / gameUIDelta;
            velY = ((postureBar.screenY - postureBar.previousScreenY) * (float)EntityPostureBarData::positionFixingMultiplierY) / gameUIDelta;
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
                    return std::tuple{ "Boss bar screen offset", BossPostureBarData::firstBossScreenX, BossPostureBarData::firstBossScreenY };
                case EntityBarOffset:
                    return std::tuple{ "Entity bar screen offset", EntityPostureBarData::offsetScreenX, EntityPostureBarData::offsetScreenY };
                case GameScreenOffset:
                    return std::tuple{ "Screen offset (white border should match game viewport)", ScreenParams::posX, ScreenParams::posY };
                case PosFixingMultiplier:
                    return std::tuple{ "Position fixing (set lower if bar are getting ahead, set bigger if they are dragging)", (float)EntityPostureBarData::positionFixingMultiplierX, (float)EntityPostureBarData::positionFixingMultiplierY };
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
        
        if (PlayerPostureBarData::drawBar)
        {
            if (auto _playerPostureBar = playerPostureBar; _playerPostureBar)
            {
                float height = PlayerPostureBarData::barHeight * ScreenParams::gameToViewportScaling;
                float width = PlayerPostureBarData::barWidth * ScreenParams::gameToViewportScaling;
                ImVec2 barSize = ImVec2(width, height);
                ImVec2 positionOnViewport = ImVec2(PlayerPostureBarData::screenX, PlayerPostureBarData::screenY) * ScreenParams::gameToViewportScaling;

                // apply screen position offset
                positionOnViewport.x += ScreenParams::posX;
                positionOnViewport.y += ScreenParams::posY;

                if (_playerPostureBar->isResetStagger)
                {
                    float timeRatio = 1.0f - _playerPostureBar->resetStaggerTimer / PlayerPostureBarData::resetStaggerTotalTime;

                    drawBar(EPostureBarType::Entity, EERDataType::Stagger, positionOnViewport, barSize, timeRatio);
                }
                else
                {
                    float fillRatio = _playerPostureBar->stagger / _playerPostureBar->maxStagger;

                    drawBar(EPostureBarType::Entity, EERDataType::Stagger, positionOnViewport, barSize, fillRatio);
                }
            }
        }

        if (BossPostureBarData::drawBars)
        {
            for (IndexType i = 0; i < BOSS_CHR_ARRAY_LEN; i++)
            {
                if (auto bossPostureBar = bossPostureBars[i]; bossPostureBar && bossPostureBar->isVisible)
                {
                    float height = BossPostureBarData::barHeight * ScreenParams::gameToViewportScaling;
                    float width = BossPostureBarData::barWidth * ScreenParams::gameToViewportScaling;
                    ImVec2 barSize = ImVec2(width, height);
                    ImVec2 viewportPosition = ImVec2(BossPostureBarData::firstBossScreenX, BossPostureBarData::firstBossScreenY) * ScreenParams::gameToViewportScaling;

                    // apply screen position offset
                    viewportPosition.x += ScreenParams::posX;
                    viewportPosition.y += ScreenParams::posY;

                    // apply offset if bar is for second and third boss
                    viewportPosition.y -= ((float)i * BossPostureBarData::nextBossBarDiffScreenY) * ScreenParams::gameToViewportScaling;

                    if (bossPostureBar->isResetStagger)
                    {
                        float timeRatio = 1.0f - bossPostureBar->resetStaggerTimer / BossPostureBarData::resetStaggerTotalTime;

                        drawBar(EPostureBarType::Boss, EERDataType::Stagger, viewportPosition, barSize, timeRatio);
                    }
                    else
                    {
                        EERDataType erDataType = bossPostureBar->isStamina ? EERDataType::Stamina : EERDataType::Stagger;
                        float fillRatio = bossPostureBar->barDatas[erDataType].GetRatio();

                        drawBar(EPostureBarType::Boss, erDataType, viewportPosition, barSize, fillRatio);
                    }

                    if (bossPostureBar->drawStatusBars)
                    {
                        float statusHeight = BossPostureBarData::statusBarHeight * ScreenParams::gameToViewportScaling;
                        float statusWidth = BossPostureBarData::statusBarWidth * ScreenParams::gameToViewportScaling;
                        ImVec2 statusBarSize = ImVec2(statusWidth, statusHeight);

                        viewportPosition.y += BossPostureBarData::firstStatusBarDiffScreenY;
                        for (size_t i = static_cast<size_t>(EERDataType::STATUSES) + 1; i < static_cast<size_t>(EERDataType::MAX); ++i)
                        {
                            EERDataType statusEffectType = static_cast<EERDataType>(i);
                            if (bossPostureBar->drawBar[statusEffectType])
                            {
                                float fillRatio = bossPostureBar->barDatas[statusEffectType].GetRatio();
                                if (fillRatio < 0.01f)
                                    continue;

                                drawBar(EPostureBarType::Boss, statusEffectType, viewportPosition, statusBarSize, fillRatio);
                                viewportPosition.y += BossPostureBarData::nextStatusBarDiffScreenY;
                            }
                        }
                    }
                }
            }
        }

        if (EntityPostureBarData::drawBars)
        {
            for (IndexType i = 0; i < ENTITY_CHR_ARRAY_LEN; i++)
            {
                if (auto entityPostureBar = entityPostureBars[i]; entityPostureBar && entityPostureBar->isVisible)
                {
                    auto&& timePoint = std::chrono::steady_clock::now();

                    float distanceModifier = entityPostureBar->distanceModifier;

                    ImVec2 gamePosition(entityPostureBar->screenX, entityPostureBar->screenY);

                    // apply fix offset from predicting previous movement
                    if (EntityPostureBarData::usePositionFixing)
                        gamePosition -= positionFixOffset(*entityPostureBar, timePoint);

                    // apply threshold to in game position
                    gamePosition.x = std::clamp(gamePosition.x, EntityPostureBarData::leftScreenThreshold, EntityPostureBarData::rightScreenThreshold);
                    gamePosition.y = std::clamp(gamePosition.y, EntityPostureBarData::topScreenThreshold, EntityPostureBarData::bottomScreenThreshold);

                    // transform in game position into viewport position
                    float height = EntityPostureBarData::barHeight * ScreenParams::gameToViewportScaling;
                    float width = EntityPostureBarData::barWidth * distanceModifier * ScreenParams::gameToViewportScaling;
                    ImVec2 barSize = ImVec2(width, height);
                    ImVec2 viewportPosition = gamePosition * ScreenParams::gameToViewportScaling;

                    // apply screen position offset
                    viewportPosition.x += ScreenParams::posX;
                    viewportPosition.y += ScreenParams::posY;

                    // apply user specified bar offset
                    viewportPosition.x += EntityPostureBarData::offsetScreenX * ScreenParams::gameToViewportScaling;
                    viewportPosition.y += EntityPostureBarData::offsetScreenY * ScreenParams::gameToViewportScaling;

                    if (entityPostureBar->isResetStagger)
                    {
                        float timeRatio = 1.0f - entityPostureBar->resetStaggerTimer / EntityPostureBarData::resetStaggerTotalTime;

                        drawBar(EPostureBarType::Entity, EERDataType::Stagger, viewportPosition, barSize, timeRatio);
                    }
                    else
                    {
                        EERDataType erDataType = entityPostureBar->isStamina ? EERDataType::Stamina : EERDataType::Stagger;
                        float fillRatio = entityPostureBar->barDatas[erDataType].GetRatio();

                        drawBar(EPostureBarType::Entity, erDataType, viewportPosition, barSize, fillRatio);
                    }

                    if (entityPostureBar->drawStatusBars)
                    {
                        float statusHeight = EntityPostureBarData::statusBarHeight * ScreenParams::gameToViewportScaling;
                        float statusWidth = EntityPostureBarData::statusBarWidth * distanceModifier * ScreenParams::gameToViewportScaling;
                        ImVec2 statusBarSize = ImVec2(statusWidth, statusHeight);

                        viewportPosition.y += EntityPostureBarData::firstStatusBarDiffScreenY;
                        for (size_t i = static_cast<size_t>(EERDataType::STATUSES) + 1; i < static_cast<size_t>(EERDataType::MAX); ++i)
                        {
                            EERDataType statusEffectType = static_cast<EERDataType>(i);
                            if (entityPostureBar->drawBar[statusEffectType])
                            {
                                float fillRatio = entityPostureBar->barDatas[statusEffectType].GetRatio();
                                if (fillRatio < 0.01f)
                                    continue;

                                drawBar(EPostureBarType::Entity, statusEffectType, viewportPosition, statusBarSize, fillRatio);
                                viewportPosition.y += EntityPostureBarData::nextStatusBarDiffScreenY;
                            }
                        }
                    }
                }
            }
        }
    }

    ImColor PostureBarUI::getBarColor(EERDataType erDataType, float fillRatio)
    {
        const auto& [colorFrom, colorTo] = getMinMaxColor(erDataType);

        // so full stagger is "from", and empty is "to"
        fillRatio = 1.0f - fillRatio;
        int r = (int)std::lerp(colorFrom.x, colorTo.x, fillRatio);
        int g = (int)std::lerp(colorFrom.y, colorTo.y, fillRatio);
        int b = (int)std::lerp(colorFrom.z, colorTo.z, fillRatio);
        int a = (int)std::lerp(colorFrom.w, colorTo.w, fillRatio);

        return ImColor(r, g, b, a);
    }

    std::pair<ImVec4, ImVec4> PostureBarUI::getMinMaxColor(EERDataType erDataType)
    {
        switch (erDataType)
        {
            case EERDataType::Stagger:
                return { ER::BarStyle::staggerMinColor, ER::BarStyle::staggerMaxColor };
            case EERDataType::Stamina:
                return { ER::BarStyle::staminaMinColor, ER::BarStyle::staminaMaxColor };
            case EERDataType::Poison:
                return { ER::BarStyle::poisonMinColor, ER::BarStyle::poisonMaxColor };
            case EERDataType::Rot:
                return { ER::BarStyle::rotMinColor, ER::BarStyle::rotMaxColor };
            case EERDataType::Bleed:
                return { ER::BarStyle::bleedMinColor, ER::BarStyle::bleedMaxColor };
            case EERDataType::Blight:
                return { ER::BarStyle::blightMinColor, ER::BarStyle::blightMaxColor };
            case EERDataType::Frost:
                return { ER::BarStyle::frostMinColor, ER::BarStyle::frostMaxColor };
            case EERDataType::Sleep:
                return { ER::BarStyle::sleepMinColor, ER::BarStyle::sleepMaxColor };
            case EERDataType::Madness:
                return { ER::BarStyle::madnessMinColor, ER::BarStyle::madnessMaxColor };
            default:
                throw std::invalid_argument("PostureBarUI::getMinMaxColor - Not supported ER Data Type");
        }
    }

    void PostureBarUI::drawBar(const EPostureBarType postureBarType, const EERDataType erDataType, const ImVec2& position, const ImVec2& size, float fillRatio)
    {
        const ImColor& barColor = getBarColor(erDataType, fillRatio);

        if (textureBarInit)
        {
            const TextureBar& textureBar = postureBarType == EPostureBarType::Entity ? entityBarTexture : bossBarTexture;
            const FillTextureOffset& textureOffset = postureBarType == EPostureBarType::Entity ? TextureData::entityOffset : TextureData::bossOffset;
            drawBar(textureBar, barColor, position, size, textureOffset, fillRatio);
        }
        else
        {
            drawBar(barColor, position, size, fillRatio);
        }
    }

    void PostureBarUI::drawBar(const TextureBar& textureBar, const ImColor& color, const ImVec2& position, const ImVec2& size, const std::pair<ImVec2 /* top-left */, ImVec2 /* bot-right */>& fillOffset, float fillRatio)
    {
        auto&& [topLeftFillOffset, botRightFillOffset] = fillOffset;
        auto&& [barTextureBorder, barTextureFill] = textureBar;
        ImVec2 fillScale(size.x / barTextureFill.width, size.y / barTextureFill.height);
        ImVec2 borderScale(size.x / barTextureBorder.width, size.y / barTextureBorder.height);

        ImVec2 fillTopLeftScaled = (ImVec2(-barTextureFill.width * 0.5f, 0.0f) + topLeftFillOffset) * fillScale;
        ImVec2 fillBotRightScaled = (ImVec2(barTextureFill.width * 0.5f, barTextureFill.height) + botRightFillOffset) * fillScale;

        auto getFillPositions = [](float width, float ratio, EFillAlignment alignment, EFillType type) -> std::pair<ImVec2, ImVec2>
        {
            ratio = (type == EFillType::EmptyToFull) ? ratio : 1.0f - ratio;

            switch (alignment)
            {
                using enum EFillAlignment;
                case Left:
                    return { ImVec2(0.0f, 0.0f), ImVec2(-width * ratio, 0.0f) };
                case Center:
                    return { ImVec2(width * ratio * 0.5f, 0.0f), ImVec2(-width * ratio * 0.5f, 0.0f) };
                case Right:
                    return { ImVec2(width * ratio, 1.0f), ImVec2(0.0f, 0.0f) };
                default:
                    throw("EFillAlignment out of bounds index");
            }
        };

        auto&& [leftFill, rightFill] = getFillPositions(std::abs(fillTopLeftScaled.x - fillBotRightScaled.x), fillRatio, BarStyle::fillAlignment, BarStyle::fillType);

        // Add Fill texture with clip to stagger ratio
        if (BarStyle::fillResizeType == EFillResizeType::Clip)
        {
            ImGui::GetBackgroundDrawList()->PushClipRect(position + fillTopLeftScaled + leftFill, position + fillBotRightScaled + rightFill);
            ImGui::GetBackgroundDrawList()->AddImage(barTextureFill.texture, position + fillTopLeftScaled, position + fillBotRightScaled, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), color);
            ImGui::GetBackgroundDrawList()->PopClipRect();
        }
        else if (BarStyle::fillResizeType == EFillResizeType::Scale)
        {
            ImGui::GetBackgroundDrawList()->AddImage(barTextureFill.texture, position + fillTopLeftScaled + leftFill, position + fillBotRightScaled + rightFill, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), color);
        }

        // Add Border texture
        ImGui::GetBackgroundDrawList()->AddImage(barTextureBorder.texture, position - ImVec2(barTextureBorder.width * 0.5f, 0.0f) * borderScale, position + ImVec2(barTextureBorder.width * 0.5f, barTextureBorder.height) * borderScale);
    }

    void PostureBarUI::drawBar(const ImColor& color, const ImVec2& position, const ImVec2& size, float fillRatio)
    {
        ImGui::GetBackgroundDrawList()->AddRectFilled(position, position + size * ImVec2(fillRatio, 1.0f), color);
    }

    bool PostureBarUI::isMenuOpen()
    {
        if (!hideBarsOnMenu)
            return false;

        auto isLoad = RPM<bool>(g_Hooking->isLoading);
        auto menuOpen = (RPM<uint32_t>(g_Hooking->menuState1) != __UINT32_MAX__ || RPM<uint32_t>(g_Hooking->menuState2) != __UINT32_MAX__ || RPM<uint32_t>(g_Hooking->menuState3) != __UINT32_MAX__);

        return isLoad || menuOpen;
    }

    void PostureBarUI::updateUIBarStructs(uintptr_t moveMapStep, uintptr_t time)
    {
        updateUIBarStructsOriginal(moveMapStep, time);
#ifdef DEBUGLOG
        try
        {
#endif // DEBUGLOG
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

        if (PlayerPostureBarData::drawBar)
        {
            if (auto&& chrIns = g_Hooking->GetChrInsFromHandleFunc(worldChar, &(*worldChar->playerArray[0])->handle); chrIns && chrIns->chrModulelBag->staggerModule->staggerMax > 0.0f)
            {
                auto&& previousPlayerPostureBarData = g_postureUI->playerPostureBar;

                auto&& timePoint = std::chrono::steady_clock::now();

                PlayerPostureBarData playerPostureBarData;

                playerPostureBarData.maxStagger = chrIns->chrModulelBag->staggerModule->staggerMax;
                playerPostureBarData.stagger = chrIns->chrModulelBag->staggerModule->stagger;

                if (previousPlayerPostureBarData)
                {
                    // if previous value was below 0 and new value was reset
                    if (PlayerPostureBarData::resetStaggerTotalTime > 0.0f && previousPlayerPostureBarData->previousStagger <= 0.0f && playerPostureBarData.stagger == playerPostureBarData.maxStagger)
                    {
                        playerPostureBarData.resetStaggerTimer = PlayerPostureBarData::resetStaggerTotalTime;
                        playerPostureBarData.lastTimePoint = timePoint;
                        playerPostureBarData.isResetStagger = true;
                    }
                    else if (previousPlayerPostureBarData->isResetStagger)
                    {
                        playerPostureBarData.resetStaggerTimer = previousPlayerPostureBarData->resetStaggerTimer - std::chrono::duration_cast<std::chrono::duration<float>>(timePoint - previousPlayerPostureBarData->lastTimePoint).count();
                        playerPostureBarData.lastTimePoint = timePoint;
                        playerPostureBarData.isResetStagger = playerPostureBarData.resetStaggerTimer > 0.0f;
                    }

                    playerPostureBarData.previousStagger = playerPostureBarData.stagger;
                }

#ifdef DEBUGLOG
                playerPostureBarData.LogDebug();
#endif
                g_postureUI->playerPostureBar = playerPostureBarData;
            }
            else
            {
                g_postureUI->playerPostureBar = std::nullopt;
            }
        }

        for (IndexType i = 0; i < BOSS_CHR_ARRAY_LEN; i++)
            if (auto&& entityHandle = feMan->bossHpBars[i].bossHandle; entityHandle != __UINT64_MAX__)
            {
                auto&& chrIns = g_Hooking->GetChrInsFromHandleFunc(worldChar, &entityHandle);
                auto&& previousBossPostureBarData = g_postureUI->bossPostureBars[i];

                if (!chrIns || chrIns->chrModulelBag->staggerModule->staggerMax <= 0.0f)
                {
                    g_postureUI->bossPostureBars[i] = std::nullopt;
                    continue;
                }

                auto&& timePoint = std::chrono::steady_clock::now();

                BossPostureBarData bossPostureBarData;

                bossPostureBarData.entityHandle = entityHandle;
                bossPostureBarData.displayId = feMan->bossHpBars[i].displayId;
                bossPostureBarData.isStamina = BossPostureBarData::useStaminaForNPC && chrIns->modelNumber == 0;
                bossPostureBarData.isVisible = chrIns->chrModulelBag->statModule->health > 0;

                bossPostureBarData.barDatas[EERDataType::Stagger].SetValue(chrIns->chrModulelBag->staggerModule->stagger, chrIns->chrModulelBag->staggerModule->staggerMax);
                bossPostureBarData.barDatas[EERDataType::Stamina].SetValue(chrIns->chrModulelBag->statModule->stamina, chrIns->chrModulelBag->statModule->staminaMax);
                if (bossPostureBarData.drawStatusBars)
                {
                    if (bossPostureBarData.drawBar[EERDataType::Poison])
                        bossPostureBarData.barDatas[EERDataType::Poison].SetInverseValue(chrIns->chrModulelBag->resistanceModule->poison, chrIns->chrModulelBag->resistanceModule->poisonMax);
                    if (bossPostureBarData.drawBar[EERDataType::Rot])
                        bossPostureBarData.barDatas[EERDataType::Rot].SetInverseValue(chrIns->chrModulelBag->resistanceModule->rot, chrIns->chrModulelBag->resistanceModule->rotMax);
                    if (bossPostureBarData.drawBar[EERDataType::Bleed])
                        bossPostureBarData.barDatas[EERDataType::Bleed].SetInverseValue(chrIns->chrModulelBag->resistanceModule->bleed, chrIns->chrModulelBag->resistanceModule->bleedMax);
                    if (bossPostureBarData.drawBar[EERDataType::Blight])
                        bossPostureBarData.barDatas[EERDataType::Blight].SetInverseValue(chrIns->chrModulelBag->resistanceModule->blight, chrIns->chrModulelBag->resistanceModule->blightMax);
                    if (bossPostureBarData.drawBar[EERDataType::Frost])
                        bossPostureBarData.barDatas[EERDataType::Frost].SetInverseValue(chrIns->chrModulelBag->resistanceModule->frost, chrIns->chrModulelBag->resistanceModule->frostMax);
                    if (bossPostureBarData.drawBar[EERDataType::Sleep])
                        bossPostureBarData.barDatas[EERDataType::Sleep].SetInverseValue(chrIns->chrModulelBag->resistanceModule->sleep, chrIns->chrModulelBag->resistanceModule->sleepMax);
                    if (bossPostureBarData.drawBar[EERDataType::Madness])
                        bossPostureBarData.barDatas[EERDataType::Madness].SetInverseValue(chrIns->chrModulelBag->resistanceModule->madness, chrIns->chrModulelBag->resistanceModule->madnessMax);
                }

                if (previousBossPostureBarData && previousBossPostureBarData->entityHandle == entityHandle)
                {
                    // if previous value was below 0 and new value was reset
                    if (BossPostureBarData::resetStaggerTotalTime > 0.0f && previousBossPostureBarData->previousStagger <= 0.0f && bossPostureBarData.barDatas[EERDataType::Stagger].IsValueMax())
                    {
                        bossPostureBarData.resetStaggerTimer = BossPostureBarData::resetStaggerTotalTime;
                        bossPostureBarData.lastTimePoint = timePoint;
                        bossPostureBarData.isResetStagger = true;
                    }
                    else if (previousBossPostureBarData->isResetStagger)
                    {
                        bossPostureBarData.resetStaggerTimer = previousBossPostureBarData->resetStaggerTimer - std::chrono::duration_cast<std::chrono::duration<float>>(timePoint - previousBossPostureBarData->lastTimePoint).count();
                        bossPostureBarData.lastTimePoint = timePoint;
                        bossPostureBarData.isResetStagger = bossPostureBarData.resetStaggerTimer > 0.0f;
                    }

                    bossPostureBarData.previousStagger = bossPostureBarData.barDatas[EERDataType::Stagger].value;
                }

#ifdef DEBUGLOG
                bossPostureBarData.LogDebug();
#endif
                g_postureUI->bossPostureBars[i] = bossPostureBarData;
            }
            else
            {
                g_postureUI->bossPostureBars[i] = std::nullopt;
            }


        for (IndexType i = 0; i < ENTITY_CHR_ARRAY_LEN; i++)
            if (auto&& entityHandle = feMan->entityHpBars[i].entityHandle; entityHandle != __UINT64_MAX__)
            {
                auto&& chrIns = g_Hooking->GetChrInsFromHandleFunc(worldChar, &entityHandle);
                auto&& previousEntityPostureBarData = g_postureUI->entityPostureBars[i];

                if (!chrIns || chrIns->chrModulelBag->staggerModule->staggerMax <= 0.0f)
                {
                    g_postureUI->entityPostureBars[i] = std::nullopt;
                    continue;
                }

                auto&& timePoint = std::chrono::steady_clock::now();

                EntityPostureBarData entityPostureBarData;

                entityPostureBarData.entityHandle = entityHandle;
                entityPostureBarData.isStamina = EntityPostureBarData::useStaminaForNPC && chrIns->modelNumber == 0;
                entityPostureBarData.screenX = feMan->entityHpBars[i].screenPosX;
                entityPostureBarData.screenY = feMan->entityHpBars[i].screenPosY;
                entityPostureBarData.distanceModifier = feMan->entityHpBars[i].mod;
                entityPostureBarData.isVisible = feMan->entityHpBars[i].isVisible && chrIns->chrModulelBag->statModule->health > 0;

                entityPostureBarData.barDatas[EERDataType::Stagger].SetValue(chrIns->chrModulelBag->staggerModule->stagger, chrIns->chrModulelBag->staggerModule->staggerMax);
                entityPostureBarData.barDatas[EERDataType::Stamina].SetValue(chrIns->chrModulelBag->statModule->stamina, chrIns->chrModulelBag->statModule->staminaMax);
                if (entityPostureBarData.drawStatusBars)
                {
                    if (entityPostureBarData.drawBar[EERDataType::Poison])
                        entityPostureBarData.barDatas[EERDataType::Poison].SetInverseValue(chrIns->chrModulelBag->resistanceModule->poison, chrIns->chrModulelBag->resistanceModule->poisonMax);
                    if (entityPostureBarData.drawBar[EERDataType::Rot])
                        entityPostureBarData.barDatas[EERDataType::Rot].SetInverseValue(chrIns->chrModulelBag->resistanceModule->rot, chrIns->chrModulelBag->resistanceModule->rotMax);
                    if (entityPostureBarData.drawBar[EERDataType::Bleed])
                        entityPostureBarData.barDatas[EERDataType::Bleed].SetInverseValue(chrIns->chrModulelBag->resistanceModule->bleed, chrIns->chrModulelBag->resistanceModule->bleedMax);
                    if (entityPostureBarData.drawBar[EERDataType::Blight])
                        entityPostureBarData.barDatas[EERDataType::Blight].SetInverseValue(chrIns->chrModulelBag->resistanceModule->blight, chrIns->chrModulelBag->resistanceModule->blightMax);
                    if (entityPostureBarData.drawBar[EERDataType::Frost])
                        entityPostureBarData.barDatas[EERDataType::Frost].SetInverseValue(chrIns->chrModulelBag->resistanceModule->frost, chrIns->chrModulelBag->resistanceModule->frostMax);
                    if (entityPostureBarData.drawBar[EERDataType::Sleep])
                        entityPostureBarData.barDatas[EERDataType::Sleep].SetInverseValue(chrIns->chrModulelBag->resistanceModule->sleep, chrIns->chrModulelBag->resistanceModule->sleepMax);
                    if (entityPostureBarData.drawBar[EERDataType::Madness])
                        entityPostureBarData.barDatas[EERDataType::Madness].SetInverseValue(chrIns->chrModulelBag->resistanceModule->madness, chrIns->chrModulelBag->resistanceModule->madnessMax);
                }

                entityPostureBarData.gameUiUpdateTimePoint = timePoint;
                entityPostureBarData.gamePreviousUiUpdateTimePoint = timePoint;
                entityPostureBarData.previousScreenX = entityPostureBarData.screenX;
                entityPostureBarData.previousScreenY = entityPostureBarData.screenY;

                if (previousEntityPostureBarData && previousEntityPostureBarData->entityHandle == entityHandle)
                {
                    // if previous value was below 0 and new value was reset
                    if (EntityPostureBarData::resetStaggerTotalTime > 0.0f && previousEntityPostureBarData->previousStagger <= 0.0f && entityPostureBarData.barDatas[EERDataType::Stagger].IsValueMax())
                    {
                        entityPostureBarData.resetStaggerTimer = EntityPostureBarData::resetStaggerTotalTime;
                        entityPostureBarData.lastTimePoint = timePoint;
                        entityPostureBarData.isResetStagger = true;
                    }
                    else if (previousEntityPostureBarData->isResetStagger)
                    {
                        entityPostureBarData.resetStaggerTimer = previousEntityPostureBarData->resetStaggerTimer - std::chrono::duration_cast<std::chrono::duration<float>>(timePoint - previousEntityPostureBarData->lastTimePoint).count();
                        entityPostureBarData.lastTimePoint = timePoint;
                        entityPostureBarData.isResetStagger = entityPostureBarData.resetStaggerTimer > 0.0f;
                    }

                    entityPostureBarData.gamePreviousUiUpdateTimePoint = previousEntityPostureBarData->gameUiUpdateTimePoint;
                    entityPostureBarData.previousScreenX = previousEntityPostureBarData->screenX;
                    entityPostureBarData.previousScreenY = previousEntityPostureBarData->screenY;
                    entityPostureBarData.previousStagger = entityPostureBarData.barDatas[EERDataType::Stagger].value;
                }

#ifdef DEBUGLOG
                entityPostureBarData.LogDebug();
#endif
                g_postureUI->entityPostureBars[i] = entityPostureBarData;
            }
            else
            {
                g_postureUI->entityPostureBars[i] = std::nullopt;
            }

#ifdef DEBUGLOG
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
#endif // DEBUGLOG
    }

#ifdef DEBUGLOG
    void PlayerPostureBarData::LogDebug()
    {
        Logger::log("---------------------------------------------------------------------------------");
        Logger::log("PlayerPostureBarData: ");
        Logger::log("\tmaxStagger: " + std::to_string(maxStagger));
        Logger::log("\tstagger: " + std::to_string(stagger));
        Logger::log("\tpreviousStagger: " + std::to_string(previousStagger));
        Logger::log("\tisResetStagger: " + std::to_string(isResetStagger));
        Logger::log("\tresetStaggerTimer: " + std::to_string(resetStaggerTimer));
        Logger::log("\tlastTimePoint: " + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(lastTimePoint.time_since_epoch()).count()));
        Logger::log("---------------------------------------------------------------------------------");
    }

    void BossPostureBarData::LogDebug()
    {
        Logger::log("---------------------------------------------------------------------------------");
        Logger::log("PlayerPostureBarData: ");
        Logger::log("\tentityHandle: " + std::to_string(entityHandle));
        Logger::log("\tdisplayId: " + std::to_string(displayId));
        Logger::log("\tisStamina: " + std::to_string(isStamina));
        Logger::log("\tisVisible: " + std::to_string(isVisible));
        Logger::log("\tbarDatas: ");
        for (std::pair<EERDataType, BarData> barEntry : barDatas)
        {
            Logger::log("\t\t" + to_string(barEntry.first) + ":");
            Logger::log("\t\tvalue:" + std::to_string(barEntry.second.value));
            Logger::log("\t\tmaxValue:" + std::to_string(barEntry.second.maxValue));
        }
        Logger::log("\tpreviousStagger: " + std::to_string(previousStagger));
        Logger::log("\tisResetStagger: " + std::to_string(isResetStagger));
        Logger::log("\tresetStaggerTimer: " + std::to_string(resetStaggerTimer));
        Logger::log("\tlastTimePoint: " + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(lastTimePoint.time_since_epoch()).count()));
        Logger::log("---------------------------------------------------------------------------------");
    }

    void EntityPostureBarData::LogDebug()
    {
        Logger::log("---------------------------------------------------------------------------------");
        Logger::log("PlayerPostureBarData: ");
        Logger::log("\tentityHandle: " + std::to_string(entityHandle));
        Logger::log("\tisStamina: " + std::to_string(isStamina));
        Logger::log("\tisVisible: " + std::to_string(isVisible));
        Logger::log("\tscreenX: " + std::to_string(screenX));
        Logger::log("\tscreenY: " + std::to_string(screenY));
        Logger::log("\tdistanceModifier: " + std::to_string(distanceModifier));
        Logger::log("\tpreviousScreenX: " + std::to_string(previousScreenX));
        Logger::log("\tpreviousScreenY: " + std::to_string(previousScreenY));
        Logger::log("\tgameUiUpdateTimePoint: " + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(gameUiUpdateTimePoint.time_since_epoch()).count()));
        Logger::log("\tgamePreviousUiUpdateTimePoint: " + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(gamePreviousUiUpdateTimePoint.time_since_epoch()).count()));
        Logger::log("\tbarDatas: ");
        for (std::pair<EERDataType, BarData> barEntry : barDatas)
        {
            Logger::log("\t\t" + to_string(barEntry.first) + ":");
            Logger::log("\t\tvalue:" + std::to_string(barEntry.second.value));
            Logger::log("\t\tmaxValue:" + std::to_string(barEntry.second.maxValue));
        }
        Logger::log("\tpreviousStagger: " + std::to_string(previousStagger));
        Logger::log("\tisResetStagger: " + std::to_string(isResetStagger));
        Logger::log("\tresetStaggerTimer: " + std::to_string(resetStaggerTimer));
        Logger::log("\tlastTimePoint: " + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(lastTimePoint.time_since_epoch()).count()));
        Logger::log("---------------------------------------------------------------------------------");
    }
#endif // DEBUGLOG
}