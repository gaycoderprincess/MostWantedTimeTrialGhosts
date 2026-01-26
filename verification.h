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