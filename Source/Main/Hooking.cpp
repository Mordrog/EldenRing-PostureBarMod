#include "Hooking.hpp"
#include "Logger.hpp"
#include "D3DRenderer.hpp"
#include "PostureBarUI.hpp"
#include "Memory.hpp"

namespace ER 
{
	Hooking::Hooking()
	{
		MH_Initialize();
	}

	Hooking::~Hooking()
	{
		MH_RemoveHook(MH_ALL_HOOKS);
	}

	void Hooking::Hook()
	{
		Logger::log("Start Hooking");
		g_D3DRenderer->Hook();

		Logger::log("Hooking Elden Ring methods");

		Logger::log("Looking for worldChr signature");
		if (worldChrSignature = Signature("48 8B 05 ? ? ? ? 48 85 C0 74 0F 48 39 88").Scan().Add(3).Rip().As<uint64_t>(); !worldChrSignature)
			Logger::log("Failed to find worldChr signature", LogLevel::Error);

		Logger::log("Looking for CSFeMan signature");
		if (CSFeManSignature = Signature("48 8B 0D ? ? ? ? 8B DA 48 85 C9 75 ? 48 8D 0D ? ? ? ? E8 ? ? ? ? 4C 8B C8 4C 8D 05 ? ? ? ? BA B4 00 00 00 48 8D 0D ? ? ? ? E8 ? ? ? ? 48 8B 0D ? ? ? ? 8B D3 E8 ? ? ? ? 48 8B D8").Scan().Add(3).Rip().As<uint64_t>(); !CSFeManSignature)
			Logger::log("Failed to find worldChr signature", LogLevel::Error);

		Logger::log("Looking for GetChrInsFromHandleFunc signature");
		if (GetChrInsFromHandleFunc = (GetChrInsFromHandle)Signature("48 83 EC 28 E8 17 FF FF FF 48 85 C0 74 08 48 8B 00 48 83 C4 28 C3").Scan().As<uint64_t>(); !GetChrInsFromHandleFunc)
			Logger::log("Failed to find GetChrInsFromHandleFunc signature", LogLevel::Error);

		Logger::log("Looking for UpdateUIBarStructsFunc signature");
		if (UpdateUIBarStructsFunc = (UpdateUIBarStructs)Signature("40 55 56 57 41 54 41 55 41 56 41 57 48 83 EC 60 48 C7 44 24 30 FE FF FF FF 48 89 9C 24 B0 00 00 00 48 8B 05 ? ? ? ? 48 33 C4 48 89 44 24 58 48").Scan().As<uint64_t>(); !UpdateUIBarStructsFunc)
			Logger::log("Failed to find UpdateUIBarStructsFunc signature", LogLevel::Error);

		Logger::log("Hooking UpdateUIBarStructsFunc");
		if (MH_CreateHook(UpdateUIBarStructsFunc, &g_postureUI->updateUIBarStructs, (void**)&g_postureUI->updateUIBarStructsOriginal) != MH_STATUS::MH_OK)
			Logger::log("Failed to hook UpdateUIBarStructsFunc", LogLevel::Error);

		MH_EnableHook(MH_ALL_HOOKS);
	}

	void Hooking::Unhook()
	{
		g_D3DRenderer->Unhook();
		MH_RemoveHook(MH_ALL_HOOKS);
	}
}