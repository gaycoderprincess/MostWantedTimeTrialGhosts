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
#else
	bInitTicker(60000.0);
	ImportIntegrityCheck<0x7C3F58 + 2>(); // QueryPerformanceCounter
	ImportIntegrityCheck<0x89017C>(); // QueryPerformanceCounter
	ImportIntegrityCheck<0x7C3F58 + 2>(); // QueryPerformanceFrequency
	ImportIntegrityCheck<0x890180>(); // QueryPerformanceFrequency
	ImportIntegrityCheck<0x8352B3 + 2>(); // GetTickCount
	ImportIntegrityCheck<0x8900A4>(); // GetTickCount
#endif
}

void ApplyVerificationPatches() {
#ifdef TIMETRIALS_PROSTREET

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
#else
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
#endif
}

bool bVerifyPlayerCollected = false;
void InvalidatePlayerPos() {
	bVerifyPlayerCollected = false;
}

tReplayTick VerifyPlayer;
void CollectPlayerPos() {
	bVerifyPlayerCollected = false;
	if (TheGameFlowManager.CurrentGameFlowState != GAMEFLOW_STATE_RACING) return;
	if (IsInLoadingScreen()) return;

	if (auto ply = GetLocalPlayerVehicle()) {
		VerifyPlayer.Collect(ply);
		bVerifyPlayerCollected = true;
	}
}

void CheckPlayerPos() {
	if (TheGameFlowManager.CurrentGameFlowState != GAMEFLOW_STATE_RACING) return;
	if (IsInLoadingScreen()) return;
	if (bViewReplayMode) return;
	if (!bVerifyPlayerCollected) return;

	if (auto ply = GetLocalPlayerVehicle()) {
		auto tmp = VerifyPlayer;
		tmp.Collect(ply);
#ifdef TIMETRIAL_UNDERGROUND2
		if (memcmp(&tmp, &VerifyPlayer.v1.state, sizeof(VerifyPlayer.v1.state))) {
#else
		if (memcmp(&tmp, &VerifyPlayer, sizeof(VerifyPlayer))) {
#endif
			exit(0);
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
			"GLOBAL/FE_ATTRIB.BIN",
			"GLOBAL/GAMEPLAY.BIN",
			"GLOBAL/GAMEPLAY.LZC",
			"TRACKS/L2RA.BUN",
			//"TRACKS/STREAML2RA.BUN",
			"TRACKS/L5RB.BUN",
			//"TRACKS/STREAML5RB.BUN",

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
		delete[] aGameData;
	}
}