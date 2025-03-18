#pragma once
#include "../Common.hpp"

namespace ER 
{
	namespace GameData
	{
		struct BossHpBar {
			int displayId = -1;
			uint32_t pad0x4 = 0;
			unsigned long long bossHandle = 0;
			int currentDisplayDamage = 0;
			int unk0x14 = 0;
			bool isHit = 0;
			uint8_t pad0x19[0x3] = {};
			float displayTime = 0;
		};

		static_assert(sizeof(BossHpBar) == 0x20);

		struct EntityHpBar {
			unsigned long long entityHandle = 0;
			float unk[2];
			float screenPosX;
			float screenPosY;
			float mod;
			uint32_t unk3;
			char unkChar1;
			char unkChar2;
			uint16_t unkShort3;
			int currentDisplayDamage = -1;
			int previousHp = 0;
			float timeDisplayed;
			float totalTimeDisplayed;
			bool isVisible;
			char unk2[11];
		};

		static_assert(sizeof(EntityHpBar) == 0x40);

		struct CSFeManImp 
		{
			uint8_t undefined[0x59F0];
			EntityHpBar entityHpBars[ENTITY_CHR_ARRAY_LEN];
			BossHpBar bossHpBars[BOSS_CHR_ARRAY_LEN];
		};

		struct StatModule
		{
			uint8_t undefined[0x138];
			int health;
			int healthMax;
			int healthBase;
			int unk;
			int focus;
			int focusMax;
			int focusBase;
			int stamina;
			int staminaMax;
			int staminaBase;
		};

		struct Module0x18
		{
			uint8_t undefined[0x20];
			int currentAnimation;
		};

		struct ResistanceModule
		{
			uint8_t undefined[0x10];
			int poison;
			int rot;
			int bleed;
			int blight;
			int frost;
			int sleep;
			int madness;

			int poisonMax;
			int rotMax;
			int bleedMax;
			int blightMax;
			int frostMax;
			int sleepMax;
			int madnessMax;

			float poisonMod;
			float rotMod;
			float bleedMod;
			float blightMod;
			float frostMod;
			float sleepMod;
			float madnessMod;

			float poisonMul;
			float rotMul;
			float bleedMul;
			float blightMul;
			float frostMul;
			float sleepMul;
			float madnessMul;
		};


		struct StaggerModule
		{
			//uint8_t undefined1;
			//bool useStagger;
			//uint8_t undefined2[0xE];
			uint8_t undefined2[0x10];
			float stagger;
			float staggerMax;
			uint8_t undefined3[0x4];
			float resetTimer;
		};

		struct PhysicsModule
		{
			char pad_0000[84];
			float Azimuth;
			char pad_0058[24];
			float worldPosX;
			float worldPosY;
			float worldPosZ;
		};

		struct ChrModuleBag
		{
			StatModule* statModule;
			uint8_t undefined1[0x10];
			Module0x18* animModule;
			ResistanceModule* resistanceModule;
			uint8_t undefined2[0x18];
			StaggerModule* staggerModule;
			uint8_t undefined3[0x20];
			PhysicsModule* physicsModule;
		};

		struct ChrIns
		{
			uint8_t undefined[0x8];
			unsigned long long handle;
			uint8_t undefined2[0x50];
			int npcParam;
			int modelNumber;
			int chrType;
			uint8_t teamType;
			uint8_t undefined3[0x123];
			ChrModuleBag* chrModulelBag;
			uint8_t undefined4[0x508];
			unsigned long long targetHandle;
		};

		struct WorldChrMan {
			char unk[0x10EF8];
			ChrIns** playerArray[0x4];
		};
	}

	class Hooking
	{
	public:
		explicit Hooking();
		~Hooking() noexcept;
		Hooking(Hooking const&) = delete;
		Hooking(Hooking&&) = delete;
		Hooking& operator=(Hooking const&) = delete;
		Hooking& operator=(Hooking&&) = delete;

		void Hook();
		void Unhook();

		// Game found Signatures
		uintptr_t worldChrSignature{};
		uintptr_t CSFeManSignature{};
		uintptr_t isLoading{};
		uintptr_t menuState1{};
		uintptr_t menuState2{};
		uintptr_t menuState3{};

		// Static in game functions
		typedef GameData::ChrIns* (*GetChrInsFromHandle)(GameData::WorldChrMan* worldChrMan, uint64_t* chrInsHandlePtr);
		GetChrInsFromHandle GetChrInsFromHandleFunc{};

		typedef void (*UpdateUIBarStructs)(uintptr_t, uintptr_t);
		UpdateUIBarStructs UpdateUIBarStructsFunc{};
	};

	inline std::unique_ptr<Hooking> g_Hooking;
}
