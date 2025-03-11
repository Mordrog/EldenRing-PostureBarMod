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
                    return std::tuple{ "Boss bar screen offset", BossPostureBarData::firstBossScreenX, BossPostureBarData::firstBossScreenY };
                case EntityBarOffset:
                    return std::tuple{ "Entity bar screen offset", PostureBarData::offsetScreenX, PostureBarData::offsetScreenY };
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
        
        if (PlayerPostureBarData::drawBar)
        {
            if (auto playerPostureBar = playerBar; playerPostureBar)
            {
                auto&& timePoint = std::chrono::steady_clock::now();

                float staggerRatio = playerPostureBar->stagger / playerPostureBar->maxStagger;

                float height = PlayerPostureBarData::barHeight * barHeightMultiplier * ScreenParams::gameToViewportScaling;
                float width = PlayerPostureBarData::barWidth * ScreenParams::gameToViewportScaling;
                ImVec2 positionOnViewport = ImVec2(PlayerPostureBarData::screenX, PlayerPostureBarData::screenY) * ScreenParams::gameToViewportScaling;

                // apply screen position offset
                positionOnViewport.x += ScreenParams::posX;
                positionOnViewport.y += ScreenParams::posY;

                if (playerPostureBar->isResetStagger)
                {
                    playerPostureBar->resetStaggerTimer -= std::chrono::duration_cast<std::chrono::duration<float>>(timePoint - playerPostureBar->lastTimePoint).count();
                    playerPostureBar->lastTimePoint = timePoint;
                 
                    if (playerPostureBar->resetStaggerTimer <= 0.0f)
                        playerPostureBar->isResetStagger = false;
                    else
                    {
                        float timeRatio = 1.0f - playerPostureBar->resetStaggerTimer / PlayerPostureBarData::resetStaggerTotalTime;

                        auto barColor = getBarColor(false, staggerRatio);

                        if (textureBarInit)
                            drawBar(entityBarTexture, barColor, positionOnViewport, ImVec2(width, height), TextureData::entityOffset, timeRatio);
                        else
                            drawBar(barColor, positionOnViewport, ImVec2(width, height), timeRatio);
                    }
                }
                else
                {
                    auto barColor = getBarColor(false, staggerRatio);

                    if (textureBarInit)
                        drawBar(entityBarTexture, barColor, positionOnViewport, ImVec2(width, height), TextureData::entityOffset, staggerRatio);
                    else
                        drawBar(barColor, positionOnViewport, ImVec2(width, height), staggerRatio);
                }
            }
        }

        if (BossPostureBarData::drawBars)
            for (IndexType i = 0; i < BOSS_CHR_ARRAY_LEN; i++)
                if (auto bossPostureBar = bossPostureBars[i]; bossPostureBar && bossPostureBar->isVisible)
                {
                    auto&& timePoint = std::chrono::steady_clock::now();

					float staggerRatio = bossPostureBar->stagger / bossPostureBar->maxStagger;

                    float poisonRatio = bossPostureBar->poison / bossPostureBar->maxPoison;
                    float rotRatio = bossPostureBar->rot / bossPostureBar->maxRot;
                    float bleedRatio = bossPostureBar->bleed / bossPostureBar->maxBleed;
                    float blightRatio = bossPostureBar->blight / bossPostureBar->maxBlight;
                    float frostRatio = bossPostureBar->frost / bossPostureBar->maxFrost;
                    float sleepRatio = bossPostureBar->sleep / bossPostureBar->maxSleep;
                    float madnessRatio = bossPostureBar->madness / bossPostureBar->maxMadness;

                    float height = BossPostureBarData::barHeight * barHeightMultiplier * ScreenParams::gameToViewportScaling;
                    float width = BossPostureBarData::barWidth * ScreenParams::gameToViewportScaling;
                    ImVec2 viewportPosition = ImVec2(BossPostureBarData::firstBossScreenX, BossPostureBarData::firstBossScreenY) * ScreenParams::gameToViewportScaling;

                    // apply screen position offset
                    viewportPosition.x += ScreenParams::posX;
                    viewportPosition.y += ScreenParams::posY;

                    // apply offset if bar is for second and third boss
                    viewportPosition.y -= ((float)i * BossPostureBarData::nextBossBarDiffScreenY) * ScreenParams::gameToViewportScaling;

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
                            {
                                drawBar(bossBarTexture, barColor, viewportPosition, ImVec2(width, height), TextureData::bossOffset, timeRatio);
                                viewportPosition.y += height;
                            }
                            else
                            {
                                drawBar(barColor, viewportPosition, ImVec2(width, height), timeRatio);
                                viewportPosition.y += height;
                            }
                        }
                    }

                    auto staggerColor = getBarColor(bossPostureBar->isStamina, staggerRatio);

                    auto poisonColor = mixBarColor(BarStyle::poisonMinColor, BarStyle::poisonMaxColor, poisonRatio);
                    auto rotColor = mixBarColor(BarStyle::rotMinColor, BarStyle::rotMaxColor, rotRatio);
                    auto bleedColor = mixBarColor(BarStyle::bleedMinColor, BarStyle::bleedMaxColor, bleedRatio);
                    auto blightColor = mixBarColor(BarStyle::blightMinColor, BarStyle::blightMaxColor, blightRatio);
                    auto frostColor = mixBarColor(BarStyle::frostMinColor, BarStyle::frostMaxColor, frostRatio);
                    auto sleepColor = mixBarColor(BarStyle::sleepMinColor, BarStyle::sleepMaxColor, sleepRatio);
                    auto madnessColor = mixBarColor(BarStyle::madnessMinColor, BarStyle::madnessMaxColor, madnessRatio);

                    if (textureBarInit)
                    {
                        if (bossPostureBar->drawPoiseBar && !bossPostureBar->isResetStagger) // Poise bar
                        {
							drawBar(bossBarTexture, staggerColor, viewportPosition, ImVec2(width, height), TextureData::bossOffset, staggerRatio);
							viewportPosition.y += height;
                        }
                        if (bossPostureBar->drawPoisonBar) // Poison bar
                        {
							drawBar(bossBarTexture, poisonColor, viewportPosition, ImVec2(width, height), TextureData::bossOffset, poisonRatio);
							viewportPosition.y += height;
                        }
                        if (bossPostureBar->drawRotBar) // Rot bar
                        {
							drawBar(bossBarTexture, rotColor, viewportPosition, ImVec2(width, height), TextureData::bossOffset, rotRatio);
							viewportPosition.y += height;
                        }
                        if (bossPostureBar->drawBleedBar) // Bleed bar
                        {
							drawBar(bossBarTexture, bleedColor, viewportPosition, ImVec2(width, height), TextureData::bossOffset, bleedRatio);
							viewportPosition.y += height;
                        }
						if (bossPostureBar->drawBlightBar) // Blight bar
                        {
							drawBar(bossBarTexture, blightColor, viewportPosition, ImVec2(width, height), TextureData::bossOffset, blightRatio);
							viewportPosition.y += height;
                        }
						if (bossPostureBar->drawFrostBar) // Frost bar
                        {
							drawBar(bossBarTexture, frostColor, viewportPosition, ImVec2(width, height), TextureData::bossOffset, frostRatio);
							viewportPosition.y += height;
                        }
						if (bossPostureBar->drawSleepBar) // Sleep bar
                        {
							drawBar(bossBarTexture, sleepColor, viewportPosition, ImVec2(width, height), TextureData::bossOffset, sleepRatio);
							viewportPosition.y += height;
						}
						if (bossPostureBar->drawMadnessBar) // Madness bar
                        {
							drawBar(bossBarTexture, madnessColor, viewportPosition, ImVec2(width, height), TextureData::bossOffset, madnessRatio);
                        }
                    }
                    else {
                        if (bossPostureBar->drawPoiseBar && !bossPostureBar->isResetStagger) // Poise bar
                        {
							drawBar(staggerColor, viewportPosition, ImVec2(width, height), staggerRatio);
							viewportPosition.y += height;
                        }
						if (bossPostureBar->drawPoisonBar) // Poison bar
                        {
							drawBar(poisonColor, viewportPosition, ImVec2(width, height), poisonRatio);
                            viewportPosition.y += height;
                        }
                        if (bossPostureBar->drawRotBar) // Rot bar
                        {
                            drawBar(rotColor, viewportPosition, ImVec2(width, height), rotRatio);
                            viewportPosition.y += height;
                        }
						if (bossPostureBar->drawBleedBar) // Bleed bar
                        {
							drawBar(bleedColor, viewportPosition, ImVec2(width, height), bleedRatio);
							viewportPosition.y += height;
                        }
						if (bossPostureBar->drawBlightBar) // Blight bar
                        {
							drawBar(blightColor, viewportPosition, ImVec2(width, height), blightRatio);
							viewportPosition.y += height;
                        }
						if (bossPostureBar->drawFrostBar) // Frost bar
                        {
							drawBar(frostColor, viewportPosition, ImVec2(width, height), frostRatio);
							viewportPosition.y += height;
                        }
						if (bossPostureBar->drawSleepBar) // Sleep bar
                        {
							drawBar(sleepColor, viewportPosition, ImVec2(width, height), sleepRatio);
							viewportPosition.y += height;
                        }
						if (bossPostureBar->drawMadnessBar) // Madness bar
                        {
							drawBar(madnessColor, viewportPosition, ImVec2(width, height), madnessRatio);
                        }
                    }
                }

        if (PostureBarData::drawBars)
            for (IndexType i = 0; i < ENTITY_CHR_ARRAY_LEN; i++)
                if (auto postureBar = postureBars[i]; postureBar && postureBar->isVisible)
                {
                    auto&& timePoint = std::chrono::steady_clock::now();

                    float staggerRatio = postureBar->stagger / postureBar->maxStagger;

					float poisonRatio = postureBar->poison / postureBar->maxPoison;
					float rotRatio = postureBar->rot / postureBar->maxRot;
					float bleedRatio = postureBar->bleed / postureBar->maxBleed;
					float blightRatio = postureBar->blight / postureBar->maxBlight;
					float frostRatio = postureBar->frost / postureBar->maxFrost;
					float sleepRatio = postureBar->sleep / postureBar->maxSleep;
					float madnessRatio = postureBar->madness / postureBar->maxMadness;

                    float distanceModifier = postureBar->distanceModifier;

                    ImVec2 gamePosition(postureBar->screenX, postureBar->screenY);

                    // apply fix offset from predicting previous movement
                    if (PostureBarData::usePositionFixing)
                        gamePosition -= positionFixOffset(*postureBar, timePoint);

                    // apply threshold to in game position
                    gamePosition.x = std::clamp(gamePosition.x, PostureBarData::leftScreenThreshold, PostureBarData::rightScreenThreshold);
                    gamePosition.y = std::clamp(gamePosition.y, PostureBarData::topScreenThreshold, PostureBarData::bottomScreenThreshold);

                    // transform in game position into viewport position
                    float height = PostureBarData::barHeight * barHeightMultiplier * ScreenParams::gameToViewportScaling;
                    float width = PostureBarData::barWidth * distanceModifier * ScreenParams::gameToViewportScaling;
                    ImVec2 viewportPosition = gamePosition * ScreenParams::gameToViewportScaling;

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
                            {
                                drawBar(entityBarTexture, barColor, viewportPosition, ImVec2(width, height), TextureData::entityOffset, timeRatio);
                                viewportPosition.y += height;
                            }
                            else
                            {
                                drawBar(barColor, viewportPosition, ImVec2(width, height), timeRatio);
                                viewportPosition.y += height;
                            }
                        }
                    }

                    auto staggerColor = getBarColor(postureBar->isStamina, staggerRatio);

					auto poisonColor = mixBarColor(BarStyle::poisonMinColor, BarStyle::poisonMaxColor, poisonRatio);
					auto rotColor = mixBarColor(BarStyle::rotMinColor, BarStyle::rotMaxColor, rotRatio);
					auto bleedColor = mixBarColor(BarStyle::bleedMinColor, BarStyle::bleedMaxColor, bleedRatio);
					auto blightColor = mixBarColor(BarStyle::blightMinColor, BarStyle::blightMaxColor, blightRatio);
					auto frostColor = mixBarColor(BarStyle::frostMinColor, BarStyle::frostMaxColor, frostRatio);
					auto sleepColor = mixBarColor(BarStyle::sleepMinColor, BarStyle::sleepMaxColor, sleepRatio);
					auto madnessColor = mixBarColor(BarStyle::madnessMinColor, BarStyle::madnessMaxColor, madnessRatio);

                    if (textureBarInit) {
                        if (postureBar->drawPoiseBar && !postureBar->isResetStagger) // Poise bar
                        {
							drawBar(entityBarTexture, staggerColor, viewportPosition, ImVec2(width, height), TextureData::entityOffset, staggerRatio);
							viewportPosition.y += height;
                        }
						if (postureBar->drawPoisonBar) // Poison bar
                        {
							drawBar(entityBarTexture, poisonColor, viewportPosition, ImVec2(width, height), TextureData::entityOffset, poisonRatio);
							viewportPosition.y += height;
						}
						if (postureBar->drawRotBar) // Rot bar
                        {
							drawBar(entityBarTexture, rotColor, viewportPosition, ImVec2(width, height), TextureData::entityOffset, rotRatio);
							viewportPosition.y += height;
                        }
						if (postureBar->drawBleedBar) // Bleed bar
                        {
							drawBar(entityBarTexture, bleedColor, viewportPosition, ImVec2(width, height), TextureData::entityOffset, bleedRatio);
							viewportPosition.y += height;
                        }
						if (postureBar->drawBlightBar) // Blight bar
                        {
							drawBar(entityBarTexture, blightColor, viewportPosition, ImVec2(width, height), TextureData::entityOffset, blightRatio);
							viewportPosition.y += height;
                        }
                        if (postureBar->drawFrostBar) // Frost bar
                        {
							drawBar(entityBarTexture, frostColor, viewportPosition, ImVec2(width, height), TextureData::entityOffset, frostRatio);
							viewportPosition.y += height;
                        }
						if (postureBar->drawSleepBar) // Sleep bar
                        {
							drawBar(entityBarTexture, sleepColor, viewportPosition, ImVec2(width, height), TextureData::entityOffset, sleepRatio);
							viewportPosition.y += height;
                        }
						if (postureBar->drawMadnessBar) // Madness bar
                        {
							drawBar(entityBarTexture, madnessColor, viewportPosition, ImVec2(width, height), TextureData::entityOffset, madnessRatio);
							viewportPosition.y += height;
                        }
                    }
                    else
                    {
                        if (postureBar->drawPoiseBar && !postureBar->isResetStagger) // Poise bar
                        {
							drawBar(staggerColor, viewportPosition, ImVec2(width, height), staggerRatio);
							viewportPosition.y += height;
                        }
                        if (postureBar->drawPoisonBar) // Poison bar
                        {
							drawBar(poisonColor, viewportPosition, ImVec2(width, height), poisonRatio);
							viewportPosition.y += height;
                        }
                        if (postureBar->drawRotBar) // Rot bar
                        {
							drawBar(rotColor, viewportPosition, ImVec2(width, height), rotRatio);
							viewportPosition.y += height;
                        }
						if (postureBar->drawBleedBar) // Bleed bar
                        {
							drawBar(bleedColor, viewportPosition, ImVec2(width, height), bleedRatio);
							viewportPosition.y += height;
                        }
						if (postureBar->drawBlightBar) // Blight bar
                        {
							drawBar(blightColor, viewportPosition, ImVec2(width, height), blightRatio);
							viewportPosition.y += height;
                        }
						if (postureBar->drawFrostBar) // Frost bar
                        {
							drawBar(frostColor, viewportPosition, ImVec2(width, height), frostRatio);
							viewportPosition.y += height;
                        }
						if (postureBar->drawSleepBar) // Sleep bar
                        {
							drawBar(sleepColor, viewportPosition, ImVec2(width, height), sleepRatio);
							viewportPosition.y += height;
                        }
						if (postureBar->drawMadnessBar) // Madness bar
                        {
							drawBar(madnessColor, viewportPosition, ImVec2(width, height), madnessRatio);
							viewportPosition.y += height;
                        }
                    }
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

    ImColor PostureBarUI::mixBarColor(ImVec4& minColor, ImVec4& maxColor, float fillRatio) {
        fillRatio = 1.0f - fillRatio;
        int r = (int)std::lerp(minColor.x, maxColor.x, fillRatio);
        int g = (int)std::lerp(minColor.y, maxColor.y, fillRatio);
        int b = (int)std::lerp(minColor.z, maxColor.z, fillRatio);
        int a = (int)std::lerp(minColor.w, maxColor.w, fillRatio);

        return ImColor(r, g, b, a);
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
                auto&& timePoint = std::chrono::steady_clock::now();
                auto&& previousPlayerStaggerBar = g_postureUI->playerBar;
                PlayerPostureBarData playerBarData;

                playerBarData.maxStagger = chrIns->chrModulelBag->staggerModule->staggerMax;
                playerBarData.stagger = chrIns->chrModulelBag->staggerModule->stagger;

                if (previousPlayerStaggerBar)
                {
                    if (PlayerPostureBarData::resetStaggerTotalTime > 0.0f && previousPlayerStaggerBar->previousStagger <= 0.0f && playerBarData.stagger == playerBarData.maxStagger)
                    {
                        playerBarData.isResetStagger = true;
                        playerBarData.resetStaggerTimer = PlayerPostureBarData::resetStaggerTotalTime;
                        playerBarData.lastTimePoint = timePoint;
                    }
                    else
                    {
                        playerBarData.isResetStagger = previousPlayerStaggerBar->isResetStagger;
                        playerBarData.resetStaggerTimer = previousPlayerStaggerBar->resetStaggerTimer;
                        playerBarData.lastTimePoint = previousPlayerStaggerBar->lastTimePoint;
                    }

                    playerBarData.previousStagger = playerBarData.stagger;
                }

                g_postureUI->playerBar = playerBarData;
            }
            else
                g_postureUI->playerBar = std::nullopt;
        }

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

                BossPostureBarData bossStaggerBarData;

                bossStaggerBarData.entityHandle = entityHandle;
                bossStaggerBarData.displayId = feMan->bossHpBars[i].displayId;
                bossStaggerBarData.isStamina = BossPostureBarData::useStaminaForNPC && chrIns->modelNumber == 0;
                bossStaggerBarData.maxStagger = !bossStaggerBarData.isStamina ? chrIns->chrModulelBag->staggerModule->staggerMax : chrIns->chrModulelBag->statModule->staminaMax;
                bossStaggerBarData.stagger = !bossStaggerBarData.isStamina ? chrIns->chrModulelBag->staggerModule->stagger : chrIns->chrModulelBag->statModule->stamina;
                bossStaggerBarData.isVisible = chrIns->chrModulelBag->statModule->health > 0;

				bossStaggerBarData.maxPoison = static_cast<float>(chrIns->chrModulelBag->resistanceModule->poisonMax);
				bossStaggerBarData.poison = bossStaggerBarData.maxPoison - static_cast<float>(chrIns->chrModulelBag->resistanceModule->poison);
				bossStaggerBarData.maxRot = static_cast<float>(chrIns->chrModulelBag->resistanceModule->rotMax);
				bossStaggerBarData.rot = bossStaggerBarData.maxRot - static_cast<float>(chrIns->chrModulelBag->resistanceModule->rot);
				bossStaggerBarData.maxBleed = static_cast<float>(chrIns->chrModulelBag->resistanceModule->bleedMax);
				bossStaggerBarData.bleed = bossStaggerBarData.maxBleed - static_cast<float>(chrIns->chrModulelBag->resistanceModule->bleed);
				bossStaggerBarData.maxBlight = static_cast<float>(chrIns->chrModulelBag->resistanceModule->blightMax);
                bossStaggerBarData.blight = bossStaggerBarData.maxBlight - static_cast<float>(chrIns->chrModulelBag->resistanceModule->blight);
                bossStaggerBarData.maxFrost = static_cast<float>(chrIns->chrModulelBag->resistanceModule->frostMax);
                bossStaggerBarData.frost = bossStaggerBarData.maxFrost - static_cast<float>(chrIns->chrModulelBag->resistanceModule->frost);
				bossStaggerBarData.maxSleep = static_cast<float>(chrIns->chrModulelBag->resistanceModule->sleepMax);
				bossStaggerBarData.sleep = bossStaggerBarData.maxSleep - static_cast<float>(chrIns->chrModulelBag->resistanceModule->sleep);
				bossStaggerBarData.maxMadness = static_cast<float>(chrIns->chrModulelBag->resistanceModule->madnessMax);
				bossStaggerBarData.madness = bossStaggerBarData.maxMadness - static_cast<float>(chrIns->chrModulelBag->resistanceModule->madness);

                if (previousBossStaggerBar && previousBossStaggerBar->entityHandle == entityHandle)
                {
                    if (!bossStaggerBarData.isStamina && BossPostureBarData::resetStaggerTotalTime > 0.0f && previousBossStaggerBar->previousStagger <= 0.0f && bossStaggerBarData.stagger == bossStaggerBarData.maxStagger)
                    {
                        bossStaggerBarData.isResetStagger = true;
                        bossStaggerBarData.resetStaggerTimer = BossPostureBarData::resetStaggerTotalTime;
                        bossStaggerBarData.lastTimePoint = timePoint;
                    }
                    else
                    {
                        bossStaggerBarData.isResetStagger = previousBossStaggerBar->isResetStagger;
                        bossStaggerBarData.resetStaggerTimer = previousBossStaggerBar->resetStaggerTimer;
                        bossStaggerBarData.lastTimePoint = previousBossStaggerBar->lastTimePoint;
                    }

                    bossStaggerBarData.previousStagger = bossStaggerBarData.stagger;
                }

                g_postureUI->bossPostureBars[i] = bossStaggerBarData;
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

				staggerBarData.maxPoison = static_cast<float>(chrIns->chrModulelBag->resistanceModule->poisonMax);
				staggerBarData.poison = staggerBarData.maxPoison - static_cast<float>(chrIns->chrModulelBag->resistanceModule->poison);
				staggerBarData.maxRot = static_cast<float>(chrIns->chrModulelBag->resistanceModule->rotMax);
				staggerBarData.rot = staggerBarData.maxRot - static_cast<float>(chrIns->chrModulelBag->resistanceModule->rot);
				staggerBarData.maxBleed = static_cast<float>(chrIns->chrModulelBag->resistanceModule->bleedMax);
				staggerBarData.bleed = staggerBarData.maxBleed - static_cast<float>(chrIns->chrModulelBag->resistanceModule->bleed);
				staggerBarData.maxBlight = static_cast<float>(chrIns->chrModulelBag->resistanceModule->blightMax);
				staggerBarData.blight = staggerBarData.maxBlight - static_cast<float>(chrIns->chrModulelBag->resistanceModule->blight);
                staggerBarData.maxFrost = static_cast<float>(chrIns->chrModulelBag->resistanceModule->frostMax);
                staggerBarData.frost = staggerBarData.maxFrost - static_cast<float>(chrIns->chrModulelBag->resistanceModule->frost);
				staggerBarData.maxSleep = static_cast<float>(chrIns->chrModulelBag->resistanceModule->sleepMax);
				staggerBarData.sleep = staggerBarData.maxSleep - static_cast<float>(chrIns->chrModulelBag->resistanceModule->sleep);
				staggerBarData.maxMadness = static_cast<float>(chrIns->chrModulelBag->resistanceModule->madnessMax);
				staggerBarData.madness = staggerBarData.maxMadness - static_cast<float>(chrIns->chrModulelBag->resistanceModule->madness);

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
}