#include "PostureBarMod.hpp"
#include "Ini/ini.h"
#include "Main/Logger.hpp"
#include "Main/D3DRenderer.hpp"
#include "Main/Hooking.hpp"
#include "Main/PostureBarUI.hpp"

using namespace ER;

bool loadIni()
{
    try    
    {
        using namespace mINI;
        INIFile config(dllPath + "PostureBarModConfig.ini");
        INIStructure ini;

        if (!config.read(ini))
            throw std::exception("Failed to read PostureModConfig.ini in mods folder");

        //-----------------------------------------------------------------------------------
        //									    General
        //-----------------------------------------------------------------------------------
        ScreenParams::inGameCoordSizeX = std::stof(ini["General"].get("InGameScreenCoordWidth"));
        ScreenParams::inGameCoordSizeY = std::stof(ini["General"].get("InGameScreenCoordHeight"));
        ScreenParams::posX = std::stof(ini["General"].get("ScreenPositionX"));
        ScreenParams::posY = std::stof(ini["General"].get("ScreenPositionY"));
        ScreenParams::gameToViewportScaling = std::stof(ini["General"].get("GameToScreenScaling"));
        ScreenParams::autoPositionSetup = ini["General"].get("AutoPositionSetup") == "true";
        ScreenParams::autoGameToViewportScaling = ini["General"].get("AutoGameToScreenScaling") == "true";

        //-----------------------------------------------------------------------------------
        //									    Textures
        //-----------------------------------------------------------------------------------
        TextureData::useTextures = ini["Textures"].get("UseTextures") == "true";
        TextureFileData::bossBarFile = dllPath + ini["Textures"].get("BossBarFillFile");
        TextureFileData::bossBorderFile = dllPath + ini["Textures"].get("BossBarBorderFile");
        TextureFileData::entityBarFile = dllPath + ini["Textures"].get("EntityBarFillFile");
        TextureFileData::entityBorderFile = dllPath + ini["Textures"].get("EntityBarBorderFile");
        TextureData::bossOffset = FillTextureOffset{
            { std::stof(ini["Textures"].get("BossFillTopLeftOffsetX")), std::stof(ini["Textures"].get("BossFillTopLeftOffsetY")) },
            { std::stof(ini["Textures"].get("BossFillBotRightOffsetX")), std::stof(ini["Textures"].get("BossFillBotRightOffsetY")) }
        };
        TextureData::entityOffset = FillTextureOffset{
            { std::stof(ini["Textures"].get("EntityFillTopLeftOffsetX")), std::stof(ini["Textures"].get("EntityFillTopLeftOffsetY")) },
            { std::stof(ini["Textures"].get("EntityFillBotRightOffsetX")), std::stof(ini["Textures"].get("EntityFillBotRightOffsetY")) }
        };

        //-----------------------------------------------------------------------------------
        //									    Style
        //-----------------------------------------------------------------------------------
        BarStyle::fillAlignment = EFillAlignment(stoi(ini["Style"].get("FillAlignment")));
        assert((int)BarStyle::fillAlignment >= 0 && BarStyle::fillAlignment < EFillAlignment::Last);
        BarStyle::fillType = EFillType(stoi(ini["Style"].get("FillType")));
        assert((int)BarStyle::fillType >= 0 && BarStyle::fillType < EFillType::Last);
        BarStyle::fillResizeType = EFillResizeType(stoi(ini["Style"].get("FillResizeType")));
        assert((int)BarStyle::fillResizeType >= 0 && BarStyle::fillResizeType < EFillResizeType::Last);

        auto&& staggerColorMax = splitString(ini["Style"].get("StaggerColorMax"), ",");
        assert(staggerColorMax.size() == 4);
        BarStyle::staggerMaxColor = ImVec4(std::stof(staggerColorMax[0]), std::stof(staggerColorMax[1]), std::stof(staggerColorMax[2]), std::stof(staggerColorMax[3]));

        auto&& staggerColorMin = splitString(ini["Style"].get("StaggerColorMin"), ",");
        assert(staggerColorMin.size() == 4);
        BarStyle::staggerMinColor = ImVec4(std::stof(staggerColorMin[0]), std::stof(staggerColorMin[1]), std::stof(staggerColorMin[2]), std::stof(staggerColorMin[3]));

        auto&& staminaColorMax = splitString(ini["Style"].get("StaminaColorMax"), ",");
        assert(staminaColorMax.size() == 4);
        BarStyle::staminaMaxColor = ImVec4(std::stof(staminaColorMax[0]), std::stof(staminaColorMax[1]), std::stof(staminaColorMax[2]), std::stof(staminaColorMax[3]));

        auto&& staminaColorMin = splitString(ini["Style"].get("StaminaColorMin"), ",");
        assert(staminaColorMin.size() == 4);
        BarStyle::staminaMinColor = ImVec4(std::stof(staminaColorMin[0]), std::stof(staminaColorMin[1]), std::stof(staminaColorMin[2]), std::stof(staminaColorMin[3]));

		barHeightMultiplier = std::stof(ini["Style"].get("BarHeightMultiplier"));
		assert(barHeightMultiplier > 0.f);

		auto&& poisonColorMax = splitString(ini["Style"].get("PoisonColorMax"), ",");
		assert(poisonColorMax.size() == 4);
		BarStyle::poisonMaxColor = ImVec4(std::stof(poisonColorMax[0]), std::stof(poisonColorMax[1]), std::stof(poisonColorMax[2]), std::stof(poisonColorMax[3]));

		auto&& poisonColorMin = splitString(ini["Style"].get("PoisonColorMin"), ",");
		assert(poisonColorMin.size() == 4);
		BarStyle::poisonMinColor = ImVec4(std::stof(poisonColorMin[0]), std::stof(poisonColorMin[1]), std::stof(poisonColorMin[2]), std::stof(poisonColorMin[3]));

		auto&& rotColorMax = splitString(ini["Style"].get("RotColorMax"), ",");
		assert(rotColorMax.size() == 4);
		BarStyle::rotMaxColor = ImVec4(std::stof(rotColorMax[0]), std::stof(rotColorMax[1]), std::stof(rotColorMax[2]), std::stof(rotColorMax[3]));

		auto&& rotColorMin = splitString(ini["Style"].get("RotColorMin"), ",");
		assert(rotColorMin.size() == 4);
		BarStyle::rotMinColor = ImVec4(std::stof(rotColorMin[0]), std::stof(rotColorMin[1]), std::stof(rotColorMin[2]), std::stof(rotColorMin[3]));

		auto&& bleedColorMax = splitString(ini["Style"].get("BleedColorMax"), ",");
		assert(bleedColorMax.size() == 4);
		BarStyle::bleedMaxColor = ImVec4(std::stof(bleedColorMax[0]), std::stof(bleedColorMax[1]), std::stof(bleedColorMax[2]), std::stof(bleedColorMax[3]));

		auto&& bleedColorMin = splitString(ini["Style"].get("BleedColorMin"), ",");
		assert(bleedColorMin.size() == 4);
		BarStyle::bleedMinColor = ImVec4(std::stof(bleedColorMin[0]), std::stof(bleedColorMin[1]), std::stof(bleedColorMin[2]), std::stof(bleedColorMin[3]));

		auto&& blightColorMax = splitString(ini["Style"].get("BlightColorMax"), ",");
		assert(blightColorMax.size() == 4);
		BarStyle::blightMaxColor = ImVec4(std::stof(blightColorMax[0]), std::stof(blightColorMax[1]), std::stof(blightColorMax[2]), std::stof(blightColorMax[3]));

		auto&& blightColorMin = splitString(ini["Style"].get("BlightColorMin"), ",");
		assert(blightColorMin.size() == 4);
		BarStyle::blightMinColor = ImVec4(std::stof(blightColorMin[0]), std::stof(blightColorMin[1]), std::stof(blightColorMin[2]), std::stof(blightColorMin[3]));

		auto&& frostColorMax = splitString(ini["Style"].get("FrostColorMax"), ",");
		assert(frostColorMax.size() == 4);
		BarStyle::frostMaxColor = ImVec4(std::stof(frostColorMax[0]), std::stof(frostColorMax[1]), std::stof(frostColorMax[2]), std::stof(frostColorMax[3]));

		auto&& frostColorMin = splitString(ini["Style"].get("FrostColorMin"), ",");
		assert(frostColorMin.size() == 4);
		BarStyle::frostMinColor = ImVec4(std::stof(frostColorMin[0]), std::stof(frostColorMin[1]), std::stof(frostColorMin[2]), std::stof(frostColorMin[3]));

		auto&& sleepColorMax = splitString(ini["Style"].get("SleepColorMax"), ",");
		assert(sleepColorMax.size() == 4);
		BarStyle::sleepMaxColor = ImVec4(std::stof(sleepColorMax[0]), std::stof(sleepColorMax[1]), std::stof(sleepColorMax[2]), std::stof(sleepColorMax[3]));

		auto&& sleepColorMin = splitString(ini["Style"].get("SleepColorMin"), ",");
		assert(sleepColorMin.size() == 4);
		BarStyle::sleepMinColor = ImVec4(std::stof(sleepColorMin[0]), std::stof(sleepColorMin[1]), std::stof(sleepColorMin[2]), std::stof(sleepColorMin[3]));

		auto&& madnessColorMax = splitString(ini["Style"].get("MadnessColorMax"), ",");
		assert(madnessColorMax.size() == 4);
		BarStyle::madnessMaxColor = ImVec4(std::stof(madnessColorMax[0]), std::stof(madnessColorMax[1]), std::stof(madnessColorMax[2]), std::stof(madnessColorMax[3]));

        //-----------------------------------------------------------------------------------
        //									    Boss Posture Bar
        //-----------------------------------------------------------------------------------
        BossPostureBarData::drawBars = ini["Boss Posture Bar"].get("DrawBars") == "true";
        BossPostureBarData::useStaminaForNPC = ini["Boss Posture Bar"].get("UseStaminaForNPC") == "true";
        BossPostureBarData::barWidth = std::stof(ini["Boss Posture Bar"].get("BarWidth"));
        BossPostureBarData::barHeight = std::stof(ini["Boss Posture Bar"].get("BarHeight"));
        BossPostureBarData::resetStaggerTotalTime = std::stof(ini["Boss Posture Bar"].get("ResetStaggerTotalTime"));
        BossPostureBarData::firstBossScreenX = std::stof(ini["Boss Posture Bar"].get("FirstBossScreenX"));
        BossPostureBarData::firstBossScreenY = std::stof(ini["Boss Posture Bar"].get("FirstBossScreenY"));
        BossPostureBarData::nextBossBarDiffScreenY = std::stof(ini["Boss Posture Bar"].get("NextBossBarDiffScreenY"));

		BossPostureBarData::drawPoiseBar = ini["Boss Posture Bar"].get("DrawPoiseBar") == "true";
        BossPostureBarData::drawPoisonBar = ini["Boss Posture Bar"].get("DrawPoisonBar") == "true";
		BossPostureBarData::drawRotBar = ini["Boss Posture Bar"].get("DrawRotBar") == "true";
		BossPostureBarData::drawBleedBar = ini["Boss Posture Bar"].get("DrawBleedBar") == "true";
		BossPostureBarData::drawBlightBar = ini["Boss Posture Bar"].get("DrawBlightBar") == "true";
		BossPostureBarData::drawFrostBar = ini["Boss Posture Bar"].get("DrawFrostBar") == "true";
		BossPostureBarData::drawSleepBar = ini["Boss Posture Bar"].get("DrawSleepBar") == "true";
		BossPostureBarData::drawMadnessBar = ini["Boss Posture Bar"].get("DrawMadnessBar") == "true";

        //-----------------------------------------------------------------------------------
        //									    Entity Posture Bar
        //-----------------------------------------------------------------------------------
        PostureBarData::drawBars = ini["Entity Posture Bar"].get("DrawBars") == "true";
        PostureBarData::useStaminaForNPC = ini["Entity Posture Bar"].get("UseStaminaForNPC") == "true";
        PostureBarData::barWidth = std::stof(ini["Entity Posture Bar"].get("BarWidth"));
        PostureBarData::barHeight = std::stof(ini["Entity Posture Bar"].get("BarHeight"));
        PostureBarData::resetStaggerTotalTime = std::stof(ini["Entity Posture Bar"].get("ResetStaggerTotalTime"));
        PostureBarData::offsetScreenX = std::stof(ini["Entity Posture Bar"].get("OffsetScreenX"));
        PostureBarData::offsetScreenY = std::stof(ini["Entity Posture Bar"].get("OffsetScreenY"));
        PostureBarData::leftScreenThreshold = std::stof(ini["Entity Posture Bar"].get("LeftScreenThreshold"));
        PostureBarData::rightScreenThreshold = std::stof(ini["Entity Posture Bar"].get("RightScreenThreshold"));
        PostureBarData::topScreenThreshold = std::stof(ini["Entity Posture Bar"].get("TopScreenThreshold"));
        PostureBarData::bottomScreenThreshold = std::stof(ini["Entity Posture Bar"].get("BottomScreenThreshold"));
        PostureBarData::usePositionFixing = ini["Entity Posture Bar"].get("UsePositionFixing") == "true";
        PostureBarData::positionFixingMultiplierX = std::stof(ini["Entity Posture Bar"].get("PositionFixingMultiplierX"));
        PostureBarData::positionFixingMultiplierY = std::stof(ini["Entity Posture Bar"].get("PositionFixingMultiplierY"));

		PostureBarData::drawPoiseBar = ini["Entity Posture Bar"].get("DrawPoiseBar") == "true";
		PostureBarData::drawPoisonBar = ini["Entity Posture Bar"].get("DrawPoisonBar") == "true";
		PostureBarData::drawRotBar = ini["Entity Posture Bar"].get("DrawRotBar") == "true";
		PostureBarData::drawBleedBar = ini["Entity Posture Bar"].get("DrawBleedBar") == "true";
		PostureBarData::drawBlightBar = ini["Entity Posture Bar"].get("DrawBlightBar") == "true";
		PostureBarData::drawFrostBar = ini["Entity Posture Bar"].get("DrawFrostBar") == "true";
		PostureBarData::drawSleepBar = ini["Entity Posture Bar"].get("DrawSleepBar") == "true";
		PostureBarData::drawMadnessBar = ini["Entity Posture Bar"].get("DrawMadnessBar") == "true";

        //-----------------------------------------------------------------------------------
        //									    Experimental
        //-----------------------------------------------------------------------------------
        hideBarsOnMenu = ini["Experimental"].get("HideBarsOnMenu") == "true";
        PlayerPostureBarData::drawBar = ini["Experimental"].get("DrawBar") == "true";
        PlayerPostureBarData::barWidth = std::stof(ini["Experimental"].get("BarWidth"));
        PlayerPostureBarData::barHeight = std::stof(ini["Experimental"].get("BarHeight"));
        PlayerPostureBarData::resetStaggerTotalTime = std::stof(ini["Experimental"].get("ResetStaggerTotalTime"));
        PlayerPostureBarData::screenX = std::stof(ini["Experimental"].get("ScreenX"));
        PlayerPostureBarData::screenY = std::stof(ini["Experimental"].get("ScreenY"));

        //-----------------------------------------------------------------------------------
        //									    Debug
        //-----------------------------------------------------------------------------------
        Logger::useLogger = ini["Debug"].get("Log") == "true";
        offsetTesting = ini["Debug"].get("OffsetTest") == "true";
    }
    catch(const std::exception& e)
    {
        Logger::useLogger = true;
        Logger::log(e.what(), LogLevel::Error);
        return false;
    }
    catch (...) 
    {
        Logger::useLogger = true;
        Logger::log("Unknown exception during loading of PostureBarModConfig.ini", LogLevel::Error);
        return false;
    }

    Logger::log("Dll path: " + dllPath);
    Logger::log("Loaded ini settings: ");
    Logger::log("\tGeneral:");
    Logger::log("\t\tInGameScreenCoordWidth: " + std::to_string(ScreenParams::inGameCoordSizeX));
    Logger::log("\t\tInGameScreenCoordHeight: " + std::to_string(ScreenParams::inGameCoordSizeY));
    Logger::log("\t\tScreenPositionX: " + std::to_string(ScreenParams::posX));
    Logger::log("\t\tScreenPositionY: " + std::to_string(ScreenParams::posY));
    Logger::log("\t\tGameToScreenScaling: " + std::to_string(ScreenParams::gameToViewportScaling));
    Logger::log("\t\tAutoPositionSetup: " + std::to_string(ScreenParams::autoPositionSetup));
    Logger::log("\t\tAutoGameToScreenScaling: " + std::to_string(ScreenParams::autoGameToViewportScaling));
    Logger::log("\tTextures:");
    Logger::log("\t\tUseTextures: " + std::to_string(TextureData::useTextures));
    Logger::log("\t\tBossBarFile: " + TextureFileData::bossBarFile);
    Logger::log("\t\tBossBorderFile: " + TextureFileData::bossBorderFile);
    Logger::log("\t\tEntityBarFile: " + TextureFileData::entityBarFile);
    Logger::log("\t\tEntityBorderFile: " + TextureFileData::entityBorderFile);
    Logger::log("\t\tBossFillOffset: " + std::to_string(TextureData::bossOffset));
    Logger::log("\t\tEntityFillOffset: " + std::to_string(TextureData::entityOffset));
    Logger::log("\tStyle:");
    Logger::log("\t\tFillAlignment: " + std::to_string((int)BarStyle::fillAlignment));
    Logger::log("\t\tFillType: " + std::to_string((int)BarStyle::fillType));
    Logger::log("\t\tFillResizeType: " + std::to_string((int)BarStyle::fillResizeType));
    Logger::log("\t\tStaggerMaxColor: " + std::to_string(BarStyle::staggerMaxColor));
    Logger::log("\t\tStaggerMinColor: " + std::to_string(BarStyle::staggerMinColor));
    Logger::log("\t\tStaminaMaxColor: " + std::to_string(BarStyle::staminaMaxColor));
    Logger::log("\t\tStaminaMinColor: " + std::to_string(BarStyle::staminaMinColor));
    Logger::log("\tBoss Posture Bar:");
    Logger::log("\t\tDrawBars: " + std::to_string(BossPostureBarData::drawBars));
    Logger::log("\t\tUseStaminaForNPC: " + std::to_string(BossPostureBarData::useStaminaForNPC));
    Logger::log("\t\tBarWidth: " + std::to_string(BossPostureBarData::barWidth));
    Logger::log("\t\tBarHeight: " + std::to_string(BossPostureBarData::barHeight));
    Logger::log("\t\tResetStaggerTotalTime: " + std::to_string(BossPostureBarData::resetStaggerTotalTime));
    Logger::log("\t\tFirstBossScreenX: " + std::to_string(BossPostureBarData::firstBossScreenX));
    Logger::log("\t\tFirstBossScreenY: " + std::to_string(BossPostureBarData::firstBossScreenY));
    Logger::log("\t\tNextBossBarDiffScreenY: " + std::to_string(BossPostureBarData::nextBossBarDiffScreenY));
    Logger::log("\tEntity Posture Bar:");
    Logger::log("\t\tDrawBars: " + std::to_string(PostureBarData::drawBars));
    Logger::log("\t\tUseStaminaForNPC: " + std::to_string(PostureBarData::useStaminaForNPC));
    Logger::log("\t\tBarWidth: " + std::to_string(PostureBarData::barWidth));
    Logger::log("\t\tBarHeight: " + std::to_string(PostureBarData::barHeight));
    Logger::log("\t\tResetStaggerTotalTime: " + std::to_string(PostureBarData::resetStaggerTotalTime));
    Logger::log("\t\tOffsetScreenX: " + std::to_string(PostureBarData::offsetScreenX));
    Logger::log("\t\tOffsetScreenY: " + std::to_string(PostureBarData::offsetScreenY));
    Logger::log("\t\tLeftScreenThreshold: " + std::to_string(PostureBarData::leftScreenThreshold));
    Logger::log("\t\tRightScreenThreshold: " + std::to_string(PostureBarData::rightScreenThreshold));
    Logger::log("\t\tTopScreenThreshold: " + std::to_string(PostureBarData::topScreenThreshold));
    Logger::log("\t\tBottomScreenThreshold: " + std::to_string(PostureBarData::bottomScreenThreshold));
    Logger::log("\t\tUsePositionFixing: " + std::to_string(PostureBarData::usePositionFixing));
    Logger::log("\t\tPositionFixingMultiplierX: " + std::to_string(PostureBarData::positionFixingMultiplierX));
    Logger::log("\t\tPositionFixingMultiplierY: " + std::to_string(PostureBarData::positionFixingMultiplierY));
    Logger::log("\tExperimental:");
    Logger::log("\t\tHideBarsOnMenu: " + std::to_string(hideBarsOnMenu));
    Logger::log("\t\tDrawBar: " + std::to_string(PlayerPostureBarData::drawBar));
    Logger::log("\t\tBarWidth: " + std::to_string(PlayerPostureBarData::barWidth));
    Logger::log("\t\tBarHeight: " + std::to_string(PlayerPostureBarData::barHeight));
    Logger::log("\t\tResetStaggerTotalTime: " + std::to_string(PlayerPostureBarData::resetStaggerTotalTime));
    Logger::log("\t\tScreenX: " + std::to_string(PlayerPostureBarData::screenX));
    Logger::log("\t\tScreenY: " + std::to_string(PlayerPostureBarData::screenY));
    Logger::log("\tDebug:");
    Logger::log("\t\tLog: " + std::to_string(Logger::useLogger));
    Logger::log("\t\tOffsetTest: " + std::to_string(offsetTesting));

    return true;
}

bool saveTestOffsetToIni()
{
    try
    {
        using namespace mINI;
        INIFile config(dllPath + "mods\\PostureBarModConfig.ini");
        INIStructure ini;

        if (!config.read(ini))
            throw std::exception("Failed to read PostureModConfig.ini in mods folder");

        switch (debugState)
        {
            using enum EDebugTestState;

            case BossBarOffset:
                ini["Boss Posture Bar"]["FirstBossScreenX"] = std::to_string(BossPostureBarData::firstBossScreenX);
                ini["Boss Posture Bar"]["FirstBossScreenY"] = std::to_string(BossPostureBarData::firstBossScreenY);
                break;
            case EntityBarOffset:
                ini["Entity Posture Bar"]["OffsetScreenX"] = std::to_string(PostureBarData::offsetScreenX);
                ini["Entity Posture Bar"]["OffsetScreenY"] = std::to_string(PostureBarData::offsetScreenY);
                break;
            case GameScreenOffset:
                ini["General"]["ScreenPositionX"] = std::to_string(ScreenParams::posX);
                ini["General"]["ScreenPositionY"] = std::to_string(ScreenParams::posY);
                break;
            case PosFixingMultiplier:
                ini["Entity Posture Bar"]["PositionFixingMultiplierX"] = std::to_string((float)PostureBarData::positionFixingMultiplierX);
                ini["Entity Posture Bar"]["PositionFixingMultiplierY"] = std::to_string((float)PostureBarData::positionFixingMultiplierY);
                break;
        }

        config.write(ini, true);
    }
    catch (const std::exception& e)
    {
        Logger::useLogger = true;
        Logger::log(e.what());
        return false;
    }
    catch (...)
    {
        Logger::useLogger = true;
        Logger::log("Unknown exception during saving of PostureBarModConfig.ini");
        return false;
    }

    return true;
}

bool findDllPath()
{
    try
    {
        char path[MAX_PATH];
        HMODULE moduleHandle = NULL;

        // Get the handle to this module (the current DLL)
        if (!GetModuleHandleExA(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCSTR>(&findDllPath),
            &moduleHandle))
            throw std::exception("Failed to find dll module handle");

        // Get the path of the current DLL
        if (!GetModuleFileNameA(moduleHandle, path, sizeof(path)))
            throw std::exception("Failed to get module name");

        dllPath = path;

        size_t lastBackslashPos = dllPath.find_last_of("\\");
        if (lastBackslashPos != std::string::npos)
        {
            // Remove the file name from the path
            dllPath.erase(lastBackslashPos + 1);
        }
    }
    catch (const std::exception& e)
    {
        Logger::useLogger = true;
        Logger::log(e.what());
        return false;
    }
    catch (...)
    {
        Logger::useLogger = true;
        Logger::log("Unknown exception during finding dll module");
        return false;
    }

    return true;
}

void MainThread()
{
    if (!findDllPath())
        return;

    if (!loadIni())
        return;

    std::this_thread::sleep_for(3s);
    Logger::log("Starting Main Thread");
    g_D3DRenderer = std::make_unique<D3DRenderer>();
    g_D3DRenderer->loadBarTextures();
    g_postureUI = std::make_unique<PostureBarUI>();
    g_Hooking = std::make_unique<Hooking>();
    g_Hooking->Hook();

    Logger::log("Starting Main Loop");
    std::pair<float, float> previousMoveVec{1.f, 1.f};
    int counter = 0;
    while (g_Running)
    {
        if (offsetTesting)
        {
            std::pair<float, float> moveVec{ float((GetAsyncKeyState(VK_RIGHT) & 1) - (GetAsyncKeyState(VK_LEFT) & 1)), float((GetAsyncKeyState(VK_DOWN) & 1) - (GetAsyncKeyState(VK_UP) & 1)) };

            if (counter <= 0)
                offsetSpeed = 0.5f;
            else
                offsetSpeed = std::min(5.f, offsetSpeed + 0.5f);

            if (previousMoveVec == moveVec)
                counter = 20;
            else
                counter = std::max(0, counter - 1);

            switch (debugState)
            {
                using enum EDebugTestState;

                case BossBarOffset:
                    BossPostureBarData::firstBossScreenX += moveVec.first * offsetSpeed;
                    BossPostureBarData::firstBossScreenY += moveVec.second * offsetSpeed;
                    break;
                case EntityBarOffset:
                    PostureBarData::offsetScreenX += moveVec.first * offsetSpeed;
                    PostureBarData::offsetScreenY += moveVec.second * offsetSpeed;
                    break;
                case GameScreenOffset:
                    ScreenParams::posX += moveVec.first * offsetSpeed;
                    ScreenParams::posY += moveVec.second * offsetSpeed;
                    break;
                case PosFixingMultiplier:
                    PostureBarData::positionFixingMultiplierX = std::clamp(PostureBarData::positionFixingMultiplierX + (moveVec.first * std::ceil(offsetSpeed) * 0.1), 0.0, 20.0);
                    PostureBarData::positionFixingMultiplierY = std::clamp(PostureBarData::positionFixingMultiplierY - (moveVec.second * std::ceil(offsetSpeed) * 0.1), 0.0, 20.0);
                    break;
            }

            if (GetAsyncKeyState(VK_INSERT) & 1)
                saveTestOffsetToIni();

            if (GetAsyncKeyState(VK_NEXT) & 1)
                debugState = cycleState(debugState, 1);

            if (GetAsyncKeyState(VK_PRIOR) & 1)
                debugState = cycleState(debugState, -1);

            if (moveVec.first != 0.f || moveVec.second != 0.f)
                previousMoveVec = moveVec;
        }

        // reload ini settings for colors and bar toggles
        if (GetAsyncKeyState(VK_RETURN)) {
            loadIni();
        }

        std::this_thread::sleep_for(3ms);
        std::this_thread::yield();
    }

    std::this_thread::sleep_for(500ms);
    FreeLibraryAndExitThread(g_Module, 0);
}