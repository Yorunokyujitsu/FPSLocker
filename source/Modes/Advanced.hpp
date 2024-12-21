class SetBuffers : public tsl::Gui {
public:
    SetBuffers() {}

    virtual tsl::elm::Element* createUI() override {
		auto frame = new tsl::elm::OverlayFrame("NVN 버퍼링 설정", " ");

		auto list = new tsl::elm::List();
		list->addItem(new tsl::elm::CategoryHeader("타이틀 재기동 시 적용됩니다", false));
		list->addItem(new tsl::elm::NoteHeader("변경 후 설정을 저장하세요", true, {0xF, 0x3, 0x3, 0xF}));
		auto *clickableListItem = new tsl::elm::ListItem2("Double");
		clickableListItem->setClickListener([](u64 keys) { 
			if ((keys & HidNpadButton_A) && PluginRunning) {
				SetBuffers_save = 2;
				tsl::goBack();
				return true;
			}
			return false;
		});
		list->addItem(clickableListItem);

		if ((Shared -> SetActiveBuffers) == 2 && (Shared -> Buffers) == 3 && !SetBuffers_save) {
			auto *clickableListItem2 = new tsl::elm::ListItem2("Triple (force)");
			clickableListItem2->setClickListener([](u64 keys) { 
				if ((keys & HidNpadButton_A) && PluginRunning) {
					SetBuffers_save = 3;
					tsl::goBack();
					return true;
				}
				return false;
			});
			list->addItem(clickableListItem2);
		}
		else {
			auto *clickableListItem2 = new tsl::elm::ListItem2("Triple");
			clickableListItem2->setClickListener([](u64 keys) { 
				if ((keys & HidNpadButton_A) && PluginRunning) {
					if ((Shared -> Buffers) == 4) SetBuffers_save = 3;
					else SetBuffers_save = 0;
					tsl::goBack();
					return true;
				}
				return false;
			});
			list->addItem(clickableListItem2);
		}
		
		if ((Shared -> Buffers) == 4) {
			if ((Shared -> SetActiveBuffers) < 4 && (Shared -> SetActiveBuffers) > 0 && (Shared -> Buffers) == 4) {
				auto *clickableListItem3 = new tsl::elm::ListItem2("Quadruple (force)");
				clickableListItem3->setClickListener([](u64 keys) { 
					if ((keys & HidNpadButton_A) && PluginRunning) {
						SetBuffers_save = 4;
						tsl::goBack();
						return true;
					}
					return false;
				});
				list->addItem(clickableListItem3);	
			}
			else {
				auto *clickableListItem3 = new tsl::elm::ListItem2("Quadruple");
				clickableListItem3->setClickListener([](u64 keys) { 
					if ((keys & HidNpadButton_A) && PluginRunning) {
						SetBuffers_save = 0;
						tsl::goBack();
						return true;
					}
					return false;
				});
				list->addItem(clickableListItem3);
			}
		}

		frame->setContent(list);

        return frame;
    }
};

class SyncMode : public tsl::Gui {
public:
    SyncMode() {}

    virtual tsl::elm::Element* createUI() override {
        auto frame = new tsl::elm::OverlayFrame("NVN 창 동기화 대기", "모드");

		auto list = new tsl::elm::List();

		auto *clickableListItem = new tsl::elm::ListItem2("활성화");
		clickableListItem->setClickListener([](u64 keys) { 
			if ((keys & HidNpadButton_A) && PluginRunning) {
				ZeroSyncMode = "On";
				(Shared -> ZeroSync) = 0;
				tsl::goBack();
				tsl::goBack();
				return true;
			}
			return false;
		});
		list->addItem(clickableListItem);

		auto *clickableListItem2 = new tsl::elm::ListItem2("준활성화");
		clickableListItem2->setClickListener([](u64 keys) { 
			if ((keys & HidNpadButton_A) && PluginRunning) {
				ZeroSyncMode = "Semi";
				(Shared -> ZeroSync) = 2;
				tsl::goBack();
				tsl::goBack();
				return true;
			}
			return false;
		});
		list->addItem(clickableListItem2);

		auto *clickableListItem3 = new tsl::elm::ListItem2("비활성화");
		clickableListItem3->setClickListener([](u64 keys) { 
			if ((keys & HidNpadButton_A) && PluginRunning) {
				ZeroSyncMode = "Off";
				(Shared -> ZeroSync) = 1;
				tsl::goBack();
				tsl::goBack();
				return true;
			}
			return false;
		});
		list->addItem(clickableListItem3);
		
        frame->setContent(list);

        return frame;
    }
};

class AdvancedGui : public tsl::Gui {
public:
	bool exitPossible = true;
    AdvancedGui() {
		configValid = LOCK::readConfig(&configPath[0]);
		if (R_FAILED(configValid)) {
			if (configValid == 0x202) {
				sprintf(&lockInvalid[0], "게임 설정 파일이 없습니다!\nTID: %016lX\nBID: %016lX", TID, BID);
			}
			else sprintf(&lockInvalid[0], "게임 설정 오류: 0x%X", configValid);
		}
		else {
			patchValid = checkFile(&patchPath[0]);
			if (R_FAILED(patchValid)) {
				if (!FileDownloaded) {
					if (R_SUCCEEDED(configValid)) {
						sprintf(&patchChar[0], "패치 파일이 없습니다!\n\"구성을 패치 파일로 변환\"\n하여 사용하세요");
					}
					else sprintf(&patchChar[0], "패치 파일이 없습니다!");
				}
				else {
					sprintf(&patchChar[0], "새 구성 다운로드 성공!\n\"구성을 패치 파일로 변환\"\n후 적용하세요");
				}
			}
			else sprintf(&patchChar[0], "패치 파일이 있습니다!");
		}
		switch((Shared -> ZeroSync)) {
			case 0:
				ZeroSyncMode = "On";
				break;
			case 1:
				ZeroSyncMode = "Off";
				break;
			case 2:
				ZeroSyncMode = "Semi";
		}
	}

	size_t base_height = 134;

    virtual tsl::elm::Element* createUI() override {
        auto frame = new tsl::elm::OverlayFrame("FPSLocker", "고급 설정");

		auto list = new tsl::elm::List();

		if ((Shared -> API)) {
			switch((Shared -> API)) {
				case 1: {
					list->addItem(new tsl::elm::CategoryHeader("GPU API 인터페이스: NVN", false));
					
					list->addItem(new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
						
						renderer->drawString(&nvnBuffers[0], false, x, y+20, 20, renderer->a(0xFFFF));
							
					}), 40);

					if ((Shared -> Buffers) == 2 || (Shared -> SetBuffers) == 2 || (Shared -> ActiveBuffers) == 2) {
						auto *clickableListItem3 = new tsl::elm::MiniListItem("창 동기화 대기", ZeroSyncMode);
						clickableListItem3->setClickListener([](u64 keys) { 
							if ((keys & HidNpadButton_A) && PluginRunning) {
								tsl::changeTo<SyncMode>();
								return true;
							}
							return false;
						});
						list->addItem(clickableListItem3);
					}
					if ((Shared -> Buffers) > 2) {
						auto *clickableListItem3 = new tsl::elm::MiniListItem("버퍼링 설정");
						clickableListItem3->setClickListener([](u64 keys) { 
							if ((keys & HidNpadButton_A) && PluginRunning) {
								tsl::changeTo<SetBuffers>();
								return true;
							}
							return false;
						});
						list->addItem(clickableListItem3);
					}
					break;
				}
				case 2:
					list->addItem(new tsl::elm::CategoryHeader("GPU API 인터페이스: EGL", false));
					break;
				case 3:
					list->addItem(new tsl::elm::CategoryHeader("GPU API 인터페이스: Vulkan", false));
			}
		}

		list->addItem(new tsl::elm::CategoryHeader("FPSLocker 패치", false));

		if (R_FAILED(configValid)) {
			base_height = 154;
		}

		list->addItem(new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
			
			if (R_SUCCEEDED(configValid)) {
				
				renderer->drawString("유효한 설정 파일 발견!", false, x, y+20, 20, renderer->a(0xFFFF));
				renderer->drawString(&patchAppliedChar[0], false, x, y+40, 20, renderer->a(0xFFFF));
				if (R_FAILED(patchValid)) {
					renderer->drawString(&patchChar[0], false, x, y+64, 20, renderer->a(0xF99F));
				}
				else renderer->drawString(&patchChar[0], false, x, y+64, 20, renderer->a(0xFFFF));
			}
			else {
				renderer->drawString(&lockInvalid[0], false, x, y+20, 20, renderer->a(0xFFFF));
				renderer->drawString(&patchChar[0], false, x, y+84, 20, renderer->a(0xF99F));
			}
				

		}), base_height);

		if (R_SUCCEEDED(configValid)) {
			list->addItem(new tsl::elm::NoteHeader("변경 후 설정을 저장하세요!", true, {0xF, 0x3, 0x3, 0xF}));
			auto *clickableListItem = new tsl::elm::MiniListItem("설정을 패치 파일로 변환");
			clickableListItem->setClickListener([](u64 keys) { 
				if ((keys & HidNpadButton_A) && PluginRunning) {
					patchValid = LOCK::createPatch(&patchPath[0]);
					if (R_SUCCEEDED(patchValid)) {
						sprintf(&patchChar[0], "패치 생성 완료!\n게임 재시작 후 FPS를,\n변경하여 패치 적용하세요");
					}
					else sprintf(&patchChar[0], "패치 생성 오류: 0x%x", patchValid);
					return true;
				}
				return false;
			});
			list->addItem(clickableListItem);

			auto *clickableListItem2 = new tsl::elm::MiniListItem("패치 파일 제거");
			clickableListItem2->setClickListener([](u64 keys) { 
				if ((keys & HidNpadButton_A) && PluginRunning) {
					if (R_SUCCEEDED(patchValid)) {
						remove(&patchPath[0]);
						patchValid = 0x202;
						sprintf(&patchChar[0], "패치 파일 제거 완료!");
					}
					return true;
				}
				return false;
			});
			list->addItem(clickableListItem2);
		}
		if (R_FAILED(configValid)) {
			list->addItem(new tsl::elm::NoteHeader("이 작업은 30초 정도 소요됩니다!", true, {0xF, 0x3, 0x3, 0xF}));
		}
		auto *clickableListItem4 = new tsl::elm::MiniListItem("설정 파일 확인/다운로드");
		clickableListItem4->setClickListener([this](u64 keys) { 
			if ((keys & HidNpadButton_A) && PluginRunning && exitPossible) {
				exitPossible = false;
				sprintf(&patchChar[0], "Warehouse 확인 중...\n완료될 때까지 종료 불가합니다!");
				threadCreate(&t1, downloadPatch, NULL, NULL, 0x20000, 0x3F, 3);
				threadStart(&t1);
				return true;
			}
			return false;
		});
		list->addItem(clickableListItem4);

		frame->setContent(list);

        return frame;
    }

	virtual void update() override {
		static uint8_t i = 10;

		if (PluginRunning) {
			if (i > 9) {
				if ((Shared -> patchApplied) == 1) {
					sprintf(patchAppliedChar, "게임 패치 로드 완료!");
				}
				else if ((Shared -> patchApplied) == 2) {
					sprintf(patchAppliedChar, "마스터 쓰기가 게임에 로드 됨");
				}
				else sprintf(patchAppliedChar, "게임 패치 플러그인 동작 실패!");
				if ((Shared -> API) == 1) {
					if (((Shared -> Buffers) >= 2 && (Shared -> Buffers) <= 4)) {
						sprintf(&nvnBuffers[0], "설정/활성화/사용 가능한 버퍼: %d/%d/%d", (Shared -> SetActiveBuffers), (Shared -> ActiveBuffers), (Shared -> Buffers));
					}
				}
				i = 0;
			}
			else i++;
		}
	}

    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
		if (exitPossible) {
			if (keysDown & HidNpadButton_B) {
				tsl::goBack();
				return true;
			}
		}
		else if (!exitPossible) {
			if (keysDown & HidNpadButton_B)
				return true;
			Result rc = error_code;
			if (rc != UINT32_MAX && rc != 0x404) {
				threadWaitForExit(&t1);
				threadClose(&t1);
				exitPossible = true;
				error_code = UINT32_MAX;
			}
			if (rc == 0x316) {
				sprintf(&patchChar[0], "연결 시간 초과!");
			}
			else if (rc == 0x212 || rc == 0x312) {
				sprintf(&patchChar[0], "설정 사용 불가! RC: 0x%x", rc);
			}
			else if (rc == 0x404) {
				sprintf(&patchChar[0], "설정 사용 불가!\n상세 내용은 Warehouse 참고...\n완료까지 종료 불가!");
			}
			else if (rc == 0x405) {
				sprintf(&patchChar[0], "설정 사용 불가!\n상세 내용은 Warehouse 참고...\n시간 초과!");
			}
			else if (rc == 0x406) {
				sprintf(&patchChar[0], "설정 사용 불가!\n상세 내용은 Warehouse 참고...\n연결 오류!");
			}
			else if (rc == 0x104) {
				sprintf(&patchChar[0], "새 설정 사용 불가!");
			}
			else if (rc == 0x412) {
				sprintf(&patchChar[0], "인터넷 연결 불가!");
			}
			else if (rc == 0x1001) {
				sprintf(&patchChar[0], "이 게임은 패치가 필요하지 않습니다!");
			}
			else if (rc == 0x1002) {
				sprintf(&patchChar[0], "Warehouse 미등록 게임입니다!");
			}
			else if (rc == 0x1003) {
				sprintf(&patchChar[0], "Warehouse에 등록었으나,\n게임 버전이 다릅니다!\n다른 버전에서도 패치가,\n필요하지 않을 수 있습니다!");
			}
			else if (rc == 0x1004) {
				sprintf(&patchChar[0], "Warehouse에 등록었으나,\n게임 버전이 다릅니다!\n다른 버전에서 패치가 권장되지만,\n해당 버전은 사용 불가합니다!");
			}
			else if (rc == 0x1005) {
				sprintf(&patchChar[0], "Warehouse에 등록었으나,\n게임 버전이 다릅니다!\n다른 버전에서 사용 가능!");
			}
			else if (rc == 0x1006) {
				sprintf(&patchChar[0], "해당 게임과 버전은 Warehouse에\n권장 패치와 함께 등록되었지만,\n이 설정은 사용 불가합니다!");
			}
			else if (R_SUCCEEDED(rc)) {
				FILE* fp = fopen(patchPath, "rb");
				if (fp) {
					fclose(fp);
					remove(patchPath);
				}
				tsl::goBack();
				tsl::changeTo<AdvancedGui>();
				return true;
			}
			else if (rc != UINT32_MAX) {
				sprintf(&patchChar[0], "연결 오류! RC: 0x%x", rc);
			}
		}
        return false;   // Return true here to signal the inputs have been consumed
    }
};