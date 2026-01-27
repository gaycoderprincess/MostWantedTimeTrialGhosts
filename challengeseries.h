std::string GetCarNameForGhost(const std::string& carPreset) {
	auto carName = carPreset;
	if (auto preset = FindFEPresetCar(FEHashUpper(carName.c_str()))) {
		auto car = Attrib::FindCollection(Attrib::StringHash32("pvehicle"), preset->VehicleKey);
		if (!car) {
			MessageBoxA(0, std::format("Failed to find pvehicle for {} ({} {:X})", carPreset, preset->CarTypeName, preset->VehicleKey).c_str(), "nya?!~", MB_ICONERROR);
			exit(0);
		}
		return *(const char**)Attrib::Collection::GetData(car, Attrib::StringHash32("CollectionName"), 0);
	}
	return carName;
}

class ChallengeSeriesEvent {
public:
	std::string sEventName;
	std::string sCarPreset;
	int nLapCountOverride = 0;

	int nNumGhosts = 0;
	std::string sCarNameForGhost;
	tReplayGhost PBGhost = {};
	tReplayGhost aTargetGhosts[NUM_DIFFICULTY] = {};

	ChallengeSeriesEvent(const char* eventName, const char* carPreset, int lapCount = 0) : sEventName(eventName), sCarPreset(carPreset), nLapCountOverride(lapCount) {}

	GRaceParameters* GetRace() const {
		return GRaceDatabase::GetRaceFromHash(GRaceDatabase::mObj, Attrib::StringHash32(sEventName.c_str()));
	}

	int GetLapCount() {
		if (nLapCountOverride > 0) return nLapCountOverride;
		return GRaceParameters::GetNumLaps(GetRace());
	}

	void ClearPBGhost() {
		PBGhost = {};
	}

	std::string GetCarModelName() {
		if (!sCarNameForGhost.empty()) return sCarNameForGhost;
		return GetCarNameForGhost(sCarPreset);
	}

	tReplayGhost GetPBGhost() {
		if (PBGhost.nFinishTime != 0) return PBGhost;

		tReplayGhost temp;
		LoadPB(&temp, GetCarModelName(), sEventName, GetLapCount(), 0, nullptr);
		PBGhost = temp;
		return temp;
	}

	tReplayGhost GetTargetGhost() {
		if (aTargetGhosts[nDifficulty].nFinishTime != 0) return aTargetGhosts[nDifficulty];

		tReplayGhost targetTime;
		auto times = CollectReplayGhosts(GetCarModelName(), sEventName, GetLapCount(), nullptr);
		if (!times.empty()) {
			times[0].aTicks.clear(); // just in case
			targetTime = aTargetGhosts[nDifficulty] = times[0];
		}
		nNumGhosts = times.size();
		return targetTime;
	}
};

extern std::vector<ChallengeSeriesEvent> aNewChallengeSeries;

ChallengeSeriesEvent* GetChallengeEvent(uint32_t hash) {
	for (auto& event : aNewChallengeSeries) {
		if (!GRaceDatabase::GetRaceFromHash(GRaceDatabase::mObj, Attrib::StringHash32(event.sEventName.c_str()))) {
			MessageBoxA(0, std::format("Failed to find event {}", event.sEventName).c_str(), "nya?!~", MB_ICONERROR);
			exit(0);
		}
	}
	for (auto& event : aNewChallengeSeries) {
		if (Attrib::StringHash32(event.sEventName.c_str()) == hash) return &event;
	}
	return nullptr;
}

ChallengeSeriesEvent* GetChallengeEvent(const std::string& str) {
	for (auto& event : aNewChallengeSeries) {
		if (event.sEventName == str) return &event;
	}
	return nullptr;
}

int CalculateTotalTimes() {
	uint32_t totalTime = 0;
	for (auto& event : aNewChallengeSeries) {
		auto time = event.GetPBGhost().nFinishTime;
		if (!time) return 0;
		totalTime += time;
	}
	return totalTime;
}

void OnChallengeSeriesEventPB() {
	auto event = GetChallengeEvent(GRaceParameters::GetEventID(GRaceStatus::fObj->mRaceParms));
	if (!event) return;
	event->ClearPBGhost();
}

void PrecacheAllChallengeGhosts() {
	// all pbs first, all targets after
	for (auto& event : aNewChallengeSeries) {
		event.GetPBGhost();
	}
	for (auto& event : aNewChallengeSeries) {
		event.GetTargetGhost();
	}
}

void OnChallengeSeriesLoaded() {
	// precache model names so the thread doesn't have to check attrib
	for (auto& event : aNewChallengeSeries) {
		event.GetCarModelName();
	}

	std::thread(PrecacheAllChallengeGhosts).detach();
}