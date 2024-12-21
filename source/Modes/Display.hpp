 class DockedFrameskipGui : public tsl::Gui {
public:
	int y_1;
	int x_1;
	int height = 480;
	int block_width = 20;
	int block_height = 40;
	int columns = 18;
	bool block;
    uint64_t tick;
    bool state;
    DockedFrameskipGui() {
		y_1 = 0;
		x_1 = 0;
        tick = 0;
		block = false;
        state = false;
	}

    virtual tsl::elm::Element* createUI() override {
        auto frame = new tsl::elm::OverlayFrame("FPSLocker", "프레임 스킵 테스터");

		auto list = new tsl::elm::List();

		list->addItem(new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
            if (!state) {
                renderer->drawString(   "사용 방법:\n\n"
                                        "1. 셔터 속도와 ISO를 수동으로 설정\n"
                                        "    가능한 카메라를 구매하십시오.\n\n"
                                        "2. 셔터 속도를 1/10초 이상으로 설정,\n"
                                        "    ISO를 너무 밝거나 어둡지 않도록\n"
                                        "    설정하십시오.\n\n"
                                        "    (일반적으로 1/10초당 약 50 정도)\n\n"
                                        "3. 계속하려면  버튼을 입력하십시오.\n\n"
                                        "4. 화면을 캡쳐하십시오.\n\n"
                                        "5. 처음과 마지막을 제외한 모든 블록의\n"
                                        "    밝기가 고르지 않은 경우, 화면이 현재\n"
                                        "    설정한 주사율을 지원하지 않으며,\n"
                                        "    다른 주사율로 실행되고 있다는 뜻입니다.\n\n"
                                        "하드웨어 솔루션이 프레임 시간을\n"
                                        "균등하게 분할하는 가장 좋은 방법이기에,\n"
                                        "디스플레이가 프레임 스킵되더라도\n"
                                        "주사율이 일치하지 않는 낮은 FPS 타겟을\n"
                                        "사용하는 것보다 훨씬 잘 작동한다는 점을\n"
                                        "고려해주시길 바랍니다.", false, x, y+20, 18, renderer->a(0xFFFF));
            }
			else if (!block) {
				renderer->fillScreen(0xF000);
				renderer->drawRect(x+x_1, y+y_1, block_width, block_height, renderer->a(0xFFFF));
				renderer->drawString(" 버튼으로 나가기", false, x+20, y+height+20, 19, renderer->a(0xFFFF));
			}
			else {
				renderer->drawString("렌더링이 너무 오래 걸립니다!\n게임을 닫고 홈 화면 이동 후,\n다시 시도하세요.", false, x, y+20, 19, renderer->a(0xFFFF));
			}
			
		}), height+40);		
		
		frame->setContent(list);

        return frame;
    }

	// Called once every frame to handle inputs not handled by other UI elements
	virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
        if (!state && (keysDown & HidNpadButton_A)) {
            state = true;
            return true;
        }
		if (state && !block) {
			y_1 += block_height;
			y_1 %= height;
			if (y_1 == 0) {
				x_1 += block_width;
				x_1 %= (block_width*columns);
			}
			if (!tick) {
				tick = svcGetSystemTick();
				return false;
			}
			if ((svcGetSystemTick() - tick) > ((19200000 / *refreshRate_shared) * 2)) {
				block = true;
			}
			else tick = svcGetSystemTick();
		}
		return false;   // Return true here to singal the inputs have been consumed
	}
};

class DockedManualGui;

class DockedWizardGui : public tsl::Gui {
public:
	uint64_t tick;
    size_t i;
	char Docked_c[512] ="해당 메뉴는 60 Hz 미만에서 지원되는\n"
						"주사율 40, 45, 50, 55 Hz를 순차적으로\n"
						"확인합니다.\n\n"
						"각 주사율이 정상 작동하는지 확인하려면\n"
						"표시된 버튼을 입력하세요.\n\n"
						"15초 이내 입력이 없으면 스킵됩니다.";

	char PressButton[32] = " 버튼으로 시작.";
	DockedModeRefreshRateAllowed rr;
	DockedModeRefreshRateAllowed rr_default;
	DockedAdditionalSettings as;
    DockedWizardGui() {
		LoadDockedModeAllowedSave(rr_default, as);
		memset(&rr, 1, sizeof(rr));
		tick = 0;
        i = 0;
	}

    virtual tsl::elm::Element* createUI() override {
        auto frame = new tsl::elm::OverlayFrame("FPSLocker", "독 모드 디스플레이 설정 마법사");

		auto list = new tsl::elm::List();

		list->addItem(new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {

			renderer->drawString(Docked_c, false, x, y+20, 20, renderer->a(0xFFFF));

			renderer->drawString(PressButton, false, x, y+200, 20, renderer->a(0xFFFF));
			
		}), 200);		
		
		frame->setContent(list);

        return frame;
    }

	// Called once every frame to handle inputs not handled by other UI elements
	virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
		smInitialize();
		if (R_SUCCEEDED(apmInitialize())) {
			ApmPerformanceMode mode = ApmPerformanceMode_Invalid;
			apmGetPerformanceMode(&mode);
			apmExit();
			if (mode != ApmPerformanceMode_Boost) {
				smExit();
				tsl::goBack();
				return true;
			}
		}
		smExit();
		if (keysHeld & HidNpadButton_B) {
			if (R_SUCCEEDED(SaltySD_Connect())) {
				SaltySD_SetAllowedDockedRefreshRates(rr_default);
				svcSleepThread(100'000);
				SaltySD_SetDisplayRefreshRate(60);
				svcSleepThread(100'000);
				SaltySD_Term();
			}
			tsl::goBack();
			return true;
		}
		static u64 keyCheck = HidNpadButton_ZL;
		if ((keysHeld & HidNpadButton_X) && !tick) {
			tick = svcGetSystemTick();
			if (R_SUCCEEDED(SaltySD_Connect())) {
				SaltySD_SetAllowedDockedRefreshRates(rr);
				svcSleepThread(100'000);
				SaltySD_SetDisplayRefreshRate(40);
				svcSleepThread(100'000);
				SaltySD_Term();
				snprintf(PressButton, sizeof(PressButton), "40 Hz 작동 확인:  입력.");
			}
			return true;
		}
		if (tick) {
			if (DockedModeRefreshRateAllowedValues[i] == 60) {
				if (R_SUCCEEDED(SaltySD_Connect())) {
					SaltySD_SetAllowedDockedRefreshRates(rr);
					svcSleepThread(100'000);
					SaltySD_Term();
				}
				SaveDockedModeAllowedSave(rr, as);
				tsl::goBack();
				tsl::changeTo<DockedManualGui>();
				return true;
			}
			if (svcGetSystemTick() - tick < (15 * 19200000)) {
				if (keysHeld & keyCheck) {
					rr[i] = true;
					i++;
					if (R_SUCCEEDED(SaltySD_Connect())) {
						SaltySD_SetDisplayRefreshRate(DockedModeRefreshRateAllowedValues[i]);
						svcSleepThread(100'000);
						SaltySD_Term();
					}
					if (i % 1 == 0) {
						keyCheck = HidNpadButton_X;
						snprintf(PressButton, sizeof(PressButton), "%d Hz 작동 확인:  입력.", DockedModeRefreshRateAllowedValues[i]);
					}
					if (i % 3 == 0) {
						keyCheck = HidNpadButton_Y;
						snprintf(PressButton, sizeof(PressButton), "%d Hz 작동 확인:  입력.", DockedModeRefreshRateAllowedValues[i]);
					}
					if (i % 2 == 0) {
						keyCheck = HidNpadButton_ZR;
						snprintf(PressButton, sizeof(PressButton), "%d Hz 작동 확인:  입력", DockedModeRefreshRateAllowedValues[i]);
					}
					tick = svcGetSystemTick();
					return true;
				}
			}
			else {
				rr[i] = false;
				i++;
				if (i % 1 == 0) {
					keyCheck = HidNpadButton_X;
					snprintf(PressButton, sizeof(PressButton), "%d Hz 작동 확인:  입력.", DockedModeRefreshRateAllowedValues[i]);
				}
				if (i % 3 == 0) {
					keyCheck = HidNpadButton_Y;
					snprintf(PressButton, sizeof(PressButton), "%d Hz 작동 확인:  입력.", DockedModeRefreshRateAllowedValues[i]);
				}
				if (i % 2 == 0) {
					keyCheck = HidNpadButton_ZR;
					snprintf(PressButton, sizeof(PressButton), "%d Hz 작동 확인:  입력.", DockedModeRefreshRateAllowedValues[i]);
				}
				if (R_SUCCEEDED(SaltySD_Connect())) {
					SaltySD_SetDisplayRefreshRate(DockedModeRefreshRateAllowedValues[i]);
					svcSleepThread(100'000);
					SaltySD_Term();
				}
				tick = svcGetSystemTick();
			}
		}
		return false;   // Return true here to singal the inputs have been consumed
	}
};

class DockedManualGui : public tsl::Gui {
public:
	uint32_t crc = 0;
	DockedModeRefreshRateAllowed rr = {0};
	DockedAdditionalSettings as;
    DockedManualGui() {
		LoadDockedModeAllowedSave(rr, as);
	}

    virtual tsl::elm::Element* createUI() override {
        auto frame = new tsl::elm::OverlayFrame("FPSLocker", "독 모드 디스플레이 수동 설정");

		auto list = new tsl::elm::List();

		size_t i = 0;
		while (i < 4) {
			char Hz[] = "120 Hz";
			snprintf(Hz, sizeof(Hz), "%d Hz", DockedModeRefreshRateAllowedValues[i]);
			auto *clickableListItem = new tsl::elm::ToggleListItem(Hz, rr[i]);
			clickableListItem->setClickListener([this, i](u64 keys) { 
				if (keys & HidNpadButton_A) {
					rr[i] = !rr[i];
					return true;
				}
				return false;
			});
			list->addItem(clickableListItem);	
			i++;
		}
		
		frame->setContent(list);

        return frame;
    }

	// Called once every frame to handle inputs not handled by other UI elements
	virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
		smInitialize();
		if (R_SUCCEEDED(apmInitialize())) {
			ApmPerformanceMode mode = ApmPerformanceMode_Invalid;
			apmGetPerformanceMode(&mode);
			apmExit();
			if (mode != ApmPerformanceMode_Boost) {
				tsl::goBack();
				return true;
			}
		}
		smExit();
		if (keysHeld & HidNpadButton_B) {
			if (R_SUCCEEDED(SaltySD_Connect())) {
				SaveDockedModeAllowedSave(rr, as);
				SaltySD_SetAllowedDockedRefreshRates(rr);
				svcSleepThread(100'000);
				SaltySD_Term();
			}
			tsl::goBack();
			return true;
		}
		return false;   // Return true here to singal the inputs have been consumed
	}
};

class DockedAdditionalGui : public tsl::Gui {
public:
	uint32_t crc = 0;
	DockedModeRefreshRateAllowed rr = {0};
	DockedAdditionalSettings as;
    DockedAdditionalGui() {
		LoadDockedModeAllowedSave(rr, as);
	}

    virtual tsl::elm::Element* createUI() override {
        auto frame = new tsl::elm::OverlayFrame("FPSLocker", "독 모드 디스플레이 추가 설정");

		auto list = new tsl::elm::List();

		auto *clickableListItem4 = new tsl::elm::ToggleListItem("패치의 60 Hz 강제 사용", !as.dontForce60InDocked);
		clickableListItem4->setClickListener([this](u64 keys) { 
			if (keys & HidNpadButton_A) {
				as.dontForce60InDocked = !as.dontForce60InDocked;
				if (R_SUCCEEDED(SaltySD_Connect())) {
					SaltySD_SetDontForce60InDocked(as.dontForce60InDocked);
					SaltySD_Term();
				}
				SaveDockedModeAllowedSave(rr, as);
				return true;
			}
			return false;
		});

		list->addItem(clickableListItem4);

		auto *clickableListItem5 = new tsl::elm::ToggleListItem("일치하지 않는 FPS 타겟에 가장 낮은 주사율 사용", as.fpsTargetWithoutRRMatchLowest);
		clickableListItem5->setClickListener([this](u64 keys) { 
			if (keys & HidNpadButton_A) {
				as.fpsTargetWithoutRRMatchLowest = !as.fpsTargetWithoutRRMatchLowest;
				if (R_SUCCEEDED(SaltySD_Connect())) {
					SaltySD_SetMatchLowestRR(as.fpsTargetWithoutRRMatchLowest);
					SaltySD_Term();
				}
				SaveDockedModeAllowedSave(rr, as);
				return true;
			}
			return false;
		});

		list->addItem(clickableListItem5);
		
		frame->setContent(list);

        return frame;
    }

	// Called once every frame to handle inputs not handled by other UI elements
	virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
		smInitialize();
		if (R_SUCCEEDED(apmInitialize())) {
			ApmPerformanceMode mode = ApmPerformanceMode_Invalid;
			apmGetPerformanceMode(&mode);
			apmExit();
			if (mode != ApmPerformanceMode_Boost) {
				tsl::goBack();
				return true;
			}
		}
		smExit();
		return false;   // Return true here to singal the inputs have been consumed
	}
};

class DockedGui : public tsl::Gui {
private:
	char Docked_c[256] = "";
	DockedModeRefreshRateAllowed rr;
	DockedAdditionalSettings as;
public:
    DockedGui() {
		mkdir("sdmc:/SaltySD/plugins/FPSLocker/", 777);
		mkdir("sdmc:/SaltySD/plugins/FPSLocker/ExtDisplays/", 777);
		LoadDockedModeAllowedSave(rr, as);
		smInitialize();
		setsysInitialize();
		SetSysEdid2 edid2 = {0};
		if (R_SUCCEEDED(setsysGetEdid2(setsysGetServiceSession(), &edid2))) {
			uint8_t highestRefreshRate = 0;
			for (int i = 0; i < 2; i++) {
				auto td = edid2.edid.timing_descriptor[i];
				uint32_t pixel_clock = td.pixel_clock * 10000;
				if (!pixel_clock) continue;
				uint32_t h_total = ((uint32_t)td.horizontal_active_pixels_msb << 8 | td.horizontal_active_pixels_lsb) + ((uint32_t)td.horizontal_blanking_pixels_msb << 8 | td.horizontal_blanking_pixels_lsb);
				uint32_t v_total = ((uint32_t)td.vertical_active_lines_msb << 8 | td.vertical_active_lines_lsb) + ((uint32_t)td.vertical_blanking_lines_msb << 8 | td.vertical_blanking_lines_lsb);
				uint8_t refreshRate = (uint8_t)round((float)pixel_clock / (float)(h_total * v_total));
				if (refreshRate > highestRefreshRate) highestRefreshRate = refreshRate;
			}
			SetSysModeLine* modes = (SetSysModeLine*)((uintptr_t)(&edid2.edid) + 0x80 + edid2.edid.dtd_start);
			for (int i = 0; i < 5; i++) {
				auto td = modes[i];
				uint32_t pixel_clock = td.pixel_clock * 10000;
				if (!pixel_clock) continue;
				uint32_t h_total = ((uint32_t)td.horizontal_active_pixels_msb << 8 | td.horizontal_active_pixels_lsb) + ((uint32_t)td.horizontal_blanking_pixels_msb << 8 | td.horizontal_blanking_pixels_lsb);
				uint32_t v_total = ((uint32_t)td.vertical_active_lines_msb << 8 | td.vertical_active_lines_lsb) + ((uint32_t)td.vertical_blanking_lines_msb << 8 | td.vertical_blanking_lines_lsb);
				uint8_t refreshRate = (uint8_t)round((float)pixel_clock / (float)(h_total * v_total));
				if (refreshRate > highestRefreshRate) highestRefreshRate = refreshRate;
			}
			snprintf(Docked_c, sizeof(Docked_c), "확인된 최대 주사율: %d Hz", highestRefreshRate);
		}
		setsysExit();
		smExit();
	}

	size_t base_height = 128;
	bool block = false;

    virtual tsl::elm::Element* createUI() override {
        auto frame = new tsl::elm::OverlayFrame("FPSLocker", "독 모드 디스플레이 설정");

		auto list = new tsl::elm::List();

		list->addItem(new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {

			renderer->drawString(Docked_c, false, x, y+20, 19, renderer->a(0xFFFF));
			
		}), 65);

		auto *clickableListItem1 = new tsl::elm::ListItem2("주사율 허용 범위");
		clickableListItem1->setClickListener([this](u64 keys) { 
			if ((keys & HidNpadButton_A) && !block) {
				tsl::changeTo<DockedManualGui>();
				return true;
			}
			return false;
		});

		list->addItem(clickableListItem1);

		auto *clickableListItem2 = new tsl::elm::ListItem2("주사율 마법사");
		clickableListItem2->setClickListener([this](u64 keys) { 
			if ((keys & HidNpadButton_A) && !block) {
				tsl::changeTo<DockedWizardGui>();
				return true;
			}
			return false;
		});

		list->addItem(clickableListItem2);

		auto *clickableListItem3 = new tsl::elm::ListItem2("프레임 스킵 테스터");
		clickableListItem3->setClickListener([this](u64 keys) { 
			if ((keys & HidNpadButton_A)) {
				tsl::changeTo<DockedFrameskipGui>();
				return true;
			}
			return false;
		});

		list->addItem(clickableListItem3);


		auto *clickableListItem4 = new tsl::elm::ListItem2("추가 설정");
		clickableListItem4->setClickListener([this](u64 keys) { 
			if ((keys & HidNpadButton_A) && !block) {
				tsl::changeTo<DockedAdditionalGui>();
				return true;
			}
			return false;
		});

		list->addItem(clickableListItem4);
		
		frame->setContent(list);

        return frame;
    }

	virtual void update() override {
		if (!block) tsl::hlp::doWithSmSession([this]{
			if (R_SUCCEEDED(apmInitialize())) {
				ApmPerformanceMode mode = ApmPerformanceMode_Invalid;
				apmGetPerformanceMode(&mode);
				if (mode != ApmPerformanceMode_Boost ) {
					block = true;
					snprintf(Docked_c, sizeof(Docked_c),	"독에 연결되어 있지 않습니다.\n"
															"본체를 독에 연결한 뒤, 다시 시작하세요.\n");
				}
				apmExit();
			}
		});
	}
};

class DockedRefreshRateChangeGui : public tsl::Gui {
public:
	DockedModeRefreshRateAllowed rr;
	DockedAdditionalSettings as;
	DockedRefreshRateChangeGui () {
		LoadDockedModeAllowedSave(rr, as);
	}

	// Called when this Gui gets loaded to create the UI
	// Allocate all elements on the heap. libtesla will make sure to clean them up when not needed anymore
	virtual tsl::elm::Element* createUI() override {
		// A OverlayFrame is the base element every overlay consists of. This will draw the default Title and Subtitle.
		// If you need more information in the header or want to change it's look, use a HeaderOverlayFrame.
		auto frame = new tsl::elm::OverlayFrame("FPSLocker", "주사율 변경");

		// A list that can contain sub elements and handles scrolling
		auto list = new tsl::elm::List();

		for (size_t i = 0; i < sizeof(rr); i++) {
			if (rr[i] == false)
				continue;
			char Hz[] = "254 Hz";
			snprintf(Hz, sizeof(Hz), "%d Hz", DockedModeRefreshRateAllowedValues[i]);
			auto *clickableListItem = new tsl::elm::MiniListItem(Hz);
			clickableListItem->setClickListener([this, i](u64 keys) { 
				if (keys & HidNpadButton_A) {
					if (!oldSalty) {
						if (R_SUCCEEDED(SaltySD_Connect())) {
							SaltySD_SetDisplayRefreshRate(DockedModeRefreshRateAllowedValues[i]);
							SaltySD_Term();
							refreshRate_g = DockedModeRefreshRateAllowedValues[i];
						}
					}
					tsl::goBack();
					return true;
				}
				return false;
			});

			list->addItem(clickableListItem);
		}

		frame->setContent(list);

		return frame;
	}

	// Called once every frame to handle inputs not handled by other UI elements
	virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
		smInitialize();
		if (R_SUCCEEDED(apmInitialize())) {
			ApmPerformanceMode mode = ApmPerformanceMode_Invalid;
			apmGetPerformanceMode(&mode);
			apmExit();
			if (mode != ApmPerformanceMode_Boost) {
				smExit();
				tsl::goBack();
				return true;
			}
		}
		smExit();
		return false;   // Return true here to singal the inputs have been consumed
	}
};

class DisplayGui : public tsl::Gui {
private:
	char refreshRate_c[48] = "";
	char oled_c[64] = "휴대모드의 OLED 기기에서는 사용불가.\n";
	bool isDocked = false;
	ApmPerformanceMode entry_mode = ApmPerformanceMode_Invalid;
public:
    DisplayGui() {
		if (isLite) entry_mode = ApmPerformanceMode_Normal;
		else {
			smInitialize();
			if (R_SUCCEEDED(apmInitialize())) {
				apmGetPerformanceMode(&entry_mode);
				apmExit();
			}
			else entry_mode = ApmPerformanceMode_Normal;
			smExit();
		}
	}
	size_t base_height = 128;

    virtual tsl::elm::Element* createUI() override {
        auto frame = new tsl::elm::OverlayFrame("FPSLocker", "디스플레이 설정");

		auto list = new tsl::elm::List();

		list->addItem(new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {

			renderer->drawString(refreshRate_c, false, x, y+20, 20, renderer->a(0xFFFF));
			if (isOLED && !isDocked) renderer->drawString(oled_c, false, x, y+50, 19, renderer->a(0xFFFF));
			
		}), 90);

		if (!displaySync) {
			if (entry_mode == ApmPerformanceMode_Normal) {
				auto *clickableListItem = new tsl::elm::ListItem2("주사율 ");
				clickableListItem->setClickListener([this](u64 keys) { 
					if ((keys & HidNpadButton_A) && (!isOLED || isDocked)) {
						if ((refreshRate_g >= 40) && (refreshRate_g < 60)) {
							if (R_SUCCEEDED(SaltySD_Connect())) {
								refreshRate_g += 5;
								SaltySD_SetDisplayRefreshRate(refreshRate_g);
								SaltySD_Term();
								if (Shared) (Shared -> displaySync) = refreshRate_g;
							}
						}
						return true;
					}
					return false;
				});

				list->addItem(clickableListItem);

				auto *clickableListItem2 = new tsl::elm::ListItem2("주사율 ");
				clickableListItem2->setClickListener([this](u64 keys) { 
					if ((keys & HidNpadButton_A) && (!isOLED || isDocked)) {
						if (refreshRate_g > 40) {
							if (R_SUCCEEDED(SaltySD_Connect())) {
								refreshRate_g -= 5;
								SaltySD_SetDisplayRefreshRate(refreshRate_g);
								if (Shared) (Shared -> displaySync) = refreshRate_g;
								SaltySD_Term();
							}
						}
						return true;
					}
					return false;
				});

				list->addItem(clickableListItem2);
			}
			else if (entry_mode == ApmPerformanceMode_Boost) {
				auto *clickableListItem2 = new tsl::elm::ListItem2("주사율 변경");
				clickableListItem2->setClickListener([](u64 keys) { 
					if (keys & HidNpadButton_A) {
						tsl::changeTo<DockedRefreshRateChangeGui>();
						return true;
					}
					return false;
				});	
				list->addItem(clickableListItem2);	
			}
		}

		if (!oldSalty) {
			list->addItem(new tsl::elm::CategoryHeader("FPS 타겟과 주사율 동기화.", true));
			auto *clickableListItem3 = new tsl::elm::ToggleListItem("디스플레이 동기화", displaySync);
			clickableListItem3->setClickListener([this](u64 keys) { 
				if (keys & HidNpadButton_A) {
					if (R_SUCCEEDED(SaltySD_Connect())) {
						SaltySD_SetDisplaySync(!displaySync);
						svcSleepThread(100'000);
						if (!isOLED || entry_mode == ApmPerformanceMode_Boost) {
							u64 PID = 0;
							Result rc = pmdmntGetApplicationProcessId(&PID);
							if (R_SUCCEEDED(rc) && Shared) {
								if (!displaySync == true && (Shared -> FPSlocked) < 40) {
									SaltySD_SetDisplayRefreshRate(60);
									(Shared -> displaySync) = 0;
								}
								else if (!displaySync == true) {
									SaltySD_SetDisplayRefreshRate((Shared -> FPSlocked));
									(Shared -> displaySync) = (Shared -> FPSlocked);
								}
								else {
									(Shared -> displaySync) = 0;
								}
							}
							else if (!displaySync == true && (R_FAILED(rc) || !PluginRunning)) {
								SaltySD_SetDisplayRefreshRate(60);
							}
						}
						SaltySD_Term();
						displaySync = !displaySync;
					}
					tsl::goBack();
					tsl::changeTo<DisplayGui>();
					return true;
				}
				return false;
			});

			list->addItem(clickableListItem3);

			if (!isLite) {
				auto *clickableListItem4 = new tsl::elm::ListItem2("독 모드 설정");
				clickableListItem4->setClickListener([this](u64 keys) { 
					if ((keys & HidNpadButton_A)) {
						tsl::changeTo<DockedGui>();
						return true;
					}
					return false;
				});

				list->addItem(clickableListItem4);
			}
		}
		
		frame->setContent(list);

        return frame;
    }

	virtual void update() override {
		refreshRate_g = *refreshRate_shared;
		snprintf(refreshRate_c, sizeof(refreshRate_c), "디스플레이 주사율: %d Hz", refreshRate_g);
	}

	// Called once every frame to handle inputs not handled by other UI elements
	virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
		if (!isLite) {
			smInitialize();
			if (R_SUCCEEDED(apmInitialize())) {
				ApmPerformanceMode mode = ApmPerformanceMode_Invalid;
				apmGetPerformanceMode(&mode);
				apmExit();
				if (mode != entry_mode) {
					smExit();
					tsl::goBack();
					tsl::changeTo<DisplayGui>();
					return true;
				}
			}
			smExit();
		}
		return false;   // Return true here to singal the inputs have been consumed
	}
};

class WarningDisplayGui : public tsl::Gui {
private:
	uint8_t refreshRate = 0;
	std::string Warning =	"경고: 실험적인 기능입니다!\n\n"
							"디스플레이에 복구 불가한 치명적인\n"
							"물리적 손상을 초래할 수 있습니다.\n\n"
							"동의 버튼 선택으로 발생할 수 있는\n"
							"모든 문제에 대한 책임은 전적으로\n"
							"사용자 본인에게 있음을 알립니다.\n\n"
							"정말 해당 기능을 사용하시겠습니까?";
public:
    WarningDisplayGui() {}

	size_t base_height = 158;

    virtual tsl::elm::Element* createUI() override {
        auto frame = new tsl::elm::OverlayFrame("FPSLocker", "디스플레이 설정 경고");

		auto list = new tsl::elm::List();

		list->addItem(new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
			renderer->drawString(Warning.c_str(), false, x, y+20, 18, renderer->a(0xFFFF));
		}), 200);

		auto *clickableListItem1 = new tsl::elm::ListItem2("거부");
		clickableListItem1->setClickListener([this](u64 keys) { 
			if (keys & HidNpadButton_A) {
				tsl::goBack();
				return true;
			}
			return false;
		});

		list->addItem(clickableListItem1);

		auto *clickableListItem2 = new tsl::elm::ListItem2("동의");
		clickableListItem2->setClickListener([this](u64 keys) { 
			if ((keys & HidNpadButton_A)) {
				tsl::goBack();
				tsl::changeTo<DisplayGui>();
				return true;
			}
			return false;
		});

		list->addItem(clickableListItem2);
		
		frame->setContent(list);

        return frame;
    }
};