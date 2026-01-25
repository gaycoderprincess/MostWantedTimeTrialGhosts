typedef FECustomizationRecord GameCustomizationRecord;

void WriteLog(const std::string& str) {
	static auto file = std::ofstream("NFSMWTimeTrialGhosts_gcp.log");

	file << str;
	file << "\n";
	file.flush();
}

bool IsInLoadingScreen() {
	if (FadeScreen::IsFadeScreenOn()) return true;
	if (cFEng::IsPackagePushed(cFEng::mInstance, "Loading.fng")) return true;
	if (cFEng::IsPackagePushed(cFEng::mInstance, "WS_Loading.fng")) return true;
	return false;
}

bool IsInNIS() {
	return INIS::mInstance && INIS::mInstance->IsPlaying();
}

UserProfile* GetUserProfile() {
	return FEDatabase->mUserProfile;
}

IPlayer* GetLocalPlayer() {
	auto& list = PLAYER_LIST::GetList(PLAYER_LOCAL);
	if (list.empty()) return nullptr;
	return list[0];
}

ISimable* GetLocalPlayerSimable() {
	auto ply = GetLocalPlayer();
	if (!ply) return nullptr;
	return ply->GetSimable();
}

template<typename T>
T* GetLocalPlayerInterface() {
	auto ply = GetLocalPlayerSimable();
	if (!ply) return nullptr;
	T* out;
	if (!ply->QueryInterface<T>(&out)) return nullptr;
	return out;
}

auto GetLocalPlayerVehicle() { return GetLocalPlayerInterface<IVehicle>(); }
auto GetLocalPlayerEngine() { return GetLocalPlayerInterface<IEngine>(); }

std::vector<IVehicle*> GetActiveVehicles(int driverClass = -1) {
	auto& list = VEHICLE_LIST::GetList(VEHICLE_ALL);
	std::vector<IVehicle*> cars;
	for (int i = 0; i < list.size(); i++) {
		if (!list[i]->IsActive()) continue;
		if (list[i]->IsLoading()) continue;
		if (driverClass >= 0 && list[i]->GetDriverClass() != driverClass) continue;
		cars.push_back(list[i]);
	}
	return cars;
}

bool IsVehicleValidAndActive(IVehicle* vehicle) {
	auto cars = GetActiveVehicles();
	for (auto& car : cars) {
		if (car == vehicle) return true;
	}
	return false;
}

IVehicle* GetClosestActiveVehicle(NyaVec3 toCoords) {
	auto sourcePos = toCoords;
	IVehicle* out = nullptr;
	float distance = 99999;
	auto cars = GetActiveVehicles();
	for (auto& car : cars) {
		auto targetPos = *car->GetPosition();
		if ((sourcePos - targetPos).length() < distance) {
			out = car;
			distance = (sourcePos - targetPos).length();
		}
	}
	return out;
}

void ValueEditorMenu(float& value) {
	ChloeMenuLib::BeginMenu();

	static char inputString[1024] = {};
	ChloeMenuLib::AddTextInputToString(inputString, 1024, true);
	ChloeMenuLib::SetEnterHint("Apply");

	if (DrawMenuOption(inputString + (std::string)"...", "", false, false) && inputString[0]) {
		value = std::stof(inputString);
		memset(inputString,0,sizeof(inputString));
		ChloeMenuLib::BackOut();
	}

	ChloeMenuLib::EndMenu();
}

void ValueEditorMenu(int& value) {
	ChloeMenuLib::BeginMenu();

	static char inputString[1024] = {};
	ChloeMenuLib::AddTextInputToString(inputString, 1024, true);
	ChloeMenuLib::SetEnterHint("Apply");

	if (DrawMenuOption(inputString + (std::string)"...", "", false, false) && inputString[0]) {
		value = std::stoi(inputString);
		memset(inputString,0,sizeof(inputString));
		ChloeMenuLib::BackOut();
	}

	ChloeMenuLib::EndMenu();
}

void ValueEditorMenu(char* value, int len) {
	ChloeMenuLib::BeginMenu();

	static char inputString[1024] = {};
	ChloeMenuLib::AddTextInputToString(inputString, 1024, false);
	ChloeMenuLib::SetEnterHint("Apply");

	if (DrawMenuOption(inputString + (std::string)"...", "", false, false) && inputString[0]) {
		strcpy_s(value, len, inputString);
		memset(inputString,0,sizeof(inputString));
		ChloeMenuLib::BackOut();
	}

	ChloeMenuLib::EndMenu();
}

void QuickValueEditor(const char* name, float& value) {
	if (DrawMenuOption(std::format("{} - {}", name, value))) { ValueEditorMenu(value); }
}

void QuickValueEditor(const char* name, int& value) {
	if (DrawMenuOption(std::format("{} - {}", name, value))) { ValueEditorMenu(value); }
}

void QuickValueEditor(const char* name, bool& value) {
	if (DrawMenuOption(std::format("{} - {}", name, value))) { value = !value; }
}

void QuickValueEditor(const char* name, char* value, int len) {
	if (DrawMenuOption(std::format("{} - {}", name, value))) { ValueEditorMenu(value, len); }
}

int GetRaceNumLaps() {
	auto race = GRaceStatus::fObj->mRaceParms;
	if (!GRaceParameters::GetIsLoopingRace(race)) return 1;
	if (GRaceParameters::GetIsPursuitRace(race)) return 1;
	if (auto index = race->mIndex) {
		return index->mNumLaps;
	}
	return GRaceParameters::GetNumLaps(race);
}

void SetRaceNumLaps(GRaceParameters* race, int numLaps) {
	if (!GRaceParameters::GetIsLoopingRace(race)) return;
	if (GRaceParameters::GetIsPursuitRace(race)) return;
	if (auto index = race->mIndex) {
		index->mNumLaps = numLaps;
	}
	*(uint8_t*)Attrib::Instance::GetAttributePointer(race->mRaceRecord, Attrib::StringHash32("NumLaps"), 0) = numLaps;
}

NyaVec3 WorldToRenderCoords(NyaVec3 world) {
	return {world.z, -world.x, world.y};
}

NyaVec3 RenderToWorldCoords(NyaVec3 render) {
	return {-render.y, render.z, render.x};
}

NyaMat4x4 WorldToRenderMatrix(NyaMat4x4 world) {
	NyaMat4x4 out;
	out.x = WorldToRenderCoords(world.x);
	out.y = -WorldToRenderCoords(world.y); // v1, up
	out.z = WorldToRenderCoords(world.z);
	out.p = WorldToRenderCoords(world.p);
	return out;
}

Camera* GetLocalPlayerCamera() {
	return eViews[EVIEW_PLAYER1].pCamera;
}

// view to world
NyaMat4x4 PrepareCameraMatrix(Camera* pCamera) {
	return pCamera->CurrentKey.Matrix.Invert();
}

// todo this is probably bad
GRacerInfo* GetRacerInfoFromHandle(HSIMABLE handle) {
	auto race = GRaceStatus::fObj;
	for (int i = 0; i < race->mRacerCount; i++) {
		if (race->mRacerInfo[i].mhSimable == handle) return &race->mRacerInfo[i];
	}
	return nullptr;
}

FECustomizationRecord CreateStockCustomizations(uint32_t carModel) {
	FECustomizationRecord record;
	FECustomizationRecord::Default(&record);

	FECarRecord tmp;
	tmp.FEKey = carModel;
	tmp.VehicleKey = carModel;
	RideInfo info;
	RideInfo::Init(&info, FECarRecord::GetType(&tmp), CarRenderUsage_Player, false, false);
	RideInfo::SetStockParts(&info);
	FECustomizationRecord::WriteRideIntoRecord(&record, &info);
	return record;
}

uint32_t GetCarFEKey(uint32_t modelHash) {
	auto collection = Attrib::FindCollection(Attrib::StringHash32("pvehicle"), modelHash);
	if (!collection) return modelHash;

	if (auto type = (uint32_t*)Attrib::Collection::GetData(collection, Attrib::StringHash32("frontend"), 0)) {
		return type[1];
	}
	return modelHash;
}

FECarRecord CreateStockCarRecord(const char* carModel) {
	uint32_t rideHash = Attrib::StringHash32(carModel);

	FECarRecord car;
	car.Handle = rideHash;
	car.FEKey = GetCarFEKey(rideHash);
	car.VehicleKey = rideHash;
	car.FilterBits = 0x10001;
	car.Customization = -1;
	car.CareerHandle = -1;
	return car;
}

void SetRacerName(GRacerInfo* racer, const char* name) {
	racer->mName = name;
}

const char* GetLocalPlayerName() {
	return GetUserProfile()->m_aProfileName;
}

wchar_t gDLLDir[MAX_PATH];
class DLLDirSetter {
public:
	wchar_t backup[MAX_PATH];

	DLLDirSetter() {
		GetCurrentDirectoryW(MAX_PATH, backup);
		SetCurrentDirectoryW(gDLLDir);
	}
	~DLLDirSetter() {
		SetCurrentDirectoryW(backup);
	}
};