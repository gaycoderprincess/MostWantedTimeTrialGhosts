template<uintptr_t addr>
void ImportIntegrityCheck() {
	static auto tmp1 = *(uint32_t*)addr;
	static auto tmp2 = **(uint32_t**)addr;
	if (tmp1 != *(uint32_t*)addr || tmp2 != **(uint32_t**)addr) {
		exit(0);
	}
}

void VerifyTimers() {
#ifdef TIMETRIALS_PROSTREET
	bInitTicker(60000.0);
	ImportIntegrityCheck<0x430F07 + 2>(); // QueryPerformanceCounter
	ImportIntegrityCheck<0x967178>(); // QueryPerformanceCounter
	ImportIntegrityCheck<0x917B6E + 2>(); // QueryPerformanceFrequency
	ImportIntegrityCheck<0x967130>(); // QueryPerformanceFrequency
	ImportIntegrityCheck<0x9048A0 + 2>(); // GetTickCount
	ImportIntegrityCheck<0x96708C>(); // GetTickCount
#elif TIMETRIALS_UNDERGROUND2
	bInitTicker();
	ImportIntegrityCheck<0x43BDF8 + 2>(); // QueryPerformanceCounter
	ImportIntegrityCheck<0x78311C>(); // QueryPerformanceCounter
	ImportIntegrityCheck<0x6F5F06 + 2>(); // QueryPerformanceFrequency
	ImportIntegrityCheck<0x783118>(); // QueryPerformanceFrequency
	ImportIntegrityCheck<0x74B730 + 2>(); // GetTickCount
	ImportIntegrityCheck<0x78320C>(); // GetTickCount
#elif TIMETRIALS_CARBON
	bInitTicker(60000.0);
	ImportIntegrityCheck<0x86B1EE + 2>(); // QueryPerformanceCounter
	ImportIntegrityCheck<0x9C1170>(); // QueryPerformanceCounter
	ImportIntegrityCheck<0x86B1E8 + 2>(); // QueryPerformanceFrequency
	ImportIntegrityCheck<0x9C1170>(); // QueryPerformanceFrequency
	ImportIntegrityCheck<0x81C740 + 2>(); // GetTickCount
	ImportIntegrityCheck<0x9C107C>(); // GetTickCount
#elif TIMETRIALS_MOST_WANTED
	bInitTicker(60000.0);
	ImportIntegrityCheck<0x7C3F58 + 2>(); // QueryPerformanceCounter
	ImportIntegrityCheck<0x89017C>(); // QueryPerformanceCounter
	ImportIntegrityCheck<0x7C3F58 + 2>(); // QueryPerformanceFrequency
	ImportIntegrityCheck<0x890180>(); // QueryPerformanceFrequency
	ImportIntegrityCheck<0x8352B3 + 2>(); // GetTickCount
	ImportIntegrityCheck<0x8900A4>(); // GetTickCount
#endif
}

bool bVerifyPlayerCollected = false;
void InvalidatePlayerPos() {
	bVerifyPlayerCollected = false;
}

#ifdef TIMETRIALS_MOST_WANTED
struct tVerifyTick {
	IVehicle* ptr;

	// car state
	bool staging;
	bool destroyed;
	bool active;
	bool loading;
	bool offWorld;
	uint8_t forceStop;
	uint8_t style;
	uint8_t physics;
	uint8_t invuln;
	uint8_t spiked[4];
	bool gearChanging;
	float mass;
	float rpm;
	float hp;

	// nos data
	bool nosEngaged;
	float nosCapacity;
	float nosBoost;
	float nosFlowRate;

	void Collect(IVehicle* pVehicle) {
		ptr = pVehicle;

		staging = pVehicle->IsStaging();
		destroyed = pVehicle->IsDestroyed();
		active = pVehicle->IsActive();
		loading = pVehicle->IsLoading();
		offWorld = pVehicle->IsOffWorld();
		forceStop = pVehicle->GetForceStop();
		style = pVehicle->GetDriverStyle();
		physics = pVehicle->GetPhysicsMode();

		mass = pVehicle->mCOMObject->Find<IRigidBody>()->GetMass();

		nosCapacity = pVehicle->mCOMObject->Find<IEngine>()->GetNOSCapacity();
		nosBoost = pVehicle->mCOMObject->Find<IEngine>()->GetNOSBoost();
		nosFlowRate = pVehicle->mCOMObject->Find<IEngine>()->GetNOSFlowRate();

		rpm = pVehicle->mCOMObject->Find<IEngine>()->GetRPM();
		nosEngaged = pVehicle->mCOMObject->Find<IEngine>()->IsNOSEngaged();
		hp = pVehicle->mCOMObject->Find<IEngine>()->GetHorsePower();

		invuln = pVehicle->mCOMObject->Find<IRBVehicle>()->GetInvulnerability();

		for (int i = 0; i < 4; i++) {
			spiked[i] = pVehicle->mCOMObject->Find<ISpikeable>()->GetTireDamage(i);
		}

		gearChanging = pVehicle->mCOMObject->Find<ITransmission>()->IsGearChanging();
	}
} VerifyPlayerExtras;
#endif

tReplayTick VerifyPlayer;

#ifdef TIMETRIALS_MOST_WANTED
template<bool checkForFinalPursuit>
#endif
void CollectPlayerPos() {
	bVerifyPlayerCollected = false;
	if (TheGameFlowManager.CurrentGameFlowState != GAMEFLOW_STATE_RACING) return;
	if (IsInLoadingScreen()) return;

#ifdef TIMETRIALS_MOST_WANTED
	auto race = GetCurrentRace();
	if (race && GRaceParameters::GetIsPursuitRace(race) != checkForFinalPursuit) return;
#endif

	if (auto ply = GetLocalPlayerVehicle()) {
		VerifyPlayer.Collect(ply);

#ifdef TIMETRIALS_MOST_WANTED
		VerifyPlayerExtras.Collect(ply);
#endif

		bVerifyPlayerCollected = true;
	}
}

#ifdef TIMETRIALS_MOST_WANTED
template<bool checkForFinalPursuit>
#endif
void CheckPlayerPos() {
	if (TheGameFlowManager.CurrentGameFlowState != GAMEFLOW_STATE_RACING) return;
	if (IsInLoadingScreen()) return;
	if (bViewReplayMode) return;
	if (!bVerifyPlayerCollected) return;

#ifdef TIMETRIALS_MOST_WANTED
	auto race = GetCurrentRace();
	if (race && GRaceParameters::GetIsPursuitRace(race) != checkForFinalPursuit) return;
#endif

	if (auto ply = GetLocalPlayerVehicle()) {
		auto tmp = VerifyPlayer;
		tmp.Collect(ply);

#ifdef TIMETRIALS_MOST_WANTED
		auto tmp2 = VerifyPlayerExtras;
		tmp2.Collect(ply);

		if (memcmp(&tmp2, &VerifyPlayerExtras, sizeof(VerifyPlayerExtras))) {
			exit(0);
		}
#endif

#ifdef TIMETRIALS_UNDERGROUND2
		if (memcmp(&tmp, &VerifyPlayer.v1.state, sizeof(VerifyPlayer.v1.state))) {
#else
		if (memcmp(&tmp, &VerifyPlayer, sizeof(VerifyPlayer))) {
#endif
			exit(0);
		}
	}
}

namespace MemoryIntegrity {
	struct MemorySection {
		std::string sName;
		uint8_t* aMemory = nullptr;
		size_t nSize = 0;
		uintptr_t nStartAddress = 0;

		size_t nCursor = 0;

		MemorySection(std::string name) : sName(name) {}
	};
	std::vector<MemorySection> aSections = { MemorySection(".text"), MemorySection(".rdata"), MemorySection(".idata") };

	uintptr_t aWhitelistedAddresses[] = {
#ifdef TIMETRIALS_UNDERCOVER
		// some random stuff that genericfix sets, which is set late for some reason??
		// anyone reading this: please for the love of god never do this
		0x59C04C,
		0x59C068,
		0x59C085,
		0x59D0F1,
		0x59D103,
		0x59D10C,
		0x59D153,
		0x753013,
		0x75301B,
		0x753026,
		0x768C62,
		0x768C72,

		0x5A49F8,
#elif TIMETRIALS_PROSTREET
		// racer ai
		0x41F040,
#elif TIMETRIALS_UNDERGROUND2

#elif TIMETRIALS_CARBON
		// HD reflections "cubemap fix"
		0x9E87E4,
		0x9E87EC,
		0x9E8804,
		0x9E880C,

		// challenge series hooks - these will change when swapping between quick race and challenges
		0x63F450,
		0x63C660,

		// racer ai
		0x9C4F80,
#elif TIMETRIALS_MOST_WANTED
		// challenge series hooks - these will change when swapping between quick race and challenges
		0x426CA6,
		0x431533,
		0x611902,
		0x61DCB7,

		// racer ai
		0x892748,

		// some random thing related to look behind?
		0x4741D0,
#endif
	};

#ifdef TIMETRIALS_UNDERCOVER
	uintptr_t aWhitelistedAddressesUCReformed[] = {
		0x5A49F8,
		0x5A4C39,
		0xC21A44,
		0xC21A48,
		0xC21A4C,
		0xC21A50,
		0xC21A54,
		0xC21A58,
		0xC21A5C,
		0xC21A60,
	};
#endif
	bool IsWhitelisted(uintptr_t address) {
		for (auto& addr : aWhitelistedAddresses) {
			if (address >= addr && address <= addr + 5) return true;
		}
#ifdef TIMETRIALS_UNDERCOVER
		for (auto& addr : aWhitelistedAddressesUCReformed) {
			if (address >= addr && address <= addr + 5) {
				gUndercoverModData.bReformedInstalled = true;
				aNewChallengeSeries = &aReformedChallengeSeries;
				return true;
			}
		}
#endif
		return false;
	}

	void CheckerThread(int sectionId) {
		while (true) {
			auto& section = aSections[sectionId];
			for (int i = 0; i < 8192; i++) {
				uintptr_t address = section.nStartAddress + section.nCursor;
				if (*(uint8_t*)address != section.aMemory[section.nCursor] && !IsWhitelisted(address)) {
					WriteLog(std::format("Integrity check failed at {:X}", address));;
					exit(0);
				}
				section.nCursor++;
				if (section.nCursor >= section.nSize) section.nCursor = 0;
			}
			Sleep(50);
		}
	}

	void Init() {
		auto module = (uintptr_t)GetModuleHandleA(0);
		auto dosHeader = (PIMAGE_DOS_HEADER)module;
		auto ntHeader = (PIMAGE_NT_HEADERS)(module + dosHeader->e_lfanew);
		auto sectionHeader = (PIMAGE_SECTION_HEADER)((uintptr_t)&ntHeader->OptionalHeader + ntHeader->FileHeader.SizeOfOptionalHeader);

		for (int i = 0; i < ntHeader->FileHeader.NumberOfSections; i++) {
			auto sectionData = &sectionHeader[i];
			for (auto& section : aSections) {
				if (section.sName == (char*)sectionData->Name) {
					section.nSize = sectionData->SizeOfRawData;
					section.nStartAddress = module + sectionData->VirtualAddress;
					break;
				}
			}
		}

		for (auto& section : aSections) {
			if (!section.nStartAddress) continue;

			section.aMemory = new uint8_t[section.nSize];
			memcpy(section.aMemory, (void*)section.nStartAddress, section.nSize);
			std::thread(CheckerThread, &section - &aSections[0]).detach();
		}
	}
}

namespace FileIntegrity {
#ifdef TIMETRIALS_UNDERGROUND2
	const char* aFilesToCheck[] = {
			"GLOBAL/GLOBALB.LZC",
			"TRACKS/L4RA.BUN",
			"TRACKS/L4RB.BUN",
			"TRACKS/L4RC.BUN",
			"TRACKS/L4RD.BUN",
			"TRACKS/L4RF.BUN",
			"TRACKS/L4RG.BUN",
			"TRACKS/L4RH.BUN",
			"TRACKS/L4RR.BUN",
	};
#else
	const char* aFilesToCheck[] = {
			"GLOBAL/ATTRIBUTES.BIN",
#ifdef TIMETRIALS_UNDERCOVER
			"GLOBAL/CARS_VAULT.BIN",
#endif
			"GLOBAL/FE_ATTRIB.BIN",
			"GLOBAL/GAMEPLAY.BIN",
			"GLOBAL/GAMEPLAY.LZC",
			"TRACKS/L2RA.BUN",
			//"TRACKS/STREAML2RA.BUN",
			"TRACKS/L5RB.BUN",
			//"TRACKS/STREAML5RB.BUN",
			"TRACKS/L8R_MW2.BUN",
			//"TRACKS/STREAML8R_MW2.BUN",

			// prostreet tracks
			"TRACKS/L6R_AutobahnDrift.BUN",
			"TRACKS/L6R_Autopolis.BUN",
			"TRACKS/L6R_ChicagoAirfield.BUN",
			"TRACKS/L6R_Ebisu.BUN",
			"TRACKS/L6R_INFINEON.BUN",
			"TRACKS/L6R_LEIPZIG.BUN",
			"TRACKS/L6R_MondelloPark.BUN",
			"TRACKS/L6R_NevadaDrift.BUN",
			"TRACKS/L6R_PortlandRaceway.BUN",
			"TRACKS/L6R_ShutoDrift.BUN",
			"TRACKS/L6R_ShutoExpressway.BUN",
			"TRACKS/L6R_TexasSpeedway.BUN",
			"TRACKS/L6R_WillowSprings.BUN",
	};
#endif
	char* aGameData = nullptr;
	size_t nGameDataCursor = 0;

	uint32_t hash32_copy(const uint8_t *str, uint32_t len, uint32_t magic) {
		int v4; // eax
		uint32_t v5; // ecx
		int v6; // ebp
		int v7; // edi
		uint32_t v9; // edx
		uint32_t v10; // ecx
		uint32_t v11; // esi
		int v12; // eax
		uint32_t v13; // ecx
		uint32_t v14; // esi
		int v15; // eax
		uint32_t v16; // ecx
		uint32_t v17; // esi
		uint32_t v18; // esi
		int v19; // edi
		uint32_t v20; // ecx
		uint32_t v21; // esi
		int v22; // edi
		uint32_t v23; // ecx
		uint32_t v24; // esi
		int v25; // edi

		v4 = len;
		v5 = 0x9E3779B9;
		v6 = len;
		v7 = 0x9E3779B9;
		if ( (uint32_t)len >= 0xC )
		{
			v9 = len / 0xCu;
			do
			{
				auto str_32 = (uint32_t*)str;
				v10 = str_32[1] + v5;
				v11 = str_32[2] + magic;
				v12 = (v11 >> 13) ^ (v7 + str_32[0] - v11 - v10);
				v13 = (v12 << 8) ^ (v10 - v11 - v12);
				v14 = (v13 >> 13) ^ (v11 - v13 - v12);
				v15 = (v14 >> 12) ^ (v12 - v14 - v13);
				v16 = (v15 << 16) ^ (v13 - v14 - v15);
				v17 = (v16 >> 5) ^ (v14 - v16 - v15);
				v7 = (v17 >> 3) ^ (v15 - v17 - v16);
				v5 = (v7 << 10) ^ (v16 - v17 - v7);
				magic = (v5 >> 15) ^ (v17 - v5 - v7);
				str += 12;
				v6 -= 12;
				--v9;
			}
			while ( v9 );
			v4 = len;
		}
		v18 = v4 + magic;
		switch ( v6 )
		{
			case 1:
				goto LABEL_16;
			case 2:
				goto LABEL_15;
			case 3:
				goto LABEL_14;
			case 4:
				goto LABEL_13;
			case 5:
				goto LABEL_12;
			case 6:
				goto LABEL_11;
			case 7:
				goto LABEL_10;
			case 8:
				goto LABEL_9;
			case 9:
				goto LABEL_8;
			case 10:
				goto LABEL_7;
			case 11:
				v18 += str[10] << 24;
			LABEL_7:
				v18 += str[9] << 16;
			LABEL_8:
				v18 += str[8] << 8;
			LABEL_9:
				v5 += str[7] << 24;
			LABEL_10:
				v5 += str[6] << 16;
			LABEL_11:
				v5 += str[5] << 8;
			LABEL_12:
				v5 += str[4];
			LABEL_13:
				v7 += str[3] << 24;
			LABEL_14:
				v7 += str[2] << 16;
			LABEL_15:
				v7 += str[1] << 8;
			LABEL_16:
				v7 += str[0];
				break;
			default:
				break;
		}
		v19 = (v18 >> 13) ^ (v7 - v18 - v5);
		v20 = (v19 << 8) ^ (v5 - v18 - v19);
		v21 = (v20 >> 13) ^ (v18 - v20 - v19);
		v22 = (v21 >> 12) ^ (v19 - v21 - v20);
		v23 = (v22 << 16) ^ (v20 - v21 - v22);
		v24 = (v23 >> 5) ^ (v21 - v23 - v22);
		v25 = (v24 >> 3) ^ (v22 - v24 - v23);
		return (((v25 << 10) ^ (v23 - v24 - v25)) >> 15) ^ (v24 - ((v25 << 10) ^ (v23 - v24 - v25)) - v25);
	}

	void AddFileData(const std::filesystem::path& filePath) {
		auto size = std::filesystem::file_size(filePath);
		std::ifstream file(filePath, std::ios::in | std::ios::binary);
		if (!file.is_open()) return;
		file.read(&aGameData[nGameDataCursor], size);
		nGameDataCursor += size;
	}

	void VerifyGameFiles() {
		size_t size = 0;
		for (auto& file : aFilesToCheck) {
			if (!std::filesystem::exists(file)) continue;
			size += std::filesystem::file_size(file);
		}
		aGameData = new char[size];
		for (auto& file : aFilesToCheck) {
			if (!std::filesystem::exists(file)) continue;
			AddFileData(file);
		}
		nLocalGameFilesHash = hash32_copy((uint8_t*)aGameData, size, 0xABCDEF00);

#ifdef TIMETRIALS_PROSTREET
		if (*(uint32_t*)0x49296F != 0x01E482D9 || *(uint32_t*)0x494059 != 0x01E482D9) bTankUnslapperPresent = true;
#endif

		delete[] aGameData;
	}
}

void ApplyVerificationPatches() {
#ifdef TIMETRIALS_UNDERCOVER
	// undo exopts gamespeed
	static float f = 1.0;
	NyaHookLib::Patch(0x7BCDD4, &f);
	NyaHookLib::Patch(0x7BCDEE, &f);

	// undo exopts rev limit
	NyaHookLib::Patch(0xC1C688, 0x7358F0);
	NyaHookLib::Patch(0xC1C8A0, 0x7358F0);
	NyaHookLib::Patch(0xC1CAC8, 0x7358F0);
	NyaHookLib::Patch(0xC1CCE0, 0x7358F0);

	Tweak_ForceStraightPursuit = 0;

	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x7B44EE, 0x4C7740); // remove exopts loop, disables hotkeys
#elif TIMETRIALS_PROSTREET

#elif TIMETRIALS_UNDERGROUND2
	// exopts - reenable barriers
	NyaHookLib::WriteString(0x7A0780, "PLAYER_BARRIERS_%d");
	NyaHookLib::WriteString(0x7A0794, "BARRIERS_%d");
	NyaHookLib::Patch<uint64_t>(0x578070, 0x890000008A80BF0F);

	// undo exopts gamespeed
	static float f = 1.0;
	NyaHookLib::Patch(0x601A65, &f);

	// exopts - drift stuff
	NyaHookLib::Patch<uint8_t>(0x56CC58, 5);
	NyaHookLib::Patch<uint8_t>(0x56CC90, 5);

	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x581470, 0x4022C0); // remove exopts loop, disables hotkeys
#elif TIMETRIALS_CARBON
	// exopts - reenable barriers
	NyaHookLib::WriteString(0x9D85C4, "BARRIER_SPLINE_4501");
	NyaHookLib::WriteString(0x9D85D8, "BARRIER_SPLINE_4500");
	NyaHookLib::WriteString(0x9D85EC, "BARRIER_SPLINE_4091");
	NyaHookLib::WriteString(0x9D8600, "BARRIER_SPLINE_4090");
	NyaHookLib::WriteString(0x9D8614, "BARRIER_SPLINE_306");
	NyaHookLib::WriteString(0x9D8628, "BARRIER_SPLINE_305");
	NyaHookLib::WriteString(0x9D8B30, "BARRIER_SPLINE_%d");

	// exopts - drift stuff
	NyaHookLib::Patch<uint8_t>(0x6BE947, 10);
	NyaHookLib::Patch<uint8_t>(0x6AB943, 20);
	NyaHookLib::Patch<uint8_t>(0x6AB945, 20);
	Tweak_DriftRaceCollisionThreshold = 3.5;
	AugmentedDriftWithEBrake = false;

	// undo exopts gamespeed
	static float f = 1.0;
	NyaHookLib::Patch(0x7683BA, &f);
	NyaHookLib::Patch(0x7683CB, &f);
	NyaHookLib::Patch<uint16_t>(0x46CE42, 0x9090);

	Tweak_GameBreakerRechargeTime = 25.0;
	Tweak_GameBreakerRechargeSpeed = 80.0;
	Tweak_GameBreakerCollisionMass = 2.0;
#elif TIMETRIALS_MOST_WANTED
	// exopts - reenable barriers
	NyaHookLib::WriteString(0x8B2810, "SCENERY_GROUP_");
	NyaHookLib::WriteString(0x8B2820, "PLAYER_BARRIERS_");
	NyaHookLib::WriteString(0x8B2834, "BARRIERS_");
	NyaHookLib::WriteString(0x8B2840, "BARRIER_");

	// undo exopts gamespeed
	static float f = 1.0;
	NyaHookLib::Patch(0x6F4D1A, &f);
	NyaHookLib::Patch(0x6F4D2B, &f);
	NyaHookLib::Patch(0x78AA77, &f);

	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x663EE8, 0x64B380); // remove exopts loop, disables hotkeys

	// tweak_infinitenos
	static bool b = false;
	NyaHookLib::Patch(0x692AB2, &b);
#endif

	MemoryIntegrity::Init();
}