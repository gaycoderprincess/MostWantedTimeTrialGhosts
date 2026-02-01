std::string GetCarName(const std::string& carModel) {
	auto tmp = NyaHelpers::CreateStockCarRecord(carModel.c_str());
	std::string carName = GetLocalizedString(FECarRecord::GetNameHash(&tmp));
	if (carName == GetLocalizedString(0x9BB9CCC3)) { // unlocalized string
		carName = carModel;
		std::transform(carName.begin(), carName.end(), carName.begin(), [](char c){ return std::toupper(c); });
	}
	else {
		if (carModel == "bmwm3gtr") carName += " (Street)";
	}

	if (carName == "CS_CLIO_TRAFPIZZA") carName = "TRAFPIZZA";
	return carName;
}

std::string GetTrackName(const std::string& eventId, uint32_t nameHash) {
	if (eventId == "1.8.1") return "Final Pursuit";
	if (eventId == "19.8.31") return "Burger King Challenge";

	auto trackName = GetLocalizedString(nameHash);
	if (trackName == GetLocalizedString(0x9BB9CCC3)) { // unlocalized string
		return eventId;
	}
	return trackName;
}

std::vector<ChallengeSeriesEvent> aNewChallengeSeries = {
	//ChallengeSeriesEvent("16.2.1", "M3GTRCAREERSTART"),
	ChallengeSeriesEvent("16.2.1", "RAZORMUSTANG"),
	ChallengeSeriesEvent("15.1.1", "CE_997S"),
	ChallengeSeriesEvent("15.1.2", "BL15"),
	ChallengeSeriesEvent("14.1.1", "CS_CAR_02"),
	ChallengeSeriesEvent("14.2.4", "BL14"),
	ChallengeSeriesEvent("16.1.1", "OPM_MUSTANG_BOSS"),
	ChallengeSeriesEvent("13.2.3", "BL13"),
	ChallengeSeriesEvent("12.7.3", "CS_CAR_14"),
	ChallengeSeriesEvent("12.2.1", "BL12"),
	ChallengeSeriesEvent("11.7.1", "CS_CAR_01"),
	ChallengeSeriesEvent("11.1.2", "BL11"),
	ChallengeSeriesEvent("10.3.2", "CE_SUPRA"),
	ChallengeSeriesEvent("10.2.1", "BL10"),
	ChallengeSeriesEvent("16.2.2", "DDAYSUPRA"),
	ChallengeSeriesEvent("9.5.3", "BL9"),
	ChallengeSeriesEvent("19.8.31", "E3_DEMO_RX8"),
	ChallengeSeriesEvent("8.2.1", "BL8"),
	ChallengeSeriesEvent("7.2.1", "CE_GTRSTREET"),
	ChallengeSeriesEvent("7.2.2", "BL7"),
	ChallengeSeriesEvent("6.2.2", "CS_CAR_07"),
	ChallengeSeriesEvent("6.1.1", "BL6"),
	ChallengeSeriesEvent("1.2.4", "CS_CAR_17"),
	ChallengeSeriesEvent("5.2.2", "BL5"),
	ChallengeSeriesEvent("3.3.2.r", "CS_CAR_08"),
	ChallengeSeriesEvent("4.5.2", "BL4"),
	ChallengeSeriesEvent("3.3.1", "CS_CAR_16"),
	ChallengeSeriesEvent("3.1.2.r", "BL3"),
	ChallengeSeriesEvent("1.5.2", "CASTROLGT"),
	ChallengeSeriesEvent("3.1.1.r", "BL2"),
	ChallengeSeriesEvent("19.8.52", "CS_CAR_PIZZA"),
	ChallengeSeriesEvent("1.2.3", "E3_DEMO_BMW"),
	ChallengeSeriesEvent("13.5.1", "CE_ELISE"),
	ChallengeSeriesEvent("1.1.1", "PresetCar/GTI_KURU", 1),
	ChallengeSeriesEvent("19.8.32", "PresetCar/911TURBO_CUSTOM"),
	ChallengeSeriesEvent("1.2.1", "CS_CAR_19"),
	ChallengeSeriesEvent("1.8.1", "E3_DEMO_BMW"),
	ChallengeSeriesEvent("16.2.3", "BL2"),
	ChallengeSeriesEvent("2.2.1.r", "COP_CROSS"),
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
	auto event = GetChallengeEvent(GRaceParameters::GetEventID(pThis));
	if (!event) return "M3GTRCAREERSTART";
	return event->sCarPreset.c_str();
}

float __thiscall GetChallengeSeriesCarPerformance(GRaceParameters* pThis) {
	/*auto event = GetChallengeEvent(GRaceParameters::GetEventID(pThis));
	if (!event) return 0;
	return event.fCarPerformance;*/

	return GRaceParameters::GetPlayerCarPerformance(pThis);
}

ChallengeSeriesEvent* pSelectedEvent = nullptr;
GRaceParameters* pSelectedEventParams = nullptr;
uint32_t __thiscall GetChallengeSeriesEventDescription1(GRaceParameters* pThis) {
	SkipMovies = true;

	pSelectedEventParams = pThis;
	pSelectedEvent = GetChallengeEvent(GRaceParameters::GetEventID(pThis));
	return CalcLanguageHash("TRACKNAME_", pThis);
}

uint32_t __thiscall GetChallengeSeriesEventDescription2(cFrontendDatabase* pThis, uint32_t a2) {
	return a2;
}

const char* GetChallengeSeriesEventDescription3(uint32_t hash) {
	DLLDirSetter _setdir;

	auto carName = GetCarNameForGhost(pSelectedEvent->sCarPreset);

	auto pbTime = pSelectedEvent->GetPBGhost().nFinishTime;
	auto target = pSelectedEvent->GetTargetGhost();
	auto targetTime = target.nFinishTime;
	auto targetName = target.sPlayerName;

	static std::string str;
	str = std::format("Track: {}\nCar: {}", GetTrackName(pSelectedEvent->sEventName, hash), GetCarName(carName));
	if (targetTime > 0) {
		str += std::format("\nTarget Time: {}", GetTimeFromMilliseconds(targetTime));
		str.pop_back();
		if (!targetName.empty()) {
			str += std::format(" ({})", targetName);
		}
	}
	if (pbTime > 0) {
		str += std::format("\nBest Time: {}", GetTimeFromMilliseconds(pbTime));
		str.pop_back();

		auto total = CalculateTotalTimes();
		if (total > 0) {
			// expertly calculated amount of spaces :3
			str += std::format("                   Completion Time: {}", GetTimeFromMilliseconds(total));
			str.pop_back();
		}
	}

	// disable SpawnCop, fixes dday issues
	NyaHookLib::Patch<uint8_t>(0x60A67A, GRaceParameters::GetIsPursuitRace(pSelectedEventParams) ? 0x74 : 0xEB);

	return str.c_str();
}

bool GetIsChallengeSeriesRace() {
	return true;
}

uint32_t __thiscall GetChallengeSeriesEventIcon1(GRaceParameters* pThis) {
	if (GRaceParameters::GetIsPursuitRace(pThis)) return GRaceParameters::GetChallengeType(pThis);
	return GRaceParameters::GetRaceType(pThis);
}

uint32_t __thiscall GetChallengeSeriesEventIcon2(cFrontendDatabase* pThis, int a1, int a2) {
	auto icon = cFrontendDatabase::GetRaceIconHash(pThis, a1);
	if (!icon) return cFrontendDatabase::GetMilestoneIconHash(pThis, a1, a2);
	return icon;
}

int __thiscall GetNumOpponentsHooked(GRaceParameters* pThis) {
	auto event = GetChallengeEvent(GRaceParameters::GetEventID(pThis));
	if (!event) return 8;

	if (bViewReplayMode) return 0;
	if (bChallengesOneGhostOnly) return 1;
	return nDifficulty != DIFFICULTY_EASY ? std::min(event->nNumGhosts[nDifficulty], 3) : 1; // only spawn one ghost for easy difficulty
}

bool __thiscall GetIsDDayRaceHooked(GRaceParameters* pThis) {
	return false;
}

bool __thiscall GetIsFinalPursuitHooked(cFrontendDatabase* pThis) {
	return false;
}

bool __thiscall GetIsFinalPursuitForCopSpawnsHooked(cFrontendDatabase* pThis) {
	if (!GRaceStatus::fObj) return false;
	auto parms = GRaceStatus::fObj->mRaceParms;
	if (!parms) return false;

	if (!strcmp(GRaceParameters::GetEventID(parms), "1.8.1")) return true;
	return false;
}

void __thiscall FinalPursuitEndHooked(ICEManager* pThis, const char* a1, const char* a2) {
	ICEManager::SetGenericCameraToPlay(pThis, a1, a2);

	if (!GRaceParameters::GetIsPursuitRace(GRaceStatus::fObj->mRaceParms)) return;

	DLLDirSetter _setdir;
	OnFinishRace();

	// do a config save when finishing a race
	DoConfigSave();
}

bool __thiscall GetIsEventCompleteHooked(GRaceDatabase* pThis, uint32_t raceHash, uint32_t a3) {
	DLLDirSetter _setdir;

	auto event = GetChallengeEvent(raceHash);
	auto pb = event->GetPBGhost().nFinishTime;
	if (pb && pb <= event->GetTargetGhost().nFinishTime) return true;
	return false;
}

std::vector<NyaHookLib::PatchWithUndo*> aChallengeSeriesHooks;
void ApplyCustomEventsHooks() {
	bChallengeSeriesMode = true;
	for (auto& patch : aChallengeSeriesHooks) {
		patch->ApplyPatch();
	}
}

void UndoCustomEventsHooks() {
	bChallengeSeriesMode = false;
	for (auto& patch : aChallengeSeriesHooks) {
		patch->UndoPatch();
	}
}

UIQRChallengeSeries* CreateQRChallengeSeries_Hooked(ScreenConstructorData* a1) {
	ApplyCustomEventsHooks();
	return CreateQRChallengeSeries(a1);
}

auto UIQRMainMenu_orig = (UIQRMainMenu*(__thiscall*)(UIQRMainMenu*, int))nullptr;
UIQRMainMenu* __thiscall UIQRMainMenu_Hooked(UIQRMainMenu* pThis, int a2) {
	UndoCustomEventsHooks();
	return UIQRMainMenu_orig(pThis, a2);
}

PresetCar* FindFEPresetCarHooked(uint32_t hash) {
	for (auto& event : aNewChallengeSeries) {
		if (event.sCarPreset.find('/') == std::string::npos) continue;
		if (hash == bStringHashUpper(event.sCarPreset.c_str())) {
			if (event.PresetCarData.PresetName[0]) return &event.PresetCarData;

			std::ifstream file(std::format("{}/CwoeeGhosts/{}", gDLLPath.string(), event.sCarPreset), std::ios::in | std::ios::binary);
			if (!file.is_open()) {
				MessageBoxA(nullptr, std::format("Failed to find preset car {}", event.sCarPreset).c_str(), "nya?!~", MB_ICONERROR);
				exit(0);
			}

			auto preset = &event.PresetCarData;
			memset(preset, 0, sizeof(*preset));
			file.read((char*)preset, sizeof(*preset));
			preset->PhysicsLevel = 0xFFFFFFFF;
			return preset;
		}
	}

	auto node = PresetCarList.HeadNode.Next;
	while (node != &PresetCarList.HeadNode) {
		if (hash == FEHashUpper(node->PresetName)) {
			return node;
		}
		node = node->Next;
	}
	return nullptr;
}

void SetupCustomEventsHooks() {
	NyaHookLib::Patch(0x8F436C, &CreateQRChallengeSeries_Hooked);
	UIQRMainMenu_orig = (UIQRMainMenu*(__thiscall*)(UIQRMainMenu*, int))NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x570803, &UIQRMainMenu_Hooked);

	NyaHooks::LateInitHook::aFunctions.push_back(OnChallengeSeriesLoaded);

	NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x748130, &FindFEPresetCarHooked);

	new NyaHookLib::PatchWithUndo(&aChallengeSeriesHooks, NyaHookLib::JMP, 0x5FBD20, &GetIsDDayRaceHooked);
	new NyaHookLib::PatchWithUndo(&aChallengeSeriesHooks, NyaHookLib::JMP, 0x56DC00, &GetIsFinalPursuitHooked);
	new NyaHookLib::PatchWithUndo(&aChallengeSeriesHooks, NyaHookLib::JMP, 0x5FC560, &GetIsFinalPursuitHooked);

	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x7AEA30, &GetIsEventCompleteHooked);
	new NyaHookLib::PatchWithUndo(&aChallengeSeriesHooks, NyaHookLib::CALL, 0x443004, &GetIsFinalPursuitForCopSpawnsHooked);
	new NyaHookLib::PatchWithUndo(&aChallengeSeriesHooks, NyaHookLib::CALL, 0x44430A, &GetIsFinalPursuitForCopSpawnsHooked);
	new NyaHookLib::PatchWithUndo(&aChallengeSeriesHooks, NyaHookLib::CALL, 0x71A826, &GetIsFinalPursuitForCopSpawnsHooked);
	new NyaHookLib::PatchWithUndo(&aChallengeSeriesHooks, NyaHookLib::JMP, 0x6F19DB, 0x6F1C1F); // disable milestone display
	new NyaHookLib::PatchWithUndo(&aChallengeSeriesHooks, NyaHookLib::CALL, 0x6412C1, &FinalPursuitEndHooked);

	new NyaHookLib::PatchWithUndo(&aChallengeSeriesHooks, NyaHookLib::CALL, 0x6F48DB, &GetChallengeSeriesCarType);
	//new NyaHookLib::PatchWithUndo(&aChallengeSeriesHooks, NyaHookLib::CALL, 0x6F4945, &GetChallengeSeriesCarPerformance);
	new NyaHookLib::PatchWithUndo(&aChallengeSeriesHooks, NyaHookLib::JMP, 0x5FC180, &GetIsChallengeSeriesRace);

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
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x7AE8C1, &GetChallengeSeriesEventIcon1);
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x7AE8DA, &GetChallengeSeriesEventIcon2);
	NyaHookLib::Patch<uint8_t>(0x7AE8C8, 0xEB);
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x7AE8FF, 0x5FAA20);
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x7AE90B, 0x56E010);

	// custom event descriptions
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x7A4369, &GetChallengeSeriesEventDescription1);
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x7A4375, &GetChallengeSeriesEventDescription2);
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x7A437B, &GetChallengeSeriesEventDescription3);

	new NyaHookLib::PatchWithUndo(&aChallengeSeriesHooks, NyaHookLib::CALL, 0x426CA6, &GetNumOpponentsHooked);
	new NyaHookLib::PatchWithUndo(&aChallengeSeriesHooks, NyaHookLib::CALL, 0x431533, &GetNumOpponentsHooked);
	new NyaHookLib::PatchWithUndo(&aChallengeSeriesHooks, NyaHookLib::CALL, 0x611902, &GetNumOpponentsHooked);
	new NyaHookLib::PatchWithUndo(&aChallengeSeriesHooks, NyaHookLib::CALL, 0x61DCB7, &GetNumOpponentsHooked);

	NyaHookLib::Patch<uint16_t>(0x7AE9E6, 0x9090); // don't check unlock states

	// don't spawn boss characters
	new NyaHookLib::PatchWithUndo(&aChallengeSeriesHooks, 0x5FD30C, 2, [](){ NyaHookLib::Patch<uint16_t>(0x5FD30C, 0x9090); });
	new NyaHookLib::PatchWithUndo(&aChallengeSeriesHooks, 0x5FD31A, 2, [](){ NyaHookLib::Patch<uint16_t>(0x5FD31A, 0x9090); });

	// don't sabotage engine
	new NyaHookLib::PatchWithUndo(&aChallengeSeriesHooks, 0x60AB66, 1, [](){ NyaHookLib::Patch<uint8_t>(0x60AB66, 0xEB); });
}