struct tChallengeSeriesEvent {
	std::string sEventName;
	std::string sCarPreset;
	int nLapCountOverride = 0;
};

std::vector<tChallengeSeriesEvent> aNewChallengeSeries = {
	//{"16.2.1", "M3GTRCAREERSTART"},
	{"16.2.1", "RAZORMUSTANG"},
	//{"19.8.32", "CS_CAR_01"},
	{"15.1.1", "CE_997S"},
	{"15.1.2", "BL15"},
	{"14.1.1", "CS_CAR_02"},
	{"14.2.4", "BL14"},
	{"16.1.1", "OPM_MUSTANG_BOSS"},
	{"13.2.3", "BL13"},
	{"12.7.3", "CS_CAR_14"},
	{"12.2.1", "BL12"},
	{"11.7.1", "CS_CAR_01"},
	{"11.1.2", "BL11"},
	{"10.3.2", "CE_SUPRA"},
	{"10.2.1", "BL10"},
	{"16.2.2", "DDAYSUPRA"},
	{"9.5.3", "BL9"},
	{"19.8.31", "E3_DEMO_RX8"},
	{"8.2.1", "BL8"},
	{"7.2.1", "CE_GTRSTREET"},
	{"7.2.2", "BL7"},
	{"6.2.2", "CS_CAR_07"},
	{"6.1.1", "BL6"},
	{"1.2.4", "CS_CAR_17"},
	{"5.2.2", "BL5"},
	{"3.3.2.r", "CS_CAR_08"},
	{"4.5.2", "BL4"},
	{"3.3.1", "CS_CAR_16"},
	{"3.1.2.r", "BL3"},
	{"1.5.2", "CASTROLGT"},
	{"3.1.1.r", "BL2"},
	{"19.8.52", "CS_CAR_PIZZA"},
	{"1.2.3", "E3_DEMO_BMW"},
	{"13.5.1", "CE_ELISE"},
	{"1.1.1", "gti", 1},
	{"2.2.1.r", "COP_CROSS"},
};

std::string GetCarNameForGhost(const std::string& carPreset) {
	auto carName = carPreset;
	if (auto preset = FindFEPresetCar(bStringHashUpper(carName.c_str()))) {
		carName = preset->CarTypeName;
		std::transform(carName.begin(), carName.end(), carName.begin(), [](char c){ return std::tolower(c); });
	}
	auto ghostCar = carName;
	if (ghostCar == "copsport") ghostCar = "copcross";
	if (ghostCar == "pizza") ghostCar = "cs_clio_trafpizza";
	return ghostCar;
}

int CalculateTotalTimes() {
	uint32_t time = 0;
	for (auto& event : aNewChallengeSeries) {
		auto eventHash = GRaceDatabase::GetRaceFromHash(GRaceDatabase::mObj, Attrib::StringHash32(event.sEventName.c_str()));
		auto trackId = event.sEventName;
		auto carName = GetCarNameForGhost(event.sCarPreset);

		int numLaps = GRaceParameters::GetNumLaps(eventHash);
		if (event.nLapCountOverride > 0) numLaps = event.nLapCountOverride;

		tReplayGhost temp;
		LoadPB(&temp, carName, trackId, numLaps, 0, nullptr);
		if (!temp.nFinishTime) return 0;
		time += temp.nFinishTime;
	}
	return time;
}

int GetNumChallengeSeriesEvents() {
	return aNewChallengeSeries.size();
}

uint32_t __thiscall GetChallengeSeriesEventHash(GRaceBin* pThis, int id) {
	return Attrib::StringHash32(aNewChallengeSeries[id].sEventName.c_str());
}

bool IsChallengeSeriesEventUnlocked(uint32_t a1, uint32_t eventHash) { // a2 is the event hash
	return true;
}

const char* __thiscall GetChallengeSeriesCarType(GRaceParameters* pThis) {
	auto event = GRaceParameters::GetEventID(pThis);
	for (auto& challenge : aNewChallengeSeries) {
		if (event == challenge.sEventName) {
			return challenge.sCarPreset.c_str();
		}
	}
	return GRaceParameters::GetPlayerCarType(pThis);
}

float __thiscall GetChallengeSeriesCarPerformance(GRaceParameters* pThis) {
	/*auto event = GRaceParameters::GetEventID(pThis);
	for (auto& challenge : aNewChallengeSeries) {
		if (event == challenge.sEventName) {
			return challenge.fCarPerformance;
		}
	}*/
	return GRaceParameters::GetPlayerCarPerformance(pThis);
}

tChallengeSeriesEvent* pSelectedEvent = nullptr;
GRaceParameters* pSelectedEventParams = nullptr;
int nNumGhostsForEvent = 0;
uint32_t __thiscall GetChallengeSeriesEventDescription1(GRaceParameters* pThis) {
	pSelectedEventParams = pThis;
	auto event = GRaceParameters::GetEventID(pThis);
	for (auto& challenge : aNewChallengeSeries) {
		if (event == challenge.sEventName) {
			pSelectedEvent = &challenge;
			break;
		}
	}
	return CalcLanguageHash("TRACKNAME_", pThis);
}

uint32_t __thiscall GetChallengeSeriesEventDescription2(cFrontendDatabase* pThis, uint32_t a2) {
	return a2;
}

std::string GetCarName(const std::string& carModel) {
	auto tmp = CreateStockCarRecord(carModel.c_str());
	std::string carName = GetLocalizedString(FECarRecord::GetNameHash(&tmp));
	if (carName == GetLocalizedString(0x9BB9CCC3)) { // unlocalized string
		carName = carModel;
		std::transform(carName.begin(), carName.end(), carName.begin(), [](char c){ return std::toupper(c); });
	}
	else {
		if (carModel == "bmwm3gtr") carName += " (Street)";
	}

	if (carName == "CEMTR" || carName == "GRAB" || carName == "PICKUPA" || carName == "PIZZA" || carName == "TAXI") {
		carName = "TRAF" + carName;
	}
	if (carName == "CS_CLIO_TRAFPIZZA") carName = "TRAFPIZZA";
	if (carName == "COPSPORT") carName = "COPCROSS";
	return carName;
}

std::string GetTrackName(const std::string& eventId, uint32_t nameHash) {
	if (eventId == "19.8.31") return "Burger King Challenge";

	auto trackName = GetLocalizedString(nameHash);
	if (trackName == GetLocalizedString(0x9BB9CCC3)) { // unlocalized string
		return eventId;
	}
	return trackName;
}

const char* GetChallengeSeriesEventDescription3(uint32_t hash) {
	DLLDirSetter _setdir;

	auto trackName = GetTrackName(pSelectedEvent->sEventName, hash);
	auto carName = GetCarNameForGhost(pSelectedEvent->sCarPreset);

	auto trackId = GRaceParameters::GetEventID(pSelectedEventParams);

	int numLaps = GRaceParameters::GetNumLaps(pSelectedEventParams);
	if (pSelectedEvent->nLapCountOverride > 0) numLaps = pSelectedEvent->nLapCountOverride;

	tReplayGhost temp;
	LoadPB(&temp, carName, trackId, numLaps, 0, nullptr);

	tReplayGhost targetTime;
	auto times = CollectReplayGhosts(carName, trackId, numLaps, nullptr);
	if (!times.empty()) targetTime = times[0];
	nNumGhostsForEvent = times.size();

	static std::string str;
	str = std::format("Track: {}\nCar: {}", GetTrackName(pSelectedEvent->sEventName, hash), GetCarName(carName));
	if (targetTime.nFinishTime > 0) {
		str += std::format("\nTarget Time: {}", GetTimeFromMilliseconds(targetTime.nFinishTime));
		str.pop_back();
		if (targetTime.sPlayerName[0]) {
			str += std::format(" ({})", targetTime.sPlayerName);
		}
	}
	if (temp.nFinishTime > 0) {
		str += std::format("\nBest Time: {}", GetTimeFromMilliseconds(temp.nFinishTime));
		str.pop_back();

		auto total = CalculateTotalTimes();
		if (total > 0) {
			// expertly calculated amount of spaces :3
			str += std::format("                   Completion Time: {}", GetTimeFromMilliseconds(total));
			str.pop_back();
		}
	}
	return str.c_str();
}

bool GetIsChallengeSeriesRace() {
	return true;
}

uint32_t __thiscall GetChallengeSeriesEventIcon1(GRaceParameters* pThis) {
	return GRaceParameters::GetRaceType(pThis);
}

uint32_t __thiscall GetChallengeSeriesEventIcon2(cFrontendDatabase* pThis, int a1, int a2) {
	return cFrontendDatabase::GetRaceIconHash(pThis, a1);
}

int __thiscall GetNumOpponentsHooked(GRaceParameters* pThis) {
	return !bChallengesOneGhostOnly && nDifficulty != DIFFICULTY_EASY ? std::min(nNumGhostsForEvent, 3) : 1; // only spawn one ghost for easy difficulty
}

bool __thiscall GetIsDDayRaceHooked(GRaceParameters* pThis) {
	return false;
}

void ApplyCustomEventsHooks() {
	NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x5FBD20, &GetIsDDayRaceHooked);

	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x6F48DB, &GetChallengeSeriesCarType);
	//NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x6F4945, &GetChallengeSeriesCarPerformance);
	NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x5FC180, &GetIsChallengeSeriesRace);

	// change event list
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x7AE97F, &GetNumChallengeSeriesEvents);
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x7AEA5E, &GetNumChallengeSeriesEvents);
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x7AE997, &GetChallengeSeriesEventHash);
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x7AE9F4, &IsChallengeSeriesEventUnlocked);

	// correct event icons up top
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x7A42BB, &GetChallengeSeriesEventIcon1);
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x7A42DE, &GetChallengeSeriesEventIcon2);
	NyaHookLib::Patch<uint8_t>(0x7A42C2, 0xEB);

	// correct event icons and event type names in the list
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x7AE8C1, 0x5FAA20);
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x7AE8DA, &GetChallengeSeriesEventIcon2);
	NyaHookLib::Patch<uint8_t>(0x7AE8C8, 0xEB);
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x7AE8FF, 0x5FAA20);
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x7AE90B, 0x56E010);

	// custom event descriptions
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x7A4369, &GetChallengeSeriesEventDescription1);
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x7A4375, &GetChallengeSeriesEventDescription2);
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x7A437B, &GetChallengeSeriesEventDescription3);

	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x426CA6, &GetNumOpponentsHooked);
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x431533, &GetNumOpponentsHooked);
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x611902, &GetNumOpponentsHooked);
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x61DCB7, &GetNumOpponentsHooked);

	NyaHookLib::Patch<uint16_t>(0x7AE9E6, 0x9090); // don't check unlock states

	NyaHookLib::Patch<uint16_t>(0x5FD30C, 0x9090); // don't spawn boss characters
	NyaHookLib::Patch<uint16_t>(0x5FD31A, 0x9090); // don't spawn boss characters

	NyaHookLib::Patch<uint8_t>(0x60AB66, 0xEB); // don't sabotage engine
}