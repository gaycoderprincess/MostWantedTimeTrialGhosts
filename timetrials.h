#include "compression.h"

const int nLocalReplayVersion = 6;
const int nMaxNumGhostsToCheck = 8;

enum eNitroType {
	NITRO_OFF,
	NITRO_ON,
	NITRO_INF,
};
int nNitroType = NITRO_ON;
int nSpeedbreakerType = NITRO_ON;

bool bTrackReversed = false; // todo

bool bViewReplayMode = false;
bool bViewReplayTargetTime = false;
bool bOpponentsOnly = false;
enum eGhostVisuals {
	GHOST_HIDE,
	GHOST_SHOW,
	GHOST_HIDE_NEARBY,
};
eGhostVisuals nGhostVisuals = GHOST_HIDE_NEARBY;
bool bShowInputsWhileDriving = false;
char sPlayerNameOverride[32] = "";
enum eDifficulty {
	DIFFICULTY_EASY, // slowest ghost only for every track
	DIFFICULTY_NORMAL, // first 3 ghosts in the folder
	DIFFICULTY_HARD, // quickest ghost only for every track
	NUM_DIFFICULTY,
};
eDifficulty nDifficulty = DIFFICULTY_HARD;
bool bChallengesOneGhostOnly = false;
bool bChallengesPBGhost = false;
bool bCheckFileIntegrity = false;

bool bDebugInputsOnly = false;

#ifdef TIMETRIALS_CARBON
struct InputControls {
	float fBanking;
	float fSteering;
	float fOverSteer;
	float fClutch;
	float fStrafeHorizontal;
	float fGas;
	float fBrake;
	float fHandBrake;
	bool fActionButton;
	bool fNOS;
};
InputControls GetPlayerControls(IVehicle* veh) {
	InputControls out;
	auto input = veh->mCOMObject->Find<IInput>();
	memset(&out, 0, sizeof(out));
	out.fSteering = input->GetControlSteering();
	out.fOverSteer = input->GetControlOverSteer();
	out.fGas = input->GetControlGas();
	out.fBrake = input->GetControlBrake();
	out.fHandBrake = input->GetControlHandBrake();
	out.fClutch = input->GetControlClutch();
	out.fNOS = input->GetControlNOS();
	out.fActionButton = GetLocalPlayer()->InGameBreaker();
	return out;
}
#endif

struct tReplayTick {
	struct tTickVersion1 {
		struct {
			UMath::Matrix4 mat;
			UMath::Vector3 vel;
			UMath::Vector3 tvel;
			int gear;
			float nitro;
		} car;
		InputControls inputs;
	} v1;
	struct tTickVersion2 {
		float raceProgress;
	} v2;
	struct tTickVersion3 {
		int driftPoints;
	} v4;

	void Collect(IVehicle* pVehicle) {
		auto rb = pVehicle->mCOMObject->Find<IRigidBody>();
		rb->GetMatrix4(&v1.car.mat);
		v1.car.mat.p = *rb->GetPosition();
		v1.car.vel = *rb->GetLinearVelocity();
		v1.car.tvel = *rb->GetAngularVelocity();
		v1.car.gear = pVehicle->mCOMObject->Find<ITransmission>()->GetGear();
		v1.car.nitro = pVehicle->mCOMObject->Find<IEngine>()->GetNOSCapacity();
#ifdef TIMETRIALS_CARBON
		v1.inputs = GetPlayerControls(pVehicle);

		auto raceType = GRaceParameters::GetRaceType(GRaceStatus::fObj->mRaceParms);
		if (raceType == GRace::kRaceType_DriftRace || raceType == GRace::kRaceType_CanyonDrift) {
			v4.driftPoints = DALRacer::GetDriftScoreReport(nullptr, 0)->totalPoints;
		}
#else
		v1.inputs = *pVehicle->mCOMObject->Find<IInput>()->GetControls();
#endif

		v2.raceProgress = 0;
		if (auto racer = GetRacerInfoFromHandle(pVehicle->mCOMObject->Find<ISimable>()->GetOwnerHandle())) {
			v2.raceProgress = racer->mPctRaceComplete;
		}
	}

	void Apply(IVehicle* pVehicle) {
		auto rb = pVehicle->mCOMObject->Find<IRigidBody>();
		auto trans = pVehicle->mCOMObject->Find<ITransmission>();
		auto engine = pVehicle->mCOMObject->Find<IEngine>();

		if (!bDebugInputsOnly || pVehicle->GetDriverClass() != DRIVER_HUMAN) {
			rb->SetOrientation(&v1.car.mat);
			rb->SetPosition((UMath::Vector3*)&v1.car.mat.p);
			rb->SetLinearVelocity(&v1.car.vel);
			rb->SetAngularVelocity(&v1.car.tvel);

			auto gear = trans->GetGear();
			if (gear != v1.car.gear) {
				trans->Shift((GearID)v1.car.gear);
			}

			engine->ChargeNOS(-engine->GetNOSCapacity());
			engine->ChargeNOS(v1.car.nitro);
		}

#ifdef TIMETRIALS_CARBON
		auto input = pVehicle->mCOMObject->Find<IInput>();
		input->SetControlSteering(v1.inputs.fSteering);
		input->SetControlOverSteer(v1.inputs.fOverSteer);
		input->SetControlGas(v1.inputs.fGas);
		input->SetControlBrake(v1.inputs.fBrake);
		input->SetControlHandBrake(v1.inputs.fHandBrake);
		input->SetControlClutch(v1.inputs.fClutch);
		input->SetControlNOS(v1.inputs.fNOS);
#else
		*pVehicle->mCOMObject->Find<IInput>()->GetControls() = v1.inputs;
#endif

#ifndef TIMETRIALS_CARBON
		if (pVehicle->GetDriverClass() == DRIVER_RACER) {
			pVehicle->SetDriverClass(DRIVER_NONE);
		}
#endif

		if (pVehicle->GetDriverClass() == DRIVER_HUMAN) {
			auto player = GetLocalPlayer();
			if (player->InGameBreaker() != v1.inputs.fActionButton) player->ToggleGameBreaker();
		}
		else {
			pVehicle->mCOMObject->Find<IRBVehicle>()->EnableObjectCollisions(false);

			if (auto racer = GetRacerInfoFromHandle(pVehicle->mCOMObject->Find<ISimable>()->GetOwnerHandle())) {
				racer->mPctRaceComplete = v2.raceProgress;
			}
		}
	}

	void ApplyPhysics(IVehicle* pVehicle) {
		if (bDebugInputsOnly && pVehicle->GetDriverClass() == DRIVER_HUMAN) return;

		auto rb = pVehicle->mCOMObject->Find<IRigidBody>();
		rb->SetOrientation(&v1.car.mat);
		rb->SetPosition((UMath::Vector3*)&v1.car.mat.p);
		rb->SetLinearVelocity(&v1.car.vel);
		rb->SetAngularVelocity(&v1.car.tvel);
	}
};

uint32_t nGlobalReplayTimer = 0;
uint32_t nGlobalReplayTimerNoCountdown = 0;

uint32_t nLocalGameFilesHash = 0;
struct tReplayGhost {
public:
	std::vector<tReplayTick> aTicks;
	uint32_t nFinishTime;
	uint32_t nFinishPoints;
	std::string sPlayerName;
	IVehicle* pLastVehicle;
	bool bHasCountdown;
	bool bHasCountdownFinalChase;
	uint32_t nGameFilesHash;
	bool bIsPersonalBest;

	tReplayGhost() {
		Invalidate();
	}

	bool IsValid() const {
		return nFinishTime != 0 && !aTicks.empty();
	}

	uint32_t GetCurrentTick() const {
		auto hasCountdown = !strcmp(GRaceParameters::GetEventID(GRaceStatus::fObj->mRaceParms), "1.8.1") ? bHasCountdownFinalChase : bHasCountdown;
		if (!hasCountdown) return nGlobalReplayTimerNoCountdown;

		// take countdown time and use globaltimernocountdown to accomodate for it
		// the countdown seems to be inconsistent otherwise
		auto finishTick = (nFinishTime * 120) / 1000;
		auto totalTicks = aTicks.size();
		if (GetLocalPlayerVehicle()->IsStaging() || GetLocalPlayerInterface<IHumanAI>()->GetAiControl()) {
			return nGlobalReplayTimer;
		}
		return nGlobalReplayTimerNoCountdown + (totalTicks - finishTick);
	}

	void Invalidate() {
		aTicks.clear();
		nFinishTime = 0;
		nFinishPoints = 0;
		sPlayerName = "";
		pLastVehicle = nullptr;
		bHasCountdown = true;
		bHasCountdownFinalChase = true;
		nGameFilesHash = 0;
		bIsPersonalBest = false;
	}
};
tReplayGhost PlayerPBGhost;
std::vector<tReplayGhost> OpponentGhosts;

bool bGhostsLoaded = false;
std::vector<tReplayTick> aRecordingTicks;

// if this fails the ghosts are paused
bool ShouldGhostRun() {
	if (IsInNIS()) return false;
	if (!GRaceStatus::fObj) return false;
	if (!GRaceStatus::fObj->mRaceParms) return false;

	auto ply = GetLocalPlayerVehicle();
	if (!ply) return false;
	return true;
}

std::vector<tReplayGhost> aLeaderboardGhosts;
void InvalidateGhost() {
	bGhostsLoaded = false;
	nGlobalReplayTimer = 0;
	nGlobalReplayTimerNoCountdown = 0;
	PlayerPBGhost.Invalidate();
	OpponentGhosts.clear();
	aRecordingTicks.clear();
	aLeaderboardGhosts.clear();
}

void InvalidateLocalGhost() {
	nGlobalReplayTimer = 0;
	nGlobalReplayTimerNoCountdown = 0;
	aRecordingTicks.clear();
}

void RunGhost(IVehicle* veh, tReplayGhost* ghost) {
	ghost->pLastVehicle = veh;

	if (auto racer = GetRacerInfoFromHandle(veh->mCOMObject->Find<ISimable>()->GetOwnerHandle())) {
		SetRacerName(racer, ghost->sPlayerName.empty() ? "RACER" : ghost->sPlayerName.c_str());
	}

	if (!ghost->IsValid()) {
#ifndef TIMETRIALS_CARBON
		if (veh->GetDriverClass() == DRIVER_RACER) veh->SetDriverClass(DRIVER_NONE);
#endif
		veh->mCOMObject->Find<IRBVehicle>()->EnableObjectCollisions(false);
		return;
	}

	auto tick = ghost->GetCurrentTick();
	if (tick >= ghost->aTicks.size()) return;

	ghost->aTicks[tick].Apply(veh);
}

void RecordGhost(IVehicle* veh) {
	switch (nNitroType) {
		case NITRO_OFF:
			veh->mCOMObject->Find<IEngine>()->ChargeNOS(-1);
			break;
		case NITRO_INF:
			veh->mCOMObject->Find<IEngine>()->ChargeNOS(1);
			break;
	}
	switch (nSpeedbreakerType) {
		case NITRO_OFF:
			veh->mCOMObject->Find<IPlayer>()->ResetGameBreaker(false);
			break;
		case NITRO_INF:
			veh->mCOMObject->Find<IPlayer>()->ChargeGameBreaker(1);
			break;
	}

	if (sPlayerNameOverride[0]) {
		if (auto racer = GetRacerInfoFromHandle(GetLocalPlayerSimable()->GetOwnerHandle())) {
			SetRacerName(racer, sPlayerNameOverride);
		}
	}

	tReplayTick state;
	state.Collect(veh);
	aRecordingTicks.push_back(state);
}

std::string GetGhostFilename(const std::string& car, const std::string& track, int lapCount, int opponentId, const GameCustomizationRecord* upgrades, const char* folder = nullptr) {
	std::string path = gDLLPath.string();
	path += "/CwoeeGhosts/";
	if (bChallengeSeriesMode) {
		path += opponentId == 0 && !folder ? "ChallengePBs/" : "Challenges/";
	}
	else {
		path += "Practice/";
	}
	if (folder) {
		path += std::format("{}/", folder);
	}
	path += std::format("{}_{}", track, car);
	if (lapCount > 1) {
		path += std::format("_lap{}", lapCount);
	}

	switch (nNitroType) {
		case NITRO_OFF:
			path += "_nos0";
			break;
		case NITRO_INF:
			path += "_nosinf";
			break;
		default:
			break;
	}
	switch (nSpeedbreakerType) {
		case NITRO_OFF:
			path += "_spdbrk0";
			break;
		case NITRO_INF:
			path += "_spdbrkinf";
			break;
		default:
			break;
	}

	// read tunings
#ifndef TIMETRIALS_CARBON
	if (!bChallengeSeriesMode) {
		path += "_up";
		for (int i = 0; i < Physics::Upgrades::PUT_MAX; i++) {
			int level = 0;
			if (upgrades) level = upgrades->InstalledPhysics.Part[i];
			path += std::format("{}", level);
		}
		path += std::format("_{:08X}", upgrades ? upgrades->InstalledPhysics.Junkman : 0);
		path += "_tune";
		for (int i = 0; i < Physics::Tunings::MAX_TUNINGS; i++) {
			int level = 0;
			if (upgrades) {
				auto tune = &upgrades->Tunings[upgrades->ActiveTuning];
				level = tune->Value[i] * 5;
			}
			path += std::format("{}", level);
		}
	}
#endif

	if (bTrackReversed) {
		path += "_rev";
	}

	if (opponentId) path += "_" + std::to_string(opponentId);
	else path += "_pb";

#ifdef TIMETRIALS_CARBON
	path += ".carbon";
#else
	path += ".mw";
#endif
	path += "rep";
	return path;
}

void SavePB(tReplayGhost* ghost, const std::string& car, const std::string& track, int lapCount, const GameCustomizationRecord* upgrades) {
	std::filesystem::create_directory("CwoeeGhosts");
	std::filesystem::create_directory("CwoeeGhosts/ChallengePBs");
	std::filesystem::create_directory("CwoeeGhosts/Practice");

	auto fileName = GetGhostFilename(car, track, lapCount, 0, upgrades);
	auto outFile = CwoeeOStream();

	char signature[4] = "nya";
	int fileVersion = nLocalReplayVersion;

	size_t size = sizeof(tReplayTick);
	outFile.write(signature, sizeof(signature));
	outFile.write((char*)&fileVersion, sizeof(fileVersion));
	outFile.write((char*)&size, sizeof(size));
	WriteStringToFile(outFile, car.c_str());
	WriteStringToFile(outFile, track.c_str());
	outFile.write((char*)&ghost->nFinishTime, sizeof(ghost->nFinishTime));
	outFile.write((char*)&ghost->nFinishPoints, sizeof(ghost->nFinishPoints));
	outFile.write((char*)&nNitroType, sizeof(nNitroType));
	outFile.write((char*)&nSpeedbreakerType, sizeof(nSpeedbreakerType));
	outFile.write((char*)&lapCount, sizeof(lapCount));
#ifndef TIMETRIALS_CARBON
	if (upgrades) {
		outFile.write((char*)&upgrades->InstalledPhysics, sizeof(upgrades->InstalledPhysics));
		outFile.write((char*)&upgrades->Tunings[upgrades->ActiveTuning], sizeof(upgrades->Tunings[upgrades->ActiveTuning]));
	}
	else
#endif
	{
		uint8_t tmp1[sizeof(Physics::Upgrades::Package)] = {};
		uint8_t tmp2[sizeof(Physics::Tunings)] = {};
		outFile.write((char*)tmp1, sizeof(tmp1));
		outFile.write((char*)tmp2, sizeof(tmp2));
	}
	auto name = GetLocalPlayerName();
	if (sPlayerNameOverride[0]) name = sPlayerNameOverride;
	outFile.write(name, 32);
	outFile.write((char*)&nLocalGameFilesHash, sizeof(nLocalGameFilesHash));
	int count = ghost->aTicks.size();
	outFile.write((char*)&count, sizeof(count));
	outFile.write((char*)&ghost->aTicks[0], sizeof(ghost->aTicks[0]) * count);

	if (!WriteCompressedPB(&outFile, fileName)) {
		WriteLog("Failed to save " + fileName + "!");
		return;
	}
}

void LoadPB(tReplayGhost* ghost, const std::string& car, const std::string& track, int lapCount, int opponentId, const GameCustomizationRecord* upgrades, const char* folder = nullptr) {
	ghost->aTicks.clear();
	ghost->nFinishTime = 0;
	ghost->nFinishPoints = 0;

	auto fileName = GetGhostFilename(car, track, lapCount, opponentId, upgrades, folder);
#ifdef TIMETRIALS_COMPRESS_EXISTING
	if (std::filesystem::exists(fileName)) {
		if (CompressPB(fileName)) {
			std::filesystem::remove(fileName);
		}
	}

	CwoeeIStream* decompress = nullptr;

	auto newFileName = fileName + "2";
	if (std::filesystem::exists(newFileName)) {
		decompress = DecompressPB(newFileName);
		if (!decompress) {
			if (TheGameFlowManager.CurrentGameFlowState > GAMEFLOW_STATE_IN_FRONTEND) {
				WriteLog("Invalid ghost for " + fileName);
			}
			return;
		}
	}
	else {
		if (TheGameFlowManager.CurrentGameFlowState > GAMEFLOW_STATE_IN_FRONTEND) {
			WriteLog("No ghost found for " + fileName);
		}
		return;
	}
#else
	CwoeeIStream* decompress = nullptr;

	auto newFileName = fileName + "2";
	if (std::filesystem::exists(newFileName)) {
		decompress = OpenCompressedPB(newFileName);
		if (!decompress) {
			if (TheGameFlowManager.CurrentGameFlowState > GAMEFLOW_STATE_IN_FRONTEND) {
				WriteLog("Invalid ghost for " + fileName);
			}
			return;
		}
	}
	else {
		if (std::filesystem::exists(fileName)) {
			decompress = OpenRawPB(fileName);
			if (!decompress) {
				if (TheGameFlowManager.CurrentGameFlowState > GAMEFLOW_STATE_IN_FRONTEND) {
					WriteLog("Invalid ghost for " + fileName);
				}
				return;
			}
		}
		else {
			if (TheGameFlowManager.CurrentGameFlowState > GAMEFLOW_STATE_IN_FRONTEND) {
				WriteLog("No ghost found for " + fileName);
			}
			return;
		}
	}
#endif

	auto& inFile = *decompress;

	class temp {
	public:
		CwoeeIStream* data;

		temp(CwoeeIStream* a) : data(a) {}
		~temp() { delete data; }
	} dtor(decompress);

	size_t tmpsize;
	int fileVersion;

	uint32_t signature = 0;
	inFile.read((char*)&signature, sizeof(signature));
	if (memcmp(&signature, "nya", 4) == 0) {
		inFile.read((char*)&fileVersion, sizeof(fileVersion));
		inFile.read((char*)&tmpsize, sizeof(tmpsize));
	}
	else {
		WriteLog("Invalid ghost " + fileName);
		return;
	}

	if (fileVersion > nLocalReplayVersion) {
		WriteLog(std::format("Too new ghost for {} (version {})", fileName, fileVersion));
		return;
	}

	Physics::Upgrades::Package playerPhysics = {};
	Physics::Tunings playerTuning = {};
#ifndef TIMETRIALS_CARBON
	if (upgrades) {
		playerPhysics = upgrades->InstalledPhysics;
		playerTuning = upgrades->Tunings[upgrades->ActiveTuning];
	}
#endif

	int tmptime, tmppoints, tmpnitro, tmpspdbrk, tmplaps;
	uint32_t fileHash = 0;
	Physics::Upgrades::Package tmpphysics;
	Physics::Tunings tmptuning;
	char tmpplayername[32];
	strcpy_s(tmpplayername, opponentId ? "OPPONENT GHOST" : "PB GHOST");
	auto tmpcar = ReadStringFromFile(inFile);
	auto tmptrack = ReadStringFromFile(inFile);
	inFile.read((char*)&tmptime, sizeof(tmptime));
	if (fileVersion >= 4) {
		inFile.read((char*)&tmppoints, sizeof(tmppoints));
	}
	else {
		tmppoints = 0;
	}
	inFile.read((char*)&tmpnitro, sizeof(tmpnitro));
	inFile.read((char*)&tmpspdbrk, sizeof(tmpspdbrk));
	inFile.read((char*)&tmplaps, sizeof(tmplaps));
	inFile.read((char*)&tmpphysics, sizeof(tmpphysics));
	inFile.read((char*)&tmptuning, sizeof(tmptuning));
	inFile.read(tmpplayername, sizeof(tmpplayername));
	if (fileVersion >= 5) {
		inFile.read((char*)&fileHash, sizeof(fileHash));
		if (!fileHash) fileHash = 0xFFFFFFFF;
	}
	//if (tmpsize != sizeof(tReplayTick)) {
	//	WriteLog("Outdated ghost for " + fileName);
	//	return;
	//}
	if (tmpcar != car || tmptrack != track) {
		WriteLog("Mismatched ghost for " + fileName);
		return;
	}
	if (tmplaps != lapCount || tmpnitro != nNitroType || tmpspdbrk != nSpeedbreakerType) {
		WriteLog("Mismatched ghost for " + fileName);
		return;
	}
	if (upgrades && !bChallengeSeriesMode) {
		if (memcmp(&playerPhysics, &tmpphysics, sizeof(playerPhysics)) != 0 || memcmp(&playerTuning, &tmptuning, sizeof(playerTuning)) != 0) {
			WriteLog("Mismatched ghost for " + fileName);
			return;
		}
	}
	int count = 0;
	inFile.read((char*)&count, sizeof(count));
	if (count <= 100) {
		WriteLog("Invalid ghost length for " + fileName);
		return;
	}
	// hack for my player name
	if (bChallengeSeriesMode && opponentId > 0) {
		if (!strcmp(tmpplayername, "woof")) strcpy_s(tmpplayername, "Chloe");
	}
	if (folder) {
		strcpy_s(tmpplayername, folder);
	}
	if (TheGameFlowManager.CurrentGameFlowState <= GAMEFLOW_STATE_IN_FRONTEND) {
		tmpplayername[31] = 0;
	}
	else {
		tmpplayername[15] = 0; // max 16 characters, even though the buffer is 32 (any higher and the racer names glitch out)
	}
	ghost->sPlayerName = tmpplayername;
	ghost->nFinishTime = tmptime;
	ghost->nFinishPoints = tmppoints;
	ghost->bHasCountdown = fileVersion >= 3;
	ghost->bHasCountdownFinalChase = fileVersion >= 6;
	ghost->nGameFilesHash = fileHash;

	// don't needlessly load the full ghost data when previewing times in the menu
	if (TheGameFlowManager.CurrentGameFlowState <= GAMEFLOW_STATE_IN_FRONTEND) return;

	ghost->aTicks.reserve(count);
	for (int i = 0; i < count; i++) {
		tReplayTick state = {};
		inFile.read((char*)&state.v1, sizeof(state.v1));
		if (fileVersion >= 2) {
			inFile.read((char*)&state.v2, sizeof(state.v2));
		}
		else {
			// basic fallback by calculating replay progression
			state.v2.raceProgress = (i / (double)count) * 100;
		}
		if (fileVersion >= 4) {
			inFile.read((char*)&state.v4, sizeof(state.v4));
		}
		ghost->aTicks.push_back(state);
	}
}

void OnChallengeSeriesEventPB();

uint32_t nLastFinishTime = 0;
void OnFinishRace() {
	auto ghost = &PlayerPBGhost;

	uint32_t replayTime = nLastFinishTime = (nGlobalReplayTimerNoCountdown / 120.0) * 1000;
	uint32_t replayPoints = 0;
#ifdef TIMETRIALS_CARBON
	auto raceType = GRaceParameters::GetRaceType(GRaceStatus::fObj->mRaceParms);
	bool isDrift = raceType == GRace::kRaceType_DriftRace || raceType == GRace::kRaceType_CanyonDrift;
	if (isDrift) {
		replayPoints = DALRacer::GetDriftScoreReport(nullptr, 0)->totalPoints;
	}
#else
	bool isDrift = false;
#endif
	if (!bViewReplayMode && replayTime > 1000) {
		bool isBetter = !ghost->nFinishTime || replayTime < ghost->nFinishTime;
		if (isDrift) {
			isBetter = !ghost->nFinishTime || replayPoints > ghost->nFinishPoints;
		}
		if (isBetter) {
			WriteLog(std::format("Saving new lap PB of {}ms {}pts", replayTime, replayPoints));
			ghost->aTicks = aRecordingTicks;
			ghost->nFinishTime = replayTime;
			ghost->nFinishPoints = replayPoints;
			ghost->bHasCountdown = true;
			ghost->bHasCountdownFinalChase = true;

			auto car = GetLocalPlayerVehicle();
			SavePB(ghost, car->GetVehicleName(), GRaceParameters::GetEventID(GRaceStatus::fObj->mRaceParms), GetRaceNumLaps(), car->GetCustomizations());

			// invalidate all ghosts to make sure the pb is re-read for the leaderboard
			InvalidateGhost();

#ifdef TIMETRIALS_CHALLENGESERIES
			OnChallengeSeriesEventPB();
#endif
		}
		//if (!ghost->nCurrentSessionPBTime || replayTime < ghost->nCurrentSessionPBTime) {
		//	ghost->nCurrentSessionPBTime = replayTime;
		//	ghost->fCurrentSessionTextHighlightTime = 5;
		//}
	}
	aRecordingTicks.clear();
}

std::vector<tReplayGhost> CollectReplayGhosts(const std::string& car, const std::string& track, int laps, const GameCustomizationRecord* upgrades, bool forFullLeaderboard = false) {
	std::vector<tReplayGhost> ghosts;

	auto difficulty = nDifficulty;
	if (forFullLeaderboard) difficulty = DIFFICULTY_HARD;

	if (difficulty != DIFFICULTY_NORMAL && std::filesystem::exists(gDLLPath.string() + "/CwoeeGhosts/Challenges")) {
		// check all subdirectories for community ghosts
		std::vector<std::string> folders;
		for (const auto& entry : std::filesystem::directory_iterator(gDLLPath.string() + "/CwoeeGhosts/Challenges")) {
			if (!entry.is_directory()) continue;

			folders.push_back(entry.path().filename().string());
		}
		for (auto& folder : folders) {
			tReplayGhost temp;
			LoadPB(&temp, car, track, laps, 0, upgrades, folder.c_str());
			if (!temp.nFinishTime) continue;
			ghosts.push_back(temp);
		}
	}

	if (forFullLeaderboard || (bChallengesPBGhost && !bViewReplayMode)) {
		tReplayGhost temp;
		LoadPB(&temp, car, track, laps, 0, upgrades);
		temp.bIsPersonalBest = true;
		if (temp.nFinishTime) {
			ghosts.push_back(temp);
		}
	}

	for (int i = 0; i < nMaxNumGhostsToCheck; i++) {
		tReplayGhost temp;
		LoadPB(&temp, car, track, laps, i+1, upgrades);
		if (!temp.nFinishTime) continue;
		ghosts.push_back(temp);
	}

	if (difficulty == DIFFICULTY_EASY) {
		std::sort(ghosts.begin(), ghosts.end(), [](const tReplayGhost& a, const tReplayGhost& b) { if (a.nFinishPoints && b.nFinishPoints) { return a.nFinishPoints < b.nFinishPoints; } return a.nFinishTime > b.nFinishTime; });
	}
	else {
		std::sort(ghosts.begin(), ghosts.end(), [](const tReplayGhost& a, const tReplayGhost& b) { if (a.nFinishPoints && b.nFinishPoints) { return a.nFinishPoints > b.nFinishPoints; } return a.nFinishTime < b.nFinishTime; });
	}

	return ghosts;
}

tReplayGhost SelectTopGhost(const std::string& car, const std::string& track, int laps, const GameCustomizationRecord* upgrades) {
	auto ghosts = CollectReplayGhosts(car, track, laps, upgrades);
	if (ghosts.empty()) return {};
	return ghosts[0];
}

void OnRaceRestart() {
	InvalidateLocalGhost();
}

tReplayGhost* GetViewReplayGhost() {
	if (bChallengeSeriesMode && bViewReplayTargetTime && !OpponentGhosts.empty()) return &OpponentGhosts[0];
	return &PlayerPBGhost;
}

void InvalidatePlayerPos();

void TimeTrialLoop() {
	if (TheGameFlowManager.CurrentGameFlowState != GAMEFLOW_STATE_RACING) {
		InvalidateGhost();
		return;
	}

	if (IsInLoadingScreen()) {
		auto cars = VEHICLE_LIST::GetList(VEHICLE_AIRACERS);
		for (int i = 0; i < cars.size(); i++) {
			auto veh = cars[i];
			veh->mCOMObject->Find<IRBVehicle>()->EnableObjectCollisions(false);
		}

		InvalidateLocalGhost();
		return;
	}

	auto ply = GetLocalPlayerVehicle();
	if (!bGhostsLoaded) {
		auto car = ply->GetVehicleName();
		auto track = GRaceParameters::GetEventID(GRaceStatus::fObj->mRaceParms);
		auto laps = GetRaceNumLaps();
		auto upgrades = ply->GetCustomizations();
		LoadPB(&PlayerPBGhost, car, track, laps, 0, upgrades);

		OpponentGhosts.clear();

		if (bChallengeSeriesMode) {
			aLeaderboardGhosts = CollectReplayGhosts(car, track, laps, upgrades, true);

			if (bChallengesOneGhostOnly || bViewReplayMode || nDifficulty == DIFFICULTY_EASY) {
				auto opponent = SelectTopGhost(car, track, laps, upgrades);
				if (opponent.nFinishTime != 0) {
					OpponentGhosts.push_back(opponent);
				}
			}
			else {
				auto ghosts = CollectReplayGhosts(car, track, laps, upgrades);

				int opponentCount = VEHICLE_LIST::GetList(VEHICLE_AIRACERS).size();
				for (int i = 0; i < opponentCount && i < ghosts.size(); i++) {
					OpponentGhosts.push_back(ghosts[i]);
				}
			}
		}
		else {
			int opponentCount = VEHICLE_LIST::GetList(VEHICLE_AIRACERS).size();
			if (!bOpponentsOnly) opponentCount--;
			for (int i = 0; i < opponentCount; i++) {
				OpponentGhosts.push_back({});
				LoadPB(&OpponentGhosts[i], car, track, laps, i+1, upgrades);
			}
		}

		bGhostsLoaded = true;
	}

	if (!ShouldGhostRun()) return;

	ICopMgr::mDisableCops = !GRaceParameters::GetIsPursuitRace(GRaceStatus::fObj->mRaceParms);
	if (!ICopMgr::mDisableCops && bViewReplayMode) {
		auto cars = GetActiveVehicles(DRIVER_COP);
		for (auto& car : cars) {
			car->mCOMObject->Find<ISimable>()->Kill();
		}
	}

#ifdef TIMETRIALS_CARBON
	auto raceType = GRaceParameters::GetRaceType(GRaceStatus::fObj->mRaceParms);
	bool isDrift = raceType == GRace::kRaceType_DriftRace || raceType == GRace::kRaceType_CanyonDrift;
	bool isPlayerDrifting = GetLocalPlayerVehicle()->GetDriverStyle() == STYLE_DRIFT;
	if (isDrift != isPlayerDrifting) {
		GetLocalPlayerVehicle()->SetDriverStyle(isDrift ? STYLE_DRIFT : STYLE_RACING);
	}
#else
	GetUserProfile()->TheOptionsSettings.TheGameplaySettings.JumpCam = false;
	for (int i = 0; i < GRaceStatus::fObj->mRacerCount; i++) {
		auto racer = &GRaceStatus::fObj->mRacerInfo[i];
		racer->mSpeedTrapsCrossed = 0;
		for (auto& speed : racer->mSpeedTrapSpeed) { speed = 0; }
	}
#endif

#ifdef TIMETRIALS_CARBON
	if (ply->IsStaging()) {
#else
	if (ply->IsStaging() || ply->mCOMObject->Find<IHumanAI>()->GetAiControl()) {
#endif
		nGlobalReplayTimerNoCountdown = 0;
	}

	if (bViewReplayMode) {
		RunGhost(ply, GetViewReplayGhost());
	}
	else {
		// set fixed start points, super ultra hack
#ifdef TIMETRIALS_CARBON
		if (GetLocalPlayerVehicle()->IsStaging() && !OpponentGhosts.empty() && OpponentGhosts[0].IsValid()) {
			InvalidatePlayerPos();
			OpponentGhosts[0].aTicks[0].ApplyPhysics(GetLocalPlayerVehicle());
		}
#endif

		auto opponents = VEHICLE_LIST::GetList(VEHICLE_AIRACERS);
		if (bOpponentsOnly) {
			for (int i = 0; i < opponents.size() && i < OpponentGhosts.size(); i++) {
				RunGhost(opponents[i], &OpponentGhosts[i]);
			}
		}
		else {
			for (int i = 0; i < opponents.size(); i++) {
				auto ghost = i == 0 ? &PlayerPBGhost : &OpponentGhosts[i-1];
				RunGhost(opponents[i], ghost);
			}
		}
	}

	RecordGhost(ply);
	if (!ply->IsStaging() || ply->mCOMObject->Find<IHumanAI>()->GetAiControl()) {
		nGlobalReplayTimerNoCountdown++;
	}
	nGlobalReplayTimer++;
}

auto gInputRGBBackground = NyaDrawing::CNyaRGBA32(215,215,215,255);
auto gInputRGBHighlight = NyaDrawing::CNyaRGBA32(0,255,0,255);
float fInputBaseXPosition = 0.45;
float fInputBaseYPosition = 0.85;

void DrawInputTriangle(float posX, float posY, float sizeX, float sizeY, float inputValue, bool invertValue) {
	float minX = std::min(posX - sizeX, posX + sizeX);
	float maxX = std::max(posX - sizeX, posX + sizeX);

	DrawTriangle(posX, posY - sizeY, posX - sizeX, std::lerp(posY - sizeY, posY + sizeY, 0.5), posX,
				 posY + sizeY, invertValue ? gInputRGBBackground : gInputRGBHighlight);

	DrawTriangle(posX, posY - sizeY, posX - sizeX, std::lerp(posY - sizeY, posY + sizeY, 0.5), posX,
				 posY + sizeY, invertValue ? gInputRGBHighlight : gInputRGBBackground, std::lerp(minX, maxX, inputValue), 0, 1, 1);
}

void DrawInputTriangleY(float posX, float posY, float sizeX, float sizeY, float inputValue, bool invertValue) {
	float minY = std::min(posY - sizeY, posY + sizeY);
	float maxY = std::max(posY - sizeY, posY + sizeY);

	DrawTriangle(std::lerp(posX - sizeX, posX + sizeX, 0.5), posY - sizeY, posX - sizeX, posY + sizeY, posX + sizeX,
				 posY + sizeY, invertValue ? gInputRGBBackground : gInputRGBHighlight);

	DrawTriangle(std::lerp(posX - sizeX, posX + sizeX, 0.5), posY - sizeY, posX - sizeX, posY + sizeY, posX + sizeX,
				 posY + sizeY, invertValue ? gInputRGBHighlight : gInputRGBBackground, 0, std::lerp(minY, maxY + 0.001, inputValue), 1, 1);
}

void DrawInputRectangle(float posX, float posY, float scaleX, float scaleY, float inputValue) {
	DrawRectangle(posX - scaleX, posX + scaleX, posY - scaleY, posY + scaleY, gInputRGBBackground);
	DrawRectangle(posX - scaleX, posX + scaleX, std::lerp(posY + scaleY, posY - scaleY, inputValue), posY + scaleY, gInputRGBHighlight);
}

void DisplayInputs(InputControls* inputs) {
	DrawInputTriangle((fInputBaseXPosition - 0.005) * GetAspectRatioInv(), fInputBaseYPosition, 0.08 * GetAspectRatioInv(), 0.07, 1 - (-inputs->fSteering), true);
	DrawInputTriangle((fInputBaseXPosition + 0.08) * GetAspectRatioInv(), fInputBaseYPosition, -0.08 * GetAspectRatioInv(), 0.07, inputs->fSteering, false);
	DrawInputTriangleY((fInputBaseXPosition + 0.0375) * GetAspectRatioInv(), fInputBaseYPosition - 0.05, 0.035 * GetAspectRatioInv(), 0.045, 1 - inputs->fGas, true);
	DrawInputTriangleY((fInputBaseXPosition + 0.0375) * GetAspectRatioInv(), fInputBaseYPosition + 0.05, 0.035 * GetAspectRatioInv(), -0.045, inputs->fBrake, false);

	//DrawInputTriangleY((fInputBaseXPosition + 0.225) * GetAspectRatioInv(), fInputBaseYPosition - 0.04, 0.035 * GetAspectRatioInv(), 0.035, 1 - (inputs->keys[INPUT_GEAR_UP] / 128.0), true);
	//DrawInputTriangleY((fInputBaseXPosition + 0.225) * GetAspectRatioInv(), fInputBaseYPosition + 0.04, 0.035 * GetAspectRatioInv(), -0.035, inputs->keys[INPUT_GEAR_DOWN] / 128.0, false);

	DrawInputRectangle((fInputBaseXPosition + 0.325) * GetAspectRatioInv(), fInputBaseYPosition + 0.05, 0.03 * GetAspectRatioInv(), 0.03, inputs->fNOS);
	DrawInputRectangle((fInputBaseXPosition + 0.425) * GetAspectRatioInv(), fInputBaseYPosition + 0.05, 0.03 * GetAspectRatioInv(), 0.03, inputs->fHandBrake);
	DrawInputRectangle((fInputBaseXPosition + 0.525) * GetAspectRatioInv(), fInputBaseYPosition + 0.05, 0.03 * GetAspectRatioInv(), 0.03, inputs->fActionButton);
}

// special cases for some player names that are above 16 chars
std::string GetRealPlayerName(const std::string& ghostName) {
	if (ghostName == "Chloe") return "gaycoderprincess";
	if (ghostName == "ProfileInProces") return "ProfileInProcess";
	return ghostName;
}

float fLeaderboardX = 0.03;
float fLeaderboardY = 0.6;
float fLeaderboardYSpacing = 0.03;
float fLeaderboardSize = 0.03;
float fLeaderboardOutlineSize = 0.02;
void DisplayLeaderboard() {
	if (bChallengeSeriesMode && GetLocalPlayerVehicle()->IsStaging() || GetLocalPlayerInterface<IHumanAI>()->GetAiControl() || GetIsGamePaused()) {
		std::vector<std::string> uniquePlayers;

		int numOnLeaderboard = 0;
		for (auto& ghost : aLeaderboardGhosts) {
			auto name = ghost.bIsPersonalBest ? ghost.sPlayerName : GetRealPlayerName(ghost.sPlayerName);
			if (std::find(uniquePlayers.begin(), uniquePlayers.end(), name) != uniquePlayers.end()) continue;
			uniquePlayers.push_back(name);
			numOnLeaderboard++;
		}
		uniquePlayers.clear();

		int ranking = 1;

		tNyaStringData data;
		data.x = fLeaderboardX * GetAspectRatioInv();
		data.y = fLeaderboardY - (numOnLeaderboard * fLeaderboardYSpacing);
		data.size = fLeaderboardSize;
		data.outlinea = 255;
		data.outlinedist = fLeaderboardOutlineSize;
		for (auto& ghost : aLeaderboardGhosts) {
			auto name = ghost.bIsPersonalBest ? ghost.sPlayerName : GetRealPlayerName(ghost.sPlayerName);
			if (std::find(uniquePlayers.begin(), uniquePlayers.end(), name) != uniquePlayers.end()) continue;
			uniquePlayers.push_back(name);

			if (ghost.bIsPersonalBest) {
#ifdef TIMETRIALS_CARBON
				data.SetColor(126, 246, 240, 255);
#else
				data.SetColor(245, 185, 110, 255);
#endif
			}
			else {
				data.SetColor(255, 255, 255, 255);
			}

#ifdef TIMETRIALS_CARBON
			auto raceType = GRaceParameters::GetRaceType(GRaceStatus::fObj->mRaceParms);
			bool isDrift = raceType == GRace::kRaceType_DriftRace || raceType == GRace::kRaceType_CanyonDrift;
#else
			bool isDrift = false;
#endif
			std::string str;
			if (isDrift) {
				str = std::format("{}. {} - {}", ranking++, name, ghost.nFinishPoints);
			}
			else {
				auto time = GetTimeFromMilliseconds(ghost.nFinishTime);
				time.pop_back();
				str = std::format("{}. {} - {}", ranking++, name, time);
			}
			if (bCheckFileIntegrity) {
				if (!ghost.nGameFilesHash) {
					str += " (Old ghost, no game data info)";
				}
				else if (ghost.nGameFilesHash != nLocalGameFilesHash) {
					str += std::format(" (Game data mismatch, {:X})", ghost.nGameFilesHash);
				}
			}
			DrawString(data, str);
			data.y += fLeaderboardYSpacing;
		}
	}
}

void DisplayPlayerNames() {
	if (bChallengeSeriesMode && nGhostVisuals != GHOST_HIDE && !GetIsGamePaused()) {
		const float fPlayerNameOffset = 0.031;
		const float fPlayerNameSize = 0.022;
		const float fPlayerNameFadeStart = 50.0;
		const float fPlayerNameFadeEnd = 150.0;
		const float fPlayerNameAlpha = 200.0;

		for (auto& ghost : OpponentGhosts) {
			auto car = ghost.pLastVehicle;
			if (!IsVehicleValidAndActive(car)) continue;

			auto name = ghost.bIsPersonalBest ? ghost.sPlayerName : GetRealPlayerName(ghost.sPlayerName);

			UMath::Vector3 dim;
			car->mCOMObject->Find<IRigidBody>()->GetDimension(&dim);
			auto pos = *car->GetPosition();
			pos.y += dim.y;

			auto cam = PrepareCameraMatrix(GetLocalPlayerCamera());
			auto camFwd = RenderToWorldCoords(cam.z);
			auto camPos = RenderToWorldCoords(cam.p);
			auto playerDir = camPos - pos;
			auto cameraDist = playerDir.length();
			playerDir.Normalize();
			if (playerDir.Dot(camFwd) > 0) continue;
			if (cameraDist > fPlayerNameFadeEnd) continue;

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
			if (cameraDist > fPlayerNameFadeStart) {
				data.a = std::lerp(fPlayerNameAlpha, 0, (cameraDist - fPlayerNameFadeStart) / (fPlayerNameFadeEnd - fPlayerNameFadeStart));
			}
			else {
				data.a = fPlayerNameAlpha;
			}
			DrawString(data, name);
		}
	}
}

void TimeTrialRenderLoop() {
	if (TheGameFlowManager.CurrentGameFlowState != GAMEFLOW_STATE_RACING) {
		InvalidateGhost();
		return;
	}
	if (IsInLoadingScreen()) return;

	DisplayLeaderboard();

	if (!ShouldGhostRun()) return;

#ifdef TIMETRIALS_CARBON
	if (bViewReplayMode) {
		auto ghost = GetViewReplayGhost();

		auto tick = ghost->GetCurrentTick();
		if (ghost->aTicks.size() > tick) {
			DisplayInputs(&ghost->aTicks[tick].v1.inputs);
		}
	}
	else if (bShowInputsWhileDriving) {
		auto inputs = GetPlayerControls(GetLocalPlayerVehicle());
		DisplayInputs(&inputs);
	}
#else
	if (bViewReplayMode) {
		auto ghost = GetViewReplayGhost();

		auto tick = ghost->GetCurrentTick();
		if (ghost->aTicks.size() > tick) {
			DisplayInputs(&ghost->aTicks[tick].v1.inputs);
		}
	}
	else if (bShowInputsWhileDriving) {
		DisplayInputs(GetLocalPlayerInterface<IInput>()->GetControls());
	}
#endif

	DisplayPlayerNames();
}

void DoConfigSave() {
	std::ofstream file("CwoeeGhosts/config.sav", std::iostream::out | std::iostream::binary);
	if (!file.is_open()) return;

	file.write((char*)&nGhostVisuals, sizeof(nGhostVisuals));
	file.write((char*)&bShowInputsWhileDriving, sizeof(bShowInputsWhileDriving));
	file.write(sPlayerNameOverride, sizeof(sPlayerNameOverride));
	file.write((char*)&nDifficulty, sizeof(nDifficulty));
	file.write((char*)&bChallengesOneGhostOnly, sizeof(bChallengesOneGhostOnly));
	file.write((char*)&bChallengesPBGhost, sizeof(bChallengesPBGhost));
}

void DoConfigLoad() {
	std::ifstream file("CwoeeGhosts/config.sav", std::iostream::in | std::iostream::binary);
	if (!file.is_open()) return;

	file.read((char*)&nGhostVisuals, sizeof(nGhostVisuals));
	file.read((char*)&bShowInputsWhileDriving, sizeof(bShowInputsWhileDriving));
	file.read(sPlayerNameOverride, sizeof(sPlayerNameOverride));
	sPlayerNameOverride[31] = 0;
	file.read((char*)&nDifficulty, sizeof(nDifficulty));
	file.read((char*)&bChallengesOneGhostOnly, sizeof(bChallengesOneGhostOnly));
	file.read((char*)&bChallengesPBGhost, sizeof(bChallengesPBGhost));
}

void DebugMenu() {
	ChloeMenuLib::BeginMenu();

	QuickValueEditor("Show Inputs While Driving", bShowInputsWhileDriving);
	QuickValueEditor("Player Name Override", sPlayerNameOverride, sizeof(sPlayerNameOverride));

	if (TheGameFlowManager.CurrentGameFlowState == GAMEFLOW_STATE_IN_FRONTEND) {
		QuickValueEditor("Replay Viewer", bViewReplayMode);
		if (bViewReplayMode) {
			if (DrawMenuOption(std::format("Replay Viewer Ghost - {}", bViewReplayTargetTime ? "Target Time" : "Personal Best"))) {
				bViewReplayTargetTime = !bViewReplayTargetTime;
			}
		}
		if (bChallengeSeriesMode) {
			const char* difficultyNames[] = {
					"Easy",
					"Normal",
					"Hard",
					"Very Hard",
			};
			const char* difficultyDescs[] = {
					"Easier ghosts",
					"Average ghosts",
					"Faster community ghosts",
					"Speedrunners' community ghosts",
			};
			if (DrawMenuOption(std::format("Difficulty - {}", difficultyNames[nDifficulty]))) {
				ChloeMenuLib::BeginMenu();
				for (int i = 0; i < NUM_DIFFICULTY; i++) {
					if (DrawMenuOption(difficultyNames[i], difficultyDescs[i])) {
						nDifficulty = (eDifficulty)i;
					}
				}
				ChloeMenuLib::EndMenu();
			}
			if (nDifficulty != DIFFICULTY_EASY) {
				QuickValueEditor("Show Target Ghost Only", bChallengesOneGhostOnly);
				if (DrawMenuOption(std::format("Show Personal Ghost - {}", bChallengesPBGhost))) {
					bChallengesPBGhost = !bChallengesPBGhost;
				}
			}
		}
		else {
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
	}

	QuickValueEditor("Verify Game Data Integrity", bCheckFileIntegrity);
	DrawMenuOption(std::format("Game Data Hash: {:X}", nLocalGameFilesHash));

	ChloeMenuLib::EndMenu();
}