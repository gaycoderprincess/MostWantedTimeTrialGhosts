template<uintptr_t addr>
void ImportIntegrityCheck() {
	static auto tmp1 = *(uint32_t*)addr;
	static auto tmp2 = **(uint32_t**)addr;
	if (tmp1 != *(uint32_t*)addr || tmp2 != **(uint32_t**)addr) {
		exit(0);
	}
}

void VerifyTimers() {
	bInitTicker(60000.0);
#ifdef TIMETRIALS_CARBON
	bInitTicker(60000.0);
	ImportIntegrityCheck<0x86B1EE + 2>(); // QueryPerformanceCounter
	ImportIntegrityCheck<0x9C1170>(); // QueryPerformanceCounter
	ImportIntegrityCheck<0x86B1E8 + 2>(); // QueryPerformanceFrequency
	ImportIntegrityCheck<0x9C1170>(); // QueryPerformanceFrequency
	ImportIntegrityCheck<0x81C740 + 2>(); // GetTickCount
	ImportIntegrityCheck<0x9C107C>(); // GetTickCount
#else
	ImportIntegrityCheck<0x7C3F58 + 2>(); // QueryPerformanceCounter
	ImportIntegrityCheck<0x89017C>(); // QueryPerformanceCounter
	ImportIntegrityCheck<0x7C3F58 + 2>(); // QueryPerformanceFrequency
	ImportIntegrityCheck<0x890180>(); // QueryPerformanceFrequency
	ImportIntegrityCheck<0x8352B3 + 2>(); // GetTickCount
	ImportIntegrityCheck<0x8900A4>(); // GetTickCount
#endif
}

void ApplyVerificationPatches() {
#ifdef TIMETRIALS_CARBON
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
#endif
}

tReplayTick VerifyPlayer;
void CollectPlayerPos() {
	if (TheGameFlowManager.CurrentGameFlowState != GAMEFLOW_STATE_RACING) return;
	if (IsInLoadingScreen()) return;

	if (auto ply = GetLocalPlayerVehicle()) {
		VerifyPlayer.Collect(ply);
	}
}

void CheckPlayerPos() {
	if (TheGameFlowManager.CurrentGameFlowState != GAMEFLOW_STATE_RACING) return;
	if (IsInLoadingScreen()) return;
	if (bViewReplayMode) return;

	if (auto ply = GetLocalPlayerVehicle()) {
		auto tmp = VerifyPlayer;
		tmp.Collect(ply);
		if (memcmp(&tmp, &VerifyPlayer, sizeof(VerifyPlayer))) {
			exit(0);
		}
	}
}