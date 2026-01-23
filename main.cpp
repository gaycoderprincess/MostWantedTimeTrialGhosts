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
	if (vehicle->carClass == DRIVER_HUMAN && !FindFEPresetCar(bStringHashUpper(pSelectedEvent->sCarPreset.c_str()))) {
		static Physics::Info::Performance temp;
		temp.Acceleration = 1;
		temp.TopSpeed = 1;
		temp.Handling = 1;
		vehicle->matched = &temp;
		vehicle->carType = Attrib::StringHash32(pSelectedEvent->sCarPreset.c_str());
		vehicle->customization = nullptr;

		if (pSelectedEvent->nLapCountOverride > 0) {
			SetRaceNumLaps(pSelectedEventParams, pSelectedEvent->nLapCountOverride);
		}
	}
	if (vehicle->carClass == DRIVER_RACER) {
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

void DebugMenu() {
	ChloeMenuLib::BeginMenu();

	QuickValueEditor("Show Inputs While Driving", bShowInputsWhileDriving);
	QuickValueEditor("Player Name Override", sPlayerNameOverride, sizeof(sPlayerNameOverride));

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

	if (bChallengeSeriesMode && TheGameFlowManager.CurrentGameFlowState == GAMEFLOW_STATE_IN_FRONTEND) {
		const char* currDifficulty = "NULL";
		switch (nDifficulty) {
			case DIFFICULTY_EASY:
				currDifficulty = "Easy";
				break;
			case DIFFICULTY_NORMAL:
				currDifficulty = "Normal";
				break;
			case DIFFICULTY_HARD:
				currDifficulty = "Hard";
				break;
		}
		if (DrawMenuOption(std::format("Difficulty - {}", currDifficulty))) {
			ChloeMenuLib::BeginMenu();
			if (DrawMenuOption("Easy")) {
				nDifficulty = DIFFICULTY_EASY;
			}
			if (DrawMenuOption("Normal")) {
				nDifficulty = DIFFICULTY_NORMAL;
			}
			if (DrawMenuOption("Hard")) {
				nDifficulty = DIFFICULTY_HARD;
			}
			ChloeMenuLib::EndMenu();
		}
		if (nDifficulty != DIFFICULTY_EASY) {
			QuickValueEditor("Show Target Ghost Only", bChallengesOneGhostOnly);
			QuickValueEditor("Show Personal Ghost", bChallengesPBGhost);
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
	if (!Sim::Internal::mSystem) return;
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
			DisplayInputs(&PlayerPBGhost.aTicks[nGlobalReplayTimer].v1.inputs);
		}
	}
	else if (bShowInputsWhileDriving) {
		DisplayInputs(GetLocalPlayerInterface<IInput>()->GetControls());
	}

	if (bChallengeSeriesMode && nGhostVisuals != GHOST_HIDE && !FEManager::mPauseRequest) {
		const float fPlayerNameOffset = 0.031;
		const float fPlayerNameSize = 0.022;
		const float fPlayerNameFadeStart = 50.0;
		const float fPlayerNameFadeEnd = 150.0;
		const float fPlayerNameAlpha = 200.0;

		for (auto& ghost : OpponentGhosts) {
			auto car = ghost.pLastVehicle;
			if (!IsVehicleValidAndActive(car)) continue;

			UMath::Vector3 dim;
			car->mCOMObject->Find<IRigidBody>()->GetDimension(&dim);
			auto pos = *car->GetPosition();
			pos.y += dim.y;

			auto cam = PrepareCameraMatrix(GetLocalPlayerCamera());
			auto camFwd = RenderToWorldCoords(cam.z);
			auto camPos = RenderToWorldCoords(cam.p);
			auto playerDir = camPos - pos;
			playerDir.Normalize();
			if (playerDir.Dot(camFwd) > 0) continue;

			bVector3 screenPos;
			auto worldPos = WorldToRenderCoords(pos);
			eViewPlatInterface::GetScreenPosition(&eViews[EVIEW_PLAYER1], &screenPos, (bVector3*)&worldPos);

			screenPos.x /= (double)nResX;
			screenPos.y /= (double)nResY;
			//if (screenPos.z <= 0) continue;

			tNyaStringData data;
			data.x = screenPos.x;
			data.y = screenPos.y - fPlayerNameOffset;
			data.size = fPlayerNameSize;
			data.XCenterAlign = true;
			if (screenPos.z > fPlayerNameFadeStart) {
				data.a = std::lerp(fPlayerNameAlpha, 0, (screenPos.z - fPlayerNameFadeStart) / (fPlayerNameFadeEnd - fPlayerNameFadeStart));
			}
			else {
				data.a = fPlayerNameAlpha;
			}

			auto name = ghost.sPlayerName;
			if (ghost.sPlayerName == "Chloe") name = "gaycoderprincess";
			if (ghost.sPlayerName == "ProfileInProces") name = "ProfileInProcess";
			DrawString(data, name);
		}
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

			NyaHookLib::Patch(0x8F5CFC, 0); // tollbooth -> sprint
			NyaHookLib::Patch(0x8F5CF4, 1); // lap knockout -> circuit
			NyaHookLib::Patch(0x8F5D04, 0); // speedtrap -> sprint
			NyaHookLib::Patch<uint8_t>(0x60A7B0, 0xC3); // disable speedtrap trigger code
			NyaHookLib::Patch<uint8_t>(0x5F4E50, 0xC3); // remove speedtraps
			NyaHookLib::Patch<uint8_t>(0x5DDCB6, 0xEB); // remove speedtraps

			NyaHookLib::Patch<uint8_t>(0x60A67A, 0xEB); // disable SpawnCop, fixes dday issues
			NyaHookLib::Patch<uint8_t>(0x611440, 0xC3); // disable KnockoutRacer

			ApplyCarRenderHooks();
			ApplyGameFixes();

			ChloeMenuLib::RegisterMenu("Time Trial Ghosts", &DebugMenu);

			DoConfigLoad();

#ifdef TIMETRIALS_CHALLENGESERIES
			SetChallengeSeriesMode(true);
			// remove quick race
			NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x54507B, 0x57397D);
#else
			UnlockAllThings = true;
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