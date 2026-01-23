void HideGhostCar(NyaMat4x4* carMatrix) {
	auto car = GetClosestActiveVehicle(RenderToWorldCoords(carMatrix->p));
	if (!car) return;
	if (car == GetLocalPlayerVehicle()) return;
	auto playerCar = GetLocalPlayerVehicle();
	if (!playerCar) return;

	bool hide = nGhostVisuals == GHOST_HIDE;
	if (!hide) {
		auto dist = (*playerCar->GetPosition() - *car->GetPosition()).length();
		if (dist < 8) hide = true;
	}

	if (!hide) return;

	// hacky solution!! it works but checking some CarRenderInfo ptr against the player and disabling DrawCars would be way better
	carMatrix->p = {0,0,0};
}

auto CarGetVisibleStateOrig = (int(__thiscall*)(eView*, const bVector3*, const bVector3*, bMatrix4*))nullptr;
int __thiscall CarGetVisibleStateHooked(eView* a1, const bVector3* a2, const bVector3* a3, bMatrix4* a4) {
	auto carMatrix = (NyaMat4x4*)a4;
	if (TheGameFlowManager.CurrentGameFlowState == GAMEFLOW_STATE_RACING && !IsInNIS() && (nGhostVisuals == GHOST_HIDE || nGhostVisuals == GHOST_HIDE_NEARBY)) {
		HideGhostCar(carMatrix);
	}
	return CarGetVisibleStateOrig(a1, a2, a3, a4);
}

void ApplyCarRenderHooks() {
	CarGetVisibleStateOrig = (int(__thiscall*)(eView*, const bVector3*, const bVector3*, bMatrix4*))NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x74E346, &CarGetVisibleStateHooked);
	NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x73786A, 0x73790A); // don't reset CarScaleMatrix when exiting to menu
	NyaHooks::LateInitHook::aFunctions.push_back([]() { CarScaleMatrix = UMath::Matrix4::kIdentity; });
}