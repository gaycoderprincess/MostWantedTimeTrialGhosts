#include <windows.h>
#include <format>
#include <fstream>

#include "nya_dx9_hookbase.h"
#include "nya_commonhooklib.h"
#include "nya_commonmath.h"
#include "nfsmw.h"
#include "chloemenulib.h"

bool bChallengeSeriesMode = false;

#include "util.h"
#include "d3dhook.h"
#include "timetrials.h"
#include "hooks/carrender.h"
#include "hooks/customevents.h"
#include "hooks/fixes.h"

void SetChallengeSeriesMode(bool on) {
	bChallengeSeriesMode = on;
	if (on) {
		bViewReplayMode = false;
		bOpponentsOnly = true;
		nNitroType = NITRO_ON;
		nSpeedbreakerType = NITRO_ON;
		ApplyCustomEventsHooks();
	}
	else {
		bOpponentsOnly = false;
	}
}

ISimable* VehicleConstructHooked(Sim::Param params) {
	auto vehicle = (VehicleParams*)params.mData;
	if (vehicle->carClass == DRIVER_RACER) {
		// copy player car for all opponents
		auto player = GetLocalPlayerVehicle();
		vehicle->matched = nullptr;
		vehicle->carType = player->GetVehicleKey();
		if (auto customization = player->GetCustomizations()) {
			static FECustomizationRecord record;
			record = *customization;
			// force nos to be enabled for proper playback
			if (vehicle->carType != Attrib::StringHash32("copcross") && record.InstalledPhysics.Part[Physics::Upgrades::PUT_NOS] <= 0) {
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

void DebugMenu() {
	ChloeMenuLib::BeginMenu();

	QuickValueEditor("UnlockAllThings", UnlockAllThings);

	QuickValueEditor("Show Inputs While Driving", bShowInputsWhileDriving);
	QuickValueEditor("Player Name Override", sPlayerNameOverride, 32);

	if (!bChallengeSeriesMode && TheGameFlowManager.CurrentGameFlowState == GAMEFLOW_STATE_IN_FRONTEND) {
		QuickValueEditor("Replay Viewer", bViewReplayMode);
		QuickValueEditor("Opponent Ghosts Only", bOpponentsOnly);

		if (bViewReplayMode) bOpponentsOnly = false;

		if (DrawMenuOption("NOS")) {
			ChloeMenuLib::BeginMenu();
			if (DrawMenuOption("Off")) {
				nNitroType = NITRO_OFF;
			}
			if (DrawMenuOption("On")) {
				nNitroType = NITRO_ON;
			}
			if (DrawMenuOption("Infinite")) {
				nNitroType = NITRO_INF;
			}
			ChloeMenuLib::EndMenu();
		}

		if (DrawMenuOption("Speedbreaker")) {
			ChloeMenuLib::BeginMenu();
			if (DrawMenuOption("Off")) {
				nSpeedbreakerType = NITRO_OFF;
			}
			if (DrawMenuOption("On")) {
				nSpeedbreakerType = NITRO_ON;
			}
			if (DrawMenuOption("Infinite")) {
				nSpeedbreakerType = NITRO_INF;
			}
			ChloeMenuLib::EndMenu();
		}
	}

	if (DrawMenuOption("Ghost Visuals")) {
		ChloeMenuLib::BeginMenu();
		if (DrawMenuOption("Hidden")) {
			nGhostVisuals = GHOST_HIDE;
		}
		if (DrawMenuOption("Shown")) {
			nGhostVisuals = GHOST_SHOW;
		}
		if (DrawMenuOption("Hide Too Close")) {
			nGhostVisuals = GHOST_HIDE_NEARBY;
		}
		ChloeMenuLib::EndMenu();
	}

	auto status = GRaceStatus::fObj;
	if (status && status->mRaceParms) {
		DrawMenuOption(std::format("Race - {}", GRaceParameters::GetEventID(status->mRaceParms)));
		DrawMenuOption(std::format("Context - {}", (int)status->mRaceContext));
	}

	ChloeMenuLib::EndMenu();
}

void MainLoop() {
	if (Sim::Internal::mSystem->mState != Sim::STATE_ACTIVE) return;

	DLLDirSetter _setdir;
	TimeTrialLoop();
}

void RenderLoop() {
	if (TheGameFlowManager.CurrentGameFlowState != GAMEFLOW_STATE_RACING) return;
	if (IsInLoadingScreen() || IsInNIS()) return;
	if (!ShouldGhostRun()) return;

	if (bViewReplayMode) {
		if (PlayerPBGhost.aTicks.size() > nGlobalReplayTimer) {
			DisplayInputs(&PlayerPBGhost.aTicks[nGlobalReplayTimer].inputs);
		}
	}
	else if (bShowInputsWhileDriving) {
		DisplayInputs(GetLocalPlayerInterface<IInput>()->GetControls());
	}
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
	return 0.0;
}

int GetNumOpponentsHooked(GRaceParameters* pThis) {
	auto count = GRaceParameters::GetNumOpponents(pThis);
	if (count < 1) return 1;
	return count;
}

BOOL WINAPI DllMain(HINSTANCE, DWORD fdwReason, LPVOID) {
	switch( fdwReason ) {
		case DLL_PROCESS_ATTACH: {
			if (NyaHookLib::GetEntryPoint() != 0x3C4040) {
				MessageBoxA(nullptr, "Unsupported game version! Make sure you're using v1.3 (.exe size of 6029312 bytes)", "nya?!~", MB_ICONERROR);
				return TRUE;
			}

			GetCurrentDirectoryW(MAX_PATH, gDLLDir);

			NyaHooks::PlaceD3DHooks();
			NyaHooks::D3DEndSceneHook::aFunctions.push_back(D3DHookMain);
			NyaHooks::D3DResetHook::aFunctions.push_back(OnD3DReset);
			NyaHooks::SimServiceHook::Init();
			NyaHooks::SimServiceHook::aFunctions.push_back(MainLoop);
			NyaHooks::LateInitHook::Init();
			NyaHooks::LateInitHook::aFunctions.push_back([]() {
				Scheduler::fgScheduler->fTimeStep = 1.0 / 120.0; // set sim framerate
				*(void**)0x92C534 = (void*)&VehicleConstructHooked;
			});

			// use SuspensionRacer instead of SuspensionSimple for racers - fixes popped tire behavior
			NyaHookLib::Patch(0x6380CB + 1, "SuspensionRacer");
			NyaHookLib::Patch(0x67F353 + 1, "SuspensionRacer");

			Game_NotifyRaceFinished = (void(*)(ISimable*))(*(uint32_t*)(0x61E9CE + 1));
			NyaHookLib::Patch(0x61E9CE + 1, &OnEventFinished);

			NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x439BBD, &TrafficDensityHooked);

			NyaHookLib::Patch<uint8_t>(0x47DDD6, 0xEB); // disable CameraMover::MinGapCars

			// remove career mode and multiplayer
			NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x544FEF, 0x57397D);
			NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x545103, 0x57397D);
			NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x545147, 0x57397D);

			NyaHookLib::Fill(0x6876FB, 0x90, 5); // don't run PVehicle::UpdateListing when changing driver class

			//NyaHookLib::Patch(0x8F5CFC, 0); // tollbooth -> sprint
			NyaHookLib::Patch(0x8F5CF4, 1); // lap knockout -> circuit
			NyaHookLib::Patch(0x8F5D04, 0); // speedtrap -> sprint

			//NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x426CA6, &GetNumOpponentsHooked);
			//NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x431533, &GetNumOpponentsHooked);
			//NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x611902, &GetNumOpponentsHooked);
			//NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x61DCB7, &GetNumOpponentsHooked);

			ApplyCarRenderHooks();
			ApplyGameFixes();

			ChloeMenuLib::RegisterMenu("Time Trial Ghosts", &DebugMenu);

			DoConfigLoad();

#ifdef TIMETRIALS_CHALLENGESERIES
			SetChallengeSeriesMode(true);
			// remove quick race
			NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x54507B, 0x57397D);
#else
			// remove challenge series
			NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x545037, 0x57397D);
#endif

			WriteLog("Mod initialized");
		} break;
		default:
			break;
	}
	return TRUE;
}