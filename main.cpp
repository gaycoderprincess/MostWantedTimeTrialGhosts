#include <windows.h>
#include <format>
#include <fstream>
#include <thread>

#include "nya_dx9_hookbase.h"
#include "nya_commonhooklib.h"
#include "nya_commonmath.h"
#include "nfsmw.h"
#include "chloemenulib.h"

bool bCareerMode = false;
bool bChallengeSeriesMode = false;

#include "util.h"
#include "d3dhook.h"
#include "timetrials.h"
#include "verification.h"
#include "hooks/carrender.h"
#include "hooks/fixes.h"

#include "gamemenu.h"
#include "challengeseries.h"
#include "hooks/customevents.h"

ISimable* VehicleConstructHooked(Sim::Param params) {
	DLLDirSetter _setdir;

	auto vehicle = (VehicleParams*)params.mData;
	if (bChallengeSeriesMode && vehicle->carClass == DRIVER_HUMAN) {
		if (pSelectedEvent->sCarPreset.starts_with("PresetCar")) {
			if (vehicle->carType == Attrib::StringHash32("gti")) {
				vehicle->customization->Tunings[vehicle->customization->ActiveTuning].Value[Physics::Tunings::AERODYNAMICS] = 1; // aerodynamics +5
			}

			static Physics::Info::Performance temp;
			temp.Acceleration = 1;
			temp.TopSpeed = 1;
			temp.Handling = 1;
			vehicle->matched = &temp;
		}
		else if (!FindFEPresetCar(bStringHashUpper(pSelectedEvent->sCarPreset.c_str()))) {
			static Physics::Info::Performance temp;
			temp.Acceleration = 1;
			temp.TopSpeed = 1;
			temp.Handling = 1;
			vehicle->matched = &temp;
			vehicle->carType = Attrib::StringHash32(pSelectedEvent->sCarPreset.c_str());
			vehicle->customization = nullptr;
		}

		if (pSelectedEvent->nLapCountOverride > 0) {
			SetRaceNumLaps(pSelectedEventParams, pSelectedEvent->nLapCountOverride);
		}
	}

	bool replaceRacers = true;
	if (bCareerMode) {
		auto race = GetCurrentRace();
		if (race && (GRaceParameters::GetIsBossRace(race) || GRaceParameters::GetIsDDayRace(race))) {
			replaceRacers = false;
		}
	}

	if (replaceRacers && vehicle->carClass == DRIVER_RACER) {
		// copy player car for all opponents
		auto player = GetLocalPlayerVehicle();
		vehicle->matched = nullptr;
		vehicle->carType = player->GetVehicleKey();
		if (auto customization = player->GetCustomizations()) {
			static FECustomizationRecord record;
			record = *customization;
			// force nos to be enabled for proper playback
			if (vehicle->carType != Attrib::StringHash32("copcross") && vehicle->carType != Attrib::StringHash32("cs_clio_trafpizza") && record.InstalledPhysics.Part[Physics::Upgrades::PUT_NOS] <= 0) {
				record.InstalledPhysics.Part[Physics::Upgrades::PUT_NOS] = 1;
			}
			vehicle->customization = &record;
		}
		else {
			vehicle->customization = nullptr;
		}

		// do a config save in every loading screen
		DoConfigSave();
	}
	return PVehicle::Construct(params);
}

void MainLoop() {
	if (!Sim::Internal::mSystem) return;
	if (Sim::Internal::mSystem->mState != Sim::STATE_ACTIVE) return;

	DLLDirSetter _setdir;
	TimeTrialLoop();
}

void RenderLoop() {
	VerifyTimers();
	TimeTrialRenderLoop();
	g_WorldLodLevel = std::min(g_WorldLodLevel, 2); // force world detail to one lower than max for props
}

auto Game_NotifyRaceFinished = (void(*)(ISimable*))0x6119F0;
void OnEventFinished(ISimable* a1) {
	Game_NotifyRaceFinished(a1);

	if ((!a1 || a1 == GetLocalPlayerSimable()) && !GetLocalPlayerVehicle()->IsDestroyed()) {
		DLLDirSetter _setdir;
		OnFinishRace();

		// do a config save when finishing a race
		DoConfigSave();
	}
}

float TrafficDensityHooked() {
	if (!GRaceStatus::fObj || !GRaceStatus::fObj->mRaceParms) return 1.0;
	return 0.0;
}

auto RaceCountdownHooked_orig = (void(__thiscall*)(void*))nullptr;
void __thiscall RaceCountdownHooked(void* pThis) {
	OnRaceRestart();
	RaceCountdownHooked_orig(pThis);
}

float __thiscall FinishTimeHooked(GTimer* pThis) {
	if (bViewReplayMode) return pThis->GetTime();
	return nLastFinishTime * 0.001;
}

float __thiscall RaceTimeHooked(GTimer* pThis) {
	return (nGlobalReplayTimerNoCountdown / 120.0);
}

float __thiscall GetTimeLimitHooked(GRaceParameters* pThis) {
	return 120;
}

const char* __thiscall GetCareerCarType(GRaceParameters* pThis) {
	return nullptr;
}

void __thiscall GenericResultDrawHooked(ResultStat* pThis) {
	auto racer = pThis->RacerInfo;
	FEPrintf(pThis->pTitle, racer->mName);
	if (racer->mEngineBlown) {
		FEngSetLanguageHash(pThis->pData, 0x1E66364);
	}
	else if (racer->mTotalled) {
		FEngSetLanguageHash(pThis->pData, 0xB7B75185);
	}
	else if (racer->mKnockedOut) {
		FEngSetLanguageHash(pThis->pData, 0x5D82DBA2);
	}
	else if (racer->mFinishedRacing) {
		if (racer->mIndex == 0) {
			auto time = GetTimeFromMilliseconds(nLastFinishTime);
			time.pop_back();
			FEPrintf(pThis->pData, time.c_str());
		}
		else {
			if (auto ghost = GetGhostForOpponent(racer->mIndex - 1)) {
				auto time = GetTimeFromMilliseconds(ghost->nFinishTime);
				time.pop_back();
				FEPrintf(pThis->pData, time.c_str());
			}
			else {
				FEPrintf(pThis->pData, "N/A");
			}
		}
	}
	else {
		FEngSetLanguageHash(pThis->pData, 0xFC1BF40);
	}
	FEPrintf(pThis->Position, "%d", racer->mRanking);
}

BOOL WINAPI DllMain(HINSTANCE, DWORD fdwReason, LPVOID) {
	switch( fdwReason ) {
		case DLL_PROCESS_ATTACH: {
			if (NyaHookLib::GetEntryPoint() != 0x3C4040) {
				MessageBoxA(nullptr, "Unsupported game version! Make sure you're using v1.3 (.exe size of 6029312 bytes)", "nya?!~", MB_ICONERROR);
				return TRUE;
			}

			gDLLPath = std::filesystem::current_path();
			GetCurrentDirectoryW(MAX_PATH, gDLLDir);

			NyaHooks::SimServiceHook::Init();
			NyaHooks::SimServiceHook::aPreFunctions.push_back(MainLoop);
			NyaHooks::LateInitHook::Init();
			NyaHooks::LateInitHook::aPreFunctions.push_back(FileIntegrity::VerifyGameFiles);
			NyaHooks::LateInitHook::aFunctions.push_back([]() {
				NyaHooks::PlaceD3DHooks();
				NyaHooks::D3DEndSceneHook::aPreFunctions.push_back(CollectPlayerPos);
				NyaHooks::D3DEndSceneHook::aFunctions.push_back(D3DHookMain);
				NyaHooks::D3DEndSceneHook::aFunctions.push_back(CheckPlayerPos);
				NyaHooks::D3DResetHook::aFunctions.push_back(OnD3DReset);

				ApplyVerificationPatches();

				Scheduler::fgScheduler->fTimeStep = 1.0 / 120.0; // set sim framerate
				*(void**)0x92C534 = (void*)&VehicleConstructHooked;
				if (GetModuleHandleA("NFSMWLimitAdjuster.asi") || std::filesystem::exists("NFSMWLimitAdjuster.ini")) {
					MessageBoxA(nullptr, "Incompatible mod detected! Please remove NFSMWLimitAdjuster from your game before using this mod.", "nya?!~", MB_ICONERROR);
					exit(0);
				}
				//if (GetModuleHandleA("NFSMWExtraOptions.asi") || std::filesystem::exists("NFSMWExtraOptionsSettings.ini") || std::filesystem::exists("scripts/NFSMWExtraOptionsSettings.ini")) {
				//	MessageBoxA(nullptr, "Potential unfair advantage detected! Please remove NFSMWExtraOptions from your game before using this mod.", "nya?!~", MB_ICONERROR);
				//	exit(0);
				//}
			});

			// use SuspensionRacer instead of SuspensionSimple for racers - fixes popped tire behavior
			NyaHookLib::Patch(0x6380CB + 1, "SuspensionRacer");
			NyaHookLib::Patch(0x67F353 + 1, "SuspensionRacer");

			Game_NotifyRaceFinished = (void(*)(ISimable*))(*(uint32_t*)(0x61E9CE + 1));
			NyaHookLib::Patch(0x61E9CE + 1, &OnEventFinished);

			NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x439BBD, &TrafficDensityHooked);

			NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x5A2CDC, &FinishTimeHooked);
			NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x6F1E00, &RaceTimeHooked);
			NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x5953F0, &GenericResultDrawHooked);
			NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x5FC260, &GetTimeLimitHooked);
			NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x5FE090, &GetTimeLimitHooked);
			NyaHookLib::Patch<uint8_t>(0x6F1DD5, 0xEB); // disable tollbooth time limit display on hud

			NyaHookLib::Patch<uint8_t>(0x47DDD6, 0xEB); // disable CameraMover::MinGapCars

			// remove career mode and multiplayer
			NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x545103, 0x57397D);
			NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x545147, 0x57397D);

			RaceCountdownHooked_orig = (void(__thiscall*)(void*))NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x63B093, &RaceCountdownHooked);

			SetRacerAIEnabled(false);
			//NyaHookLib::Fill(0x6876FB, 0x90, 5); // don't run PVehicle::UpdateListing when changing driver class

#ifdef TIMETRIALS_CAREER
			bCareerMode = true;
			NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x54507B, 0x57397D); // quick race
			NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x545037, 0x57397D); // challenge series

			// change prologue m3 to razor m3
			NyaHookLib::Patch(0x57F60D + 1, "E3_DEMO_BMW");
			NyaHookLib::Patch(0x5A3A7C + 1, "E3_DEMO_BMW");
			NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x6F48DB, &GetCareerCarType);

			// unlock all cars and upgrades
			NyaHookLib::Patch<uint16_t>(0x58A9F8, 0x9090);
			NyaHookLib::Patch<uint16_t>(0x58A8D8, 0x9090);
			NyaHookLib::Patch<uint8_t>(0x576708, 0xEB);
#else
			NyaHookLib::Patch(0x8F5CFC, 0); // tollbooth -> sprint
			NyaHookLib::Patch(0x8F5D04, 0); // speedtrap -> sprint
			NyaHookLib::Patch<uint8_t>(0x60A7B0, 0xC3); // disable speedtrap trigger code
			NyaHookLib::Patch<uint8_t>(0x5F4E50, 0xC3); // remove speedtraps
			NyaHookLib::Patch<uint8_t>(0x5DDCB6, 0xEB); // remove speedtraps

			NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x544FEF, 0x57397D); // career mode

			UnlockAllThings = true;
#endif

			NyaHookLib::Patch(0x8F5CF4, 1); // lap knockout -> circuit

			NyaHookLib::Patch<uint8_t>(0x60A67A, 0xEB); // disable SpawnCop, fixes dday issues
			NyaHookLib::Patch<uint8_t>(0x611440, 0xC3); // disable KnockoutRacer

			NyaHookLib::Patch<uint16_t>(0x7AA753, 0x9090); // remove quickplay

			ApplyCarRenderHooks();
			ApplyGameFixes();

			ChloeMenuLib::RegisterMenu("Time Trial Ghosts", &DebugMenu);

			DoConfigLoad();

			SetupCustomEventsHooks();
			ApplyCustomMenuHooks();

			WriteLog("Mod initialized");
		} break;
		default:
			break;
	}
	return TRUE;
}