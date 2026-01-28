class TTDifficulty : public FEToggleWidget {
public:
	void* operator new(size_t size) {
		return GAME_malloc(size);
	}

	TTDifficulty(bool state) : FEToggleWidget(state) {}

	void Act(const char* a2, uint32_t a3) override {
		if (a3 == 0x9120409E) {
			nDifficulty--;
			if (nDifficulty < 0) nDifficulty = NUM_DIFFICULTY-1;
		}
		if (a3 == 0xB5971BF1) {
			nDifficulty++;
			if (nDifficulty >= NUM_DIFFICULTY) nDifficulty = 0;
		}

		bMovedLastUpdate = true;
		BlinkArrows(a3);
		Draw();
	}
	void Draw() override {
		if (auto title = pTitle) {
			title->LabelHash = Attrib::StringHash32("DIFFICULTY");
			title->Flags |= 0x400000;
			title->Flags = title->Flags & 0xFFBFFFFD | 0x400000;
		}
		if (auto title = pData) {
			switch (nDifficulty) {
				case DIFFICULTY_EASY:
					FEPrintf(title, "Easy");
					break;
				case DIFFICULTY_NORMAL:
					FEPrintf(title, "Normal");
					break;
				case DIFFICULTY_HARD:
					FEPrintf(title, "Hard");
					break;
			}
			title->Flags |= 0x400000;
			title->Flags = title->Flags & 0xFFBFFFFD | 0x400000;
		}
	}
};

class TTGhostVisuals : public FEToggleWidget {
public:
	void* operator new(size_t size) {
		return GAME_malloc(size);
	}

	TTGhostVisuals(bool state) : FEToggleWidget(state) {}

	void Act(const char* a2, uint32_t a3) override {
		if (a3 == 0x9120409E) {
			nGhostVisuals--;
			if (nGhostVisuals < 0) nGhostVisuals = NUM_GHOST_VISUALS-1;
		}
		if (a3 == 0xB5971BF1) {
			nGhostVisuals++;
			if (nGhostVisuals >= NUM_GHOST_VISUALS) nGhostVisuals = 0;
		}

		bMovedLastUpdate = true;
		BlinkArrows(a3);
		Draw();
	}
	void Draw() override {
		if (auto title = pTitle) {
			title->LabelHash = Attrib::StringHash32("GHOST_VISUALS");
			title->Flags |= 0x400000;
			title->Flags = title->Flags & 0xFFBFFFFD | 0x400000;
		}
		if (auto title = pData) {
			switch (nGhostVisuals) {
				case GHOST_HIDE:
					FEPrintf(title, "Hidden");
					break;
				case GHOST_SHOW:
					FEPrintf(title, "Shown");
					break;
				case GHOST_HIDE_NEARBY:
					FEPrintf(title, "Hide When Close");
					break;
			}
		}
	}
};

class TTPBGhost : public FEToggleWidget {
public:
	void* operator new(size_t size) {
		return GAME_malloc(size);
	}

	TTPBGhost(bool state) : FEToggleWidget(state) {}

	void Act(const char* a2, uint32_t a3) override {
		if (a3 == 0x9120409E || a3 == 0xB5971BF1) {
			bChallengesPBGhost = !bChallengesPBGhost;
		}

		bMovedLastUpdate = true;
		BlinkArrows(a3);
		Draw();
	}
	void Draw() override {
		if (auto title = pTitle) {
			title->LabelHash = Attrib::StringHash32("PB_GHOST");
			title->Flags |= 0x400000;
			title->Flags = title->Flags & 0xFFBFFFFD | 0x400000;
		}
		if (auto title = pData) {
			title->LabelHash = bChallengesPBGhost ? 0x16FDEF36 : 0xF6BBD534;
			title->Flags |= 0x400000;
			title->Flags = title->Flags & 0xFFBFFFFD | 0x400000;
		}
	}
};

class TTInputDisplay : public FEToggleWidget {
public:
	void* operator new(size_t size) {
		return GAME_malloc(size);
	}

	TTInputDisplay(bool state) : FEToggleWidget(state) {}

	void Act(const char* a2, uint32_t a3) override {
		if (a3 == 0x9120409E || a3 == 0xB5971BF1) {
			bShowInputsWhileDriving = !bShowInputsWhileDriving;
		}

		bMovedLastUpdate = true;
		BlinkArrows(a3);
		Draw();
	}
	void Draw() override {
		if (auto title = pTitle) {
			title->LabelHash = Attrib::StringHash32("INPUT_DISPLAY");
			title->Flags |= 0x400000;
			title->Flags = title->Flags & 0xFFBFFFFD | 0x400000;
		}
		if (auto title = pData) {
			title->LabelHash = bShowInputsWhileDriving ? 0x16FDEF36 : 0xF6BBD534;
			title->Flags |= 0x400000;
			title->Flags = title->Flags & 0xFFBFFFFD | 0x400000;
		}
	}
};

// just using FEPrintf or somesuch to create a literal string seems to offset the text labels a bit, this seems to be the only way to make them look correct
auto SearchForString_orig = (const char*(__fastcall*)(void*, uint32_t))nullptr;
const char* __fastcall SearchForStringHooked(void* a1, uint32_t hash) {
	auto str = SearchForString_orig(a1, hash);
	if (!str) {
		if (hash == Attrib::StringHash32("GHOST_OPTIONS")) { return "Time Trials"; }
		if (hash == Attrib::StringHash32("GHOST_OPTIONS_L")) { return TheGameFlowManager.CurrentGameFlowState == GAMEFLOW_STATE_RACING ? "TIME TRIALS" : "time trials"; }
		if (hash == Attrib::StringHash32("DIFFICULTY")) { return "Difficulty"; }
		if (hash == Attrib::StringHash32("GHOST_VISUALS")) { return "Ghosts"; }
		if (hash == Attrib::StringHash32("PB_GHOST")) { return "Show Personal Best"; }
		if (hash == Attrib::StringHash32("INPUT_DISPLAY")) { return "Input Display"; }
	}
	return str;
}

void __thiscall OnlineOptionsHooked(UIWidgetMenu* pThis, FEToggleWidget* widget, bool a3) {
	//UIWidgetMenu::AddToggleOption(pThis, widget, a3);
	if (TheGameFlowManager.CurrentGameFlowState != GAMEFLOW_STATE_RACING) {
		UIWidgetMenu::AddToggleOption(pThis, new TTDifficulty(true), a3);
		UIWidgetMenu::AddToggleOption(pThis, new TTGhostVisuals(true), a3);
		UIWidgetMenu::AddToggleOption(pThis, new TTPBGhost(true), a3);
		UIWidgetMenu::AddToggleOption(pThis, new TTInputDisplay(true), a3);
	}
	else {
		UIWidgetMenu::AddToggleOption(pThis, new TTGhostVisuals(true), a3);
		UIWidgetMenu::AddToggleOption(pThis, new TTInputDisplay(true), a3);
	}
}

void ApplyCustomMenuHooks() {
	// enable online options
	// todo make this an actual addition, not a replacement
	NyaHookLib::Patch<uint16_t>(0x5290B2, 0x9090);
	NyaHookLib::Patch(0x5290D6 + 1, 0x4DF98FB2); // change icon hash
	NyaHookLib::Patch(0x5290D1 + 1, Attrib::StringHash32("GHOST_OPTIONS"));
	NyaHookLib::Patch(0x52A315 + 1, Attrib::StringHash32("GHOST_OPTIONS_L"));
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x52A30D, &OnlineOptionsHooked);

	SearchForString_orig = (const char*(__fastcall*)(void*, uint32_t))NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x57E924, &SearchForStringHooked);
}