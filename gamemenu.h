class TTDifficulty : public FEToggleWidget {
public:
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

class TTPBGhostPractice : public FEToggleWidget {
public:
	TTPBGhostPractice(bool state) : FEToggleWidget(state) {}

	void Act(const char* a2, uint32_t a3) override {
		if (a3 == 0x9120409E || a3 == 0xB5971BF1) {
			bPracticeOpponentsOnly = !bPracticeOpponentsOnly;
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
			title->LabelHash = !bPracticeOpponentsOnly ? 0x16FDEF36 : 0xF6BBD534;
			title->Flags |= 0x400000;
			title->Flags = title->Flags & 0xFFBFFFFD | 0x400000;
		}
	}
};

class TTInputDisplay : public FEToggleWidget {
public:
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

class TTNOS : public FEToggleWidget {
public:
	TTNOS(bool state) : FEToggleWidget(state) {}

	void Act(const char* a2, uint32_t a3) override {
		if (a3 == 0x9120409E) {
			nNitroType--;
			if (nNitroType < 0) nNitroType = NUM_NITRO-1;
		}
		if (a3 == 0xB5971BF1) {
			nNitroType++;
			if (nNitroType >= NUM_NITRO) nNitroType = 0;
		}

		bMovedLastUpdate = true;
		BlinkArrows(a3);
		Draw();
	}
	void Draw() override {
		if (auto title = pTitle) {
			title->LabelHash = Attrib::StringHash32("GHOST_NITRO");
			title->Flags |= 0x400000;
			title->Flags = title->Flags & 0xFFBFFFFD | 0x400000;
		}
		if (auto title = pData) {
			switch (nNitroType) {
				case NITRO_OFF:
					FEPrintf(title, "Off");
					break;
				case NITRO_ON:
					FEPrintf(title, "On");
					break;
				case NITRO_INF:
					FEPrintf(title, "Infinite");
					break;
			}
		}
	}
};

class TTSpdbrk : public FEToggleWidget {
public:
	TTSpdbrk(bool state) : FEToggleWidget(state) {}

	void Act(const char* a2, uint32_t a3) override {
		if (a3 == 0x9120409E) {
			nSpeedbreakerType--;
			if (nSpeedbreakerType < 0) nSpeedbreakerType = NUM_NITRO-1;
		}
		if (a3 == 0xB5971BF1) {
			nSpeedbreakerType++;
			if (nSpeedbreakerType >= NUM_NITRO) nSpeedbreakerType = 0;
		}

		bMovedLastUpdate = true;
		BlinkArrows(a3);
		Draw();
	}
	void Draw() override {
		if (auto title = pTitle) {
			title->LabelHash = Attrib::StringHash32("GHOST_SPDBRK");
			title->Flags |= 0x400000;
			title->Flags = title->Flags & 0xFFBFFFFD | 0x400000;
		}
		if (auto title = pData) {
			switch (nSpeedbreakerType) {
				case NITRO_OFF:
					FEPrintf(title, "Off");
					break;
				case NITRO_ON:
					FEPrintf(title, "On");
					break;
				case NITRO_INF:
					FEPrintf(title, "Infinite");
					break;
			}
		}
	}
};

void __thiscall SetupTimeTrial(UIOptionsScreen* pThis) {
	FEngSetLanguageHash(pThis->PackageFilename, 0x42ADB44C, Attrib::StringHash32(pThis->mCalledFromPauseMenu ? "GHOST_OPTIONS_H" : "GHOST_OPTIONS_L"));

	//bool showChallengeSeries = bChallengeSeriesMode || TheGameFlowManager.CurrentGameFlowState == GAMEFLOW_STATE_IN_FRONTEND;
	//bool showCareer = bCareerMode || TheGameFlowManager.CurrentGameFlowState == GAMEFLOW_STATE_IN_FRONTEND;
	//bool showQR = (!bChallengeSeriesMode && !bCareerMode) || TheGameFlowManager.CurrentGameFlowState == GAMEFLOW_STATE_IN_FRONTEND;
	bool showChallengeSeries = bChallengeSeriesMode;
	bool showCareer = bCareerMode;
	bool showQR = !bChallengeSeriesMode && !bCareerMode;

	if (TheGameFlowManager.CurrentGameFlowState != GAMEFLOW_STATE_RACING && showChallengeSeries) {
		UIWidgetMenu::AddToggleOption(pThis, new TTDifficulty(true), true);
	}

	UIWidgetMenu::AddToggleOption(pThis, new TTGhostVisuals(true), true);
	if (showChallengeSeries) UIWidgetMenu::AddToggleOption(pThis, new TTPBGhost(true), true);
	if (showQR) UIWidgetMenu::AddToggleOption(pThis, new TTPBGhostPractice(true), true);
	UIWidgetMenu::AddToggleOption(pThis, new TTInputDisplay(true), true);

	if (TheGameFlowManager.CurrentGameFlowState != GAMEFLOW_STATE_RACING && showQR) {
		UIWidgetMenu::AddToggleOption(pThis, new TTNOS(true), true);
		UIWidgetMenu::AddToggleOption(pThis, new TTSpdbrk(true), true);
	}
}

void ApplyCustomMenuHooks() {
	NyaHooks::OptionsMenuHook::AddNewMenu((void*)SetupTimeTrial, 0x4DF98FB2, Attrib::StringHash32("GHOST_OPTIONS"));
	NyaHooks::OptionsMenuHook::AddStringRecord(Attrib::StringHash32("GHOST_OPTIONS"), "Time Trials");
	NyaHooks::OptionsMenuHook::AddStringRecord(Attrib::StringHash32("GHOST_OPTIONS_H"), "TIME TRIALS");
	NyaHooks::OptionsMenuHook::AddStringRecord(Attrib::StringHash32("GHOST_OPTIONS_L"), "time trials");
	NyaHooks::OptionsMenuHook::AddStringRecord(Attrib::StringHash32("DIFFICULTY"), "Difficulty");
	NyaHooks::OptionsMenuHook::AddStringRecord(Attrib::StringHash32("GHOST_VISUALS"), "Ghosts");
	NyaHooks::OptionsMenuHook::AddStringRecord(Attrib::StringHash32("PB_GHOST"), "Show Personal Best");
	NyaHooks::OptionsMenuHook::AddStringRecord(Attrib::StringHash32("INPUT_DISPLAY"), "Input Display");
	NyaHooks::OptionsMenuHook::AddStringRecord(Attrib::StringHash32("GHOST_NITRO"), "N20");
	NyaHooks::OptionsMenuHook::AddStringRecord(Attrib::StringHash32("GHOST_SPDBRK"), "Speedbreaker");
}