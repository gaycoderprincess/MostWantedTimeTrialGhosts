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

void __thiscall SetupTimeTrial(UIOptionsScreen* pThis) {
	FEngSetLanguageHash(pThis->PackageFilename, 0x42ADB44C, Attrib::StringHash32(pThis->mCalledFromPauseMenu ? "GHOST_OPTIONS_H" : "GHOST_OPTIONS_L"));

	if (TheGameFlowManager.CurrentGameFlowState != GAMEFLOW_STATE_RACING) {
		UIWidgetMenu::AddToggleOption(pThis, new TTDifficulty(true), true);
		UIWidgetMenu::AddToggleOption(pThis, new TTGhostVisuals(true), true);
		UIWidgetMenu::AddToggleOption(pThis, new TTPBGhost(true), true);
		UIWidgetMenu::AddToggleOption(pThis, new TTInputDisplay(true), true);
	}
	else {
		UIWidgetMenu::AddToggleOption(pThis, new TTGhostVisuals(true), true);
		UIWidgetMenu::AddToggleOption(pThis, new TTInputDisplay(true), true);
	}
}

void ApplyCustomMenuHooks() {
	NyaHooks::OptionsMenuHook::AddMenuOption((void*)SetupTimeTrial, 0x4DF98FB2, Attrib::StringHash32("GHOST_OPTIONS"));
	NyaHooks::OptionsMenuHook::AddStringRecord(Attrib::StringHash32("GHOST_OPTIONS"), "Time Trials");
	NyaHooks::OptionsMenuHook::AddStringRecord(Attrib::StringHash32("GHOST_OPTIONS_H"), "TIME TRIALS");
	NyaHooks::OptionsMenuHook::AddStringRecord(Attrib::StringHash32("GHOST_OPTIONS_L"), "time trials");
	NyaHooks::OptionsMenuHook::AddStringRecord(Attrib::StringHash32("DIFFICULTY"), "Difficulty");
	NyaHooks::OptionsMenuHook::AddStringRecord(Attrib::StringHash32("GHOST_VISUALS"), "Ghosts");
	NyaHooks::OptionsMenuHook::AddStringRecord(Attrib::StringHash32("PB_GHOST"), "Show Personal Best");
	NyaHooks::OptionsMenuHook::AddStringRecord(Attrib::StringHash32("INPUT_DISPLAY"), "Input Display");
}