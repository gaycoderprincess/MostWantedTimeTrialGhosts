struct tChallengeSeriesEvent {
	std::string sEventName;
	std::string sCarPreset;
};

std::vector<tChallengeSeriesEvent> aNewChallengeSeries = {
	//{"16.2.1", "M3GTRCAREERSTART"},
	{"16.2.1", "RAZORMUSTANG"},
	//{"19.8.32", "CS_CAR_01"},
	{"15.1.1", "CE_997S"},
	{"15.1.2", "BL15"},
	//{"19.8.52", "CS_CAR_PIZZA"},
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
	{"6.2.2", "CE_SUPRA"},
	{"6.1.1", "BL6"},
	{"1.2.4", "CS_CAR_17"},
	{"5.2.2", "BL5"},
	{"3.3.2.r", "CS_CAR_08"},
	{"4.5.2", "BL4"},
	{"3.3.1", "CS_CAR_16"},
	{"3.1.2.r", "BL3"},
	{"3.1.1.r", "BL2"},
	{"1.2.3", "E3_DEMO_BMW"},
	{"2.2.1.r", "COP_CROSS"},
};

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

const char* GetChallengeSeriesEventDescription3(uint32_t hash) {
	DLLDirSetter _setdir;

	auto trackName = GetLocalizedString(hash);
	if (pSelectedEvent->sEventName == "19.8.31") trackName = "Burger King Challenge";
	static std::string str;
	uint32_t pbTime = 0;
	tReplayGhost targetTime;
	auto carName = pSelectedEvent->sCarPreset;
	if (auto preset = FindFEPresetCar(bStringHashUpper(carName.c_str()))) {
		carName = preset->CarTypeName;
		std::transform(carName.begin(), carName.end(), carName.begin(), [](wchar_t c){ return std::tolower(c); });

		auto trackId = GRaceParameters::GetEventID(pSelectedEventParams);
		if (carName == "copsport") carName = "copcross";

		tReplayGhost temp;
		LoadPB(&temp, carName, trackId, GRaceParameters::GetNumLaps(pSelectedEventParams), 0, nullptr);
		pbTime = temp.nFinishTime;

		targetTime = SelectTopGhost(carName, trackId, GRaceParameters::GetNumLaps(pSelectedEventParams), nullptr);

		auto tmp = CreateStockCarRecord(carName.c_str());
		carName = GetLocalizedString(FECarRecord::GetNameHash(&tmp));
		if (pSelectedEvent->sCarPreset == "CE_GTRSTREET") carName += " (Street)";
	}
	str = std::format("Track: {}\nCar: {}", trackName, carName);
	if (targetTime.nFinishTime > 0) {
		str += std::format("\nTarget Time: {}", GetTimeFromMilliseconds(targetTime.nFinishTime));
		str.pop_back();
		if (targetTime.sPlayerName[0]) {
			str += std::format(" ({})", targetTime.sPlayerName);
		}
	}
	if (pbTime > 0) {
		str += std::format("\nBest Time: {}", GetTimeFromMilliseconds(pbTime));
		str.pop_back();
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
	auto count = GRaceParameters::GetNumOpponents(pThis);
	if (count < 1) return 1;
	if (nDifficulty != DIFFICULTY_NORMAL) return 1; // only spawn one ghost for easy and hard difficulty
	return count;
}

void ApplyCustomEventsHooks() {
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

	NyaHookLib::Patch<uint8_t>(0x60AB66, 0xEB); // don't sabotage engine
}