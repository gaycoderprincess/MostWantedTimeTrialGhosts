struct tChallengeSeriesEvent {
	std::string sEventName;
	std::string sCarPreset;
};

std::vector<tChallengeSeriesEvent> aNewChallengeSeries = {
	{"16.1.1", "OPM_MUSTANG_BOSS"},
	{"16.2.1", "M3GTRCAREERSTART"},
	{"19.8.32", "CS_CAR_01"},
	{"15.1.1", "CE_997S"},
	{"15.1.2", "BL15"},
	{"19.8.52", "CS_CAR_PIZZA"},
	{"14.2.4", "BL14"},
	{"13.2.3", "BL13"},
	{"12.2.1", "BL12"},
	{"11.1.2", "BL11"},
	{"10.2.1", "BL10"},
	{"9.5.3", "BL9"},
	{"8.2.1", "BL8"},
	{"7.2.2", "BL7"},
	{"6.1.1", "BL6"},
	{"5.2.2", "BL5"},
	{"4.5.2", "BL4"},
	{"3.1.2.r", "BL3"},
	{"3.1.1.r", "BL2"},
	{"1.2.3", "E3_DEMO_BMW"},
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

uint32_t __thiscall GetChallengeSeriesEventDescription1(GRaceParameters* pThis) {
	return CalcLanguageHash("TRACKNAME_", pThis);
}

uint32_t __thiscall GetChallengeSeriesEventDescription2(cFrontendDatabase* pThis, uint32_t a2) {
	return a2;
}

void ApplyCustomEventsHooks() {
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x7AE97F, &GetNumChallengeSeriesEvents);
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x7AEA5E, &GetNumChallengeSeriesEvents);
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x7AE997, &GetChallengeSeriesEventHash);
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x7AE9F4, &IsChallengeSeriesEventUnlocked);
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x6F48DB, &GetChallengeSeriesCarType);
	//NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x6F4945, &GetChallengeSeriesCarPerformance);
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x7A4369, &GetChallengeSeriesEventDescription1);
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x7A4375, &GetChallengeSeriesEventDescription2);
	NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x7AE8A0, 0x51F120); // use UISafehouseRaceSheet::AddRace for correct event icons
	NyaHookLib::Patch(0x51F1B6, 0x8B79D4); // make UISafehouseRaceSheet::AddRace use ChallengeDatum vtable
	NyaHookLib::Patch<uint16_t>(0x7AE9E6, 0x9090); // don't check unlock states
	NyaHookLib::Patch<uint8_t>(0x60AB66, 0xEB); // don't sabotage engine
}