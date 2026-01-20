#include <windows.h>
#include <format>
#include <fstream>

#include "nya_commonhooklib.h"
#include "nya_commonmath.h"
#include "nya_commontimer.h"
#include "nfsmw.h"
#include "chloemenulib.h"

#include "util.h"
#include "timetrials.h"
#include "hooks/carrender.h"

ISimable* VehicleConstructHooked(Sim::Param params) {
	auto vehicle = (VehicleParams*)params.mData;
	if (vehicle->carClass == DRIVER_RACER) {
		// copy player car for all opponents
		auto player = GetLocalPlayerVehicle();
		vehicle->matched = nullptr;
		vehicle->carType = player->GetVehicleKey();
		vehicle->customization = (FECustomizationRecord*)player->GetCustomizations();
	}
	return PVehicle::Construct(params);
}

void DebugMenu() {
	ChloeMenuLib::BeginMenu();

	QuickValueEditor("UnlockAllThings", UnlockAllThings);

	if (TheGameFlowManager.CurrentGameFlowState == GAMEFLOW_STATE_IN_FRONTEND) {
		QuickValueEditor("Replay Viewer Mode", bViewReplayMode);
		QuickValueEditor("Opponent Ghosts Only", bOpponentsOnly);

		if (bViewReplayMode) bOpponentsOnly = false;
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

auto Game_NotifyRaceFinished = (void(*)(ISimable*))0x6119F0;
void OnEventFinished(ISimable* a1) {
	Game_NotifyRaceFinished(a1);

	if (!a1 || a1 == GetLocalPlayerSimable()) {
		DLLDirSetter _setdir;
		OnFinishRace();
	}
}

float TrafficDensityHooked() {
	return 0.0;
}

BOOL WINAPI DllMain(HINSTANCE, DWORD fdwReason, LPVOID) {
	switch( fdwReason ) {
		case DLL_PROCESS_ATTACH: {
			if (NyaHookLib::GetEntryPoint() != 0x3C4040) {
				MessageBoxA(nullptr, "Unsupported game version! Make sure you're using v1.3 (.exe size of 6029312 bytes)", "nya?!~", MB_ICONERROR);
				return TRUE;
			}

			GetCurrentDirectoryW(MAX_PATH, gDLLDir);

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

			NyaHookLib::Fill(0x6876FB, 0x90, 5); // don't run PVehicle::UpdateListing when changing driver class

			ApplyCarRenderHooks();

			ChloeMenuLib::RegisterMenu("Time Trial Ghosts", &DebugMenu);

			WriteLog("Mod initialized");
		} break;
		default:
			break;
	}
	return TRUE;
}