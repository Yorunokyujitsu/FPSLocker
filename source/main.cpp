#define TESLA_INIT_IMPL // If you have more than one file using the tesla header, only define this in the main one
#include <tesla.hpp>    // The Tesla Header
//#include "MiniList.hpp"
#include "NoteHeader.hpp"
#include <sys/stat.h>
#include <dirent.h>
#include "SaltyNX.h"
#include "Lock.hpp"
#include "Utils.hpp"

bool displaySync = false;
bool isOLED = false;
uint8_t refreshRate_g = 60;
bool oldSalty = false;
ApmPerformanceMode performanceMode = ApmPerformanceMode_Invalid;
bool isDocked = false;

class SetBuffers : public tsl::Gui {
public:
    SetBuffers() {}

    virtual tsl::elm::Element* createUI() override {
		auto frame = new tsl::elm::OverlayFrame("NVN 버퍼링 설정", " ");

		auto list = new tsl::elm::List();
		list->addItem(new tsl::elm::CategoryHeader("타이틀 재기동 시 적용됩니다", false));
		list->addItem(new tsl::elm::NoteHeader("변경 후 설정을 저장하세요", true, {0xF, 0x3, 0x3, 0xF}));
		auto *clickableListItem = new tsl::elm::ListItem("Double");
		clickableListItem->setClickListener([](u64 keys) { 
			if ((keys & HidNpadButton_A) && PluginRunning) {
				SetBuffers_save = 2;
				tsl::goBack();
				return true;
			}
			return false;
		});
		list->addItem(clickableListItem);

		if (*SetActiveBuffers_shared == 2 && *Buffers_shared == 3 && !SetBuffers_save) {
			auto *clickableListItem2 = new tsl::elm::ListItem("Triple (force)");
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
			auto *clickableListItem2 = new tsl::elm::ListItem("Triple");
			clickableListItem2->setClickListener([](u64 keys) { 
				if ((keys & HidNpadButton_A) && PluginRunning) {
					if (*Buffers_shared == 4) SetBuffers_save = 3;
					else SetBuffers_save = 0;
					tsl::goBack();
					return true;
				}
				return false;
			});
			list->addItem(clickableListItem2);
		}
		
		if (*Buffers_shared == 4) {
			if (*SetActiveBuffers_shared < 4 && *SetActiveBuffers_shared > 0 && *Buffers_shared == 4) {
				auto *clickableListItem3 = new tsl::elm::ListItem("Quadruple (force)");
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
				auto *clickableListItem3 = new tsl::elm::ListItem("Quadruple");
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

		auto *clickableListItem = new tsl::elm::ListItem("활성화");
		clickableListItem->setClickListener([](u64 keys) { 
			if ((keys & HidNpadButton_A) && PluginRunning) {
				ZeroSyncMode = "On";
				*ZeroSync_shared = 0;
				tsl::goBack();
				tsl::goBack();
				return true;
			}
			return false;
		});
		list->addItem(clickableListItem);

		auto *clickableListItem2 = new tsl::elm::ListItem("준활성화");
		clickableListItem2->setClickListener([](u64 keys) { 
			if ((keys & HidNpadButton_A) && PluginRunning) {
				ZeroSyncMode = "Semi";
				*ZeroSync_shared = 2;
				tsl::goBack();
				tsl::goBack();
				return true;
			}
			return false;
		});
		list->addItem(clickableListItem2);

		auto *clickableListItem3 = new tsl::elm::ListItem("비활성화");
		clickableListItem3->setClickListener([](u64 keys) { 
			if ((keys & HidNpadButton_A) && PluginRunning) {
				ZeroSyncMode = "Off";
				*ZeroSync_shared = 1;
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
		switch(*ZeroSync_shared) {
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

		if (*API_shared) {
			switch(*API_shared) {
				case 1: {
					list->addItem(new tsl::elm::CategoryHeader("GPU API 인터페이스: NVN", false));
					
					list->addItem(new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
						
						renderer->drawString(&nvnBuffers[0], false, x, y+20, 20, renderer->a(0xFFFF));
							
					}), 40);

					if (*Buffers_shared == 2 || *SetBuffers_shared == 2 || *ActiveBuffers_shared == 2) {
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
					if (*Buffers_shared > 2) {
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
				if (*patchApplied_shared == 1) {
					sprintf(patchAppliedChar, "게임 패치 로드 완료!");
				}
				else if (*patchApplied_shared == 2) {
					sprintf(patchAppliedChar, "마스터 쓰기가 게임에 로드 됨");
				}
				else sprintf(patchAppliedChar, "게임 패치 플러그인 동작 실패!");
				if (*API_shared == 1) {
					if ((*Buffers_shared >= 2 && *Buffers_shared <= 4)) {
						sprintf(&nvnBuffers[0], "설정/활성화/사용 가능한 버퍼: %d/%d/%d", *SetActiveBuffers_shared, *ActiveBuffers_shared, *Buffers_shared);
					}
				}
				i = 0;
			}
			else i++;
		}
	}

    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
		if (exitPossible) {
			if (keysHeld & HidNpadButton_B) {
				tsl::goBack();
				return true;
			}
		}
		else if (!exitPossible) {
			if (keysHeld & HidNpadButton_B)
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

class NoGameSub : public tsl::Gui {
public:
	uint64_t _titleid = 0;
	char _titleidc[17] = "";
	std::string _titleName = "";

	NoGameSub(uint64_t titleID, std::string titleName) {
		_titleid = titleID;
		sprintf(&_titleidc[0], "%016lX", _titleid);
		_titleName = titleName;
	}

	// Called when this Gui gets loaded to create the UI
	// Allocate all elements on the heap. libtesla will make sure to clean them up when not needed anymore
	virtual tsl::elm::Element* createUI() override {
		// A OverlayFrame is the base element every overlay consists of. This will draw the default Title and Subtitle.
		// If you need more information in the header or want to change it's look, use a HeaderOverlayFrame.
		auto frame = new tsl::elm::OverlayFrame(_titleidc, _titleName);

		// A list that can contain sub elements and handles scrolling
		auto list = new tsl::elm::List();

		auto *clickableListItem = new tsl::elm::ListItem("설정 제거");
		clickableListItem->setClickListener([this](u64 keys) { 
			if (keys & HidNpadButton_A) {
				char path[512] = "";
				if (_titleid != 0x1234567890ABCDEF) {
					sprintf(&path[0], "sdmc:/SaltySD/plugins/FPSLocker/%016lx.dat", _titleid);
					remove(path);
				}
				else {
					struct dirent *entry;
    				DIR *dp;
					sprintf(&path[0], "sdmc:/SaltySD/plugins/FPSLocker/");

					dp = opendir(path);
					if (!dp)
						return true;
					while ((entry = readdir(dp))) {
						if (entry -> d_type != DT_DIR && std::string(entry -> d_name).find(".dat") != std::string::npos) {
							sprintf(&path[0], "sdmc:/SaltySD/plugins/FPSLocker/%s", entry->d_name);
							remove(path);
						}
					}
					closedir(dp);
				}
				return true;
			}
			return false;
		});

		list->addItem(clickableListItem);

		auto *clickableListItem2 = new tsl::elm::ListItem("패치 제거");
		clickableListItem2->setClickListener([this](u64 keys) { 
			if (keys & HidNpadButton_A) {
				char folder[640] = "";
				if (_titleid != 0x1234567890ABCDEF) {
					sprintf(&folder[0], "sdmc:/SaltySD/plugins/FPSLocker/patches/%016lx/", _titleid);

					struct dirent *entry;
    				DIR *dp;

					dp = opendir(folder);
					if (!dp)
						return true;
					while ((entry = readdir(dp))) {
						if (entry -> d_type != DT_DIR && std::string(entry -> d_name).find(".bin") != std::string::npos) {
							sprintf(&folder[0], "sdmc:/SaltySD/plugins/FPSLocker/patches/%016lx/%s", _titleid, entry -> d_name);
							remove(folder);
						}
					}
					closedir(dp);
				}
				else {
					struct dirent *entry;
					struct dirent *entry2;
    				DIR *dp;
					DIR *dp2;

					sprintf(&folder[0], "sdmc:/SaltySD/plugins/FPSLocker/patches/");
					dp = opendir(folder);
					if (!dp)
						return true;
					while ((entry = readdir(dp))) {
						if (entry -> d_type != DT_DIR)
							continue;
						sprintf(&folder[0], "sdmc:/SaltySD/plugins/FPSLocker/patches/%s/", entry -> d_name);
						dp2 = opendir(folder);
						while ((entry2 = readdir(dp2))) {
							if (entry2 -> d_type != DT_DIR && std::string(entry2 -> d_name).find(".bin") != std::string::npos) {
								sprintf(&folder[0], "sdmc:/SaltySD/plugins/FPSLocker/patches/%s/%s", entry -> d_name, entry2 -> d_name);
								remove(folder);
							}
						}
						closedir(dp2);
					}
					closedir(dp);
				}
				return true;
			}
			return false;
		});

		list->addItem(clickableListItem2);

		frame->setContent(list);

		return frame;
	}
};

class NoGame2 : public tsl::Gui {
public:

	Result rc = 1;
	NoGame2(Result result, u8 arg2, bool arg3) {
		rc = result;
	}

	// Called when this Gui gets loaded to create the UI
	// Allocate all elements on the heap. libtesla will make sure to clean them up when not needed anymore
	virtual tsl::elm::Element* createUI() override {
		// A OverlayFrame is the base element every overlay consists of. This will draw the default Title and Subtitle.
		// If you need more information in the header or want to change it's look, use a HeaderOverlayFrame.
		auto frame = new tsl::elm::OverlayFrame("FPSLocker", APP_VERSION"-ASAP");

		// A list that can contain sub elements and handles scrolling
		auto list = new tsl::elm::List();

		if (oldSalty || isOLED || !SaltySD) {
			list->addItem(new tsl::elm::CustomDrawer([](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
				if (!SaltySD) {
					renderer->drawString("SaltyNX가 작동하지 않습니다!", false, x, y+20, 20, renderer->a(0xF33F));
				}
				else if (!plugin) {
					renderer->drawString("SD 카드의 NX-FPS 플러그인 감지 실패!", false, x, y+20, 20, renderer->a(0xF33F));
				}
				else if (!check) {
					renderer->drawString("실행된 게임이 없습니다!", false, x, y+20, 19, renderer->a(0xF33F));
				}
			}), 30);
		}

		if (R_FAILED(rc)) {
			char error[24] = "";
			sprintf(&error[0], "오류: 0x%x", rc);
			auto *clickableListItem2 = new tsl::elm::ListItem(error);
			clickableListItem2->setClickListener([](u64 keys) { 
				if (keys & HidNpadButton_A) {
					return true;
				}
				return false;
			});

			list->addItem(clickableListItem2);
		}
		else {
			auto *clickableListItem3 = new tsl::elm::ListItem("All");
			clickableListItem3->setClickListener([](u64 keys) { 
				if (keys & HidNpadButton_A) {
					tsl::changeTo<NoGameSub>(0x1234567890ABCDEF, "Everything");
					return true;
				}
				return false;
			});

			list->addItem(clickableListItem3);

			for (size_t i = 0; i < titles.size(); i++) {
				auto *clickableListItem = new tsl::elm::ListItem(titles[i].TitleName);
				clickableListItem->setClickListener([i](u64 keys) { 
					if (keys & HidNpadButton_A) {
						tsl::changeTo<NoGameSub>(titles[i].TitleID, titles[i].TitleName);
						return true;
					}
					return false;
				});

				list->addItem(clickableListItem);
			}
		}

		frame->setContent(list);

		return frame;
	}

	virtual void update() override {}

	// Called once every frame to handle inputs not handled by other UI elements
	virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
		return false;   // Return true here to singal the inputs have been consumed
	}
};

class DisplayGui : public tsl::Gui {
private:
	char refreshRate_c[32] = "";
	uint8_t refreshRate = 0;
public:
    DisplayGui() {
		apmGetPerformanceMode(&performanceMode);
		if (performanceMode == ApmPerformanceMode_Boost) {
			isDocked = true;
		}
		else if (performanceMode == ApmPerformanceMode_Normal) {
			isDocked = false;
		}
		if (!isDocked && R_SUCCEEDED(SaltySD_Connect())) {
			SaltySD_GetDisplayRefreshRate(&refreshRate);
			svcSleepThread(100'000);
			SaltySD_Term();
			refreshRate_g = refreshRate;
		}
	}

	size_t base_height = 128;

    virtual tsl::elm::Element* createUI() override {
        auto frame = new tsl::elm::OverlayFrame("FPSLocker", "디스플레이 설정");

		auto list = new tsl::elm::List();

		list->addItem(new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {

			renderer->drawString(this -> refreshRate_c, false, x, y+20, 20, renderer->a(0xFFFF));
			
		}), 50);

		if (!displaySync) {

			auto *clickableListItem = new tsl::elm::ListItem("새로고침 빈도 UP");
			clickableListItem->setClickListener([this](u64 keys) { 
				if ((keys & HidNpadButton_A) && !isDocked) {
					if ((this -> refreshRate >= 40) && (this -> refreshRate < 60)) {
						this -> refreshRate += 5;
						if (!isDocked && R_SUCCEEDED(SaltySD_Connect())) {
							SaltySD_SetDisplayRefreshRate(this -> refreshRate);
							svcSleepThread(100'000);
							SaltySD_GetDisplayRefreshRate(&(this -> refreshRate));
							if (displaySync_shared)
								*displaySync_shared = this -> refreshRate;
							SaltySD_Term();
							refreshRate_g = this -> refreshRate;
						}
					}
					return true;
				}
				return false;
			});

			list->addItem(clickableListItem);

			auto *clickableListItem2 = new tsl::elm::ListItem("새로고침 빈도 DOWN");
			clickableListItem2->setClickListener([this](u64 keys) { 
				if ((keys & HidNpadButton_A) && !isDocked) {
					if (this -> refreshRate > 40) {
						this -> refreshRate -= 5;
						if (!isDocked && R_SUCCEEDED(SaltySD_Connect())) {
							SaltySD_SetDisplayRefreshRate(this -> refreshRate);
							svcSleepThread(100'000);
							SaltySD_GetDisplayRefreshRate(&(this -> refreshRate));
							if (displaySync_shared)
								*displaySync_shared = this -> refreshRate;
							SaltySD_Term();
							refreshRate_g = this -> refreshRate;
						}
					}
					return true;
				}
				return false;
			});

			list->addItem(clickableListItem2);
		}

		if (!oldSalty) {
			list->addItem(new tsl::elm::CategoryHeader("FPS 및 새로고침 빈도 동기화", true));
			auto *clickableListItem3 = new tsl::elm::ToggleListItem("디스플레이 동기화", displaySync);
			clickableListItem3->setClickListener([this](u64 keys) { 
				if (keys & HidNpadButton_A) {
					if (R_SUCCEEDED(SaltySD_Connect())) {
						SaltySD_SetDisplaySync(!displaySync);
						svcSleepThread(100'000);
						u64 PID = 0;
						Result rc = pmdmntGetApplicationProcessId(&PID);
						if (!isDocked && R_SUCCEEDED(rc) && FPSlocked_shared) {
							if (!displaySync == true && *FPSlocked_shared < 40) {
								SaltySD_SetDisplayRefreshRate(60);
								*displaySync_shared = 0;
								refreshRate_g = 0;
							}
							else if (!displaySync == true) {
								SaltySD_SetDisplayRefreshRate(*FPSlocked_shared);
								*displaySync_shared = *FPSlocked_shared;
								refreshRate_g = *FPSlocked_shared;
							}
							else {
								*displaySync_shared = 0;
								refreshRate_g = 0;
							}
						}
						else if (!isDocked && !displaySync == true && (R_FAILED(rc) || !PluginRunning)) {
							SaltySD_SetDisplayRefreshRate(60);
							refreshRate_g = 0;
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
		}
		
		frame->setContent(list);

        return frame;
    }

	virtual void update() override {
		apmGetPerformanceMode(&performanceMode);
		if (performanceMode == ApmPerformanceMode_Boost) {
			isDocked = true;
		}
		else if (performanceMode == ApmPerformanceMode_Normal) {
			isDocked = false;
		}
		if (!isDocked)
			snprintf(refreshRate_c, sizeof(refreshRate_c), "LCD 재생률: %d Hz", refreshRate);
		else strncpy(refreshRate_c, "독 모드 이용 불가!", 30);
	}
};

class WarningDisplayGui : public tsl::Gui {
private:
	uint8_t refreshRate = 0;
	std::string Warning =	"실험적 기능입니다!\n\n"
							"디스플레이에 복구 불가한\n"
							"손상을 초래할 수 있습니다.\n\n"
							"수락을 선택하여 발생할 수 있는\n"
							"모든 문제에 대하여 전적으로\n"
							"책임은 본인에게 있습니다.";

	std::string Docked =	"독 모드에서는 해당 기능을\n"
							"사용할 수 없습니다!\n\n"
							"수락 버튼 비활성화 상태";
public:
    WarningDisplayGui() {
		apmGetPerformanceMode(&performanceMode);
		if (performanceMode == ApmPerformanceMode_Boost) {
			isDocked = true;
		}
		else if (performanceMode == ApmPerformanceMode_Normal) {
			isDocked = false;
		}
	}

	size_t base_height = 128;

    virtual tsl::elm::Element* createUI() override {
        auto frame = new tsl::elm::OverlayFrame("FPSLocker", "디스플레이 설정 경고");

		auto list = new tsl::elm::List();

		list->addItem(new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
			if (!isDocked)
				renderer->drawString(Warning.c_str(), false, x, y+20, 20, renderer->a(0xFFFF));
			else renderer->drawString(Docked.c_str(), false, x, y+20, 20, renderer->a(0xFFFF));
		}), 200);

		auto *clickableListItem1 = new tsl::elm::ListItem("거부");
		clickableListItem1->setClickListener([this](u64 keys) { 
			if (keys & HidNpadButton_A) {
				tsl::goBack();
				return true;
			}
			return false;
		});

		list->addItem(clickableListItem1);

		auto *clickableListItem2 = new tsl::elm::ListItem("수락");
		clickableListItem2->setClickListener([this](u64 keys) { 
			if ((keys & HidNpadButton_A) && !isDocked) {
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

class NoGame : public tsl::Gui {
public:

	Result rc = 1;
	NoGame(Result result, u8 arg2, bool arg3) {
		rc = result;
	}

	// Called when this Gui gets loaded to create the UI
	// Allocate all elements on the heap. libtesla will make sure to clean them up when not needed anymore
	virtual tsl::elm::Element* createUI() override {
		// A OverlayFrame is the base element every overlay consists of. This will draw the default Title and Subtitle.
		// If you need more information in the header or want to change it's look, use a HeaderOverlayFrame.
		auto frame = new tsl::elm::OverlayFrame("FPSLocker", APP_VERSION"-ASAP");

		// A list that can contain sub elements and handles scrolling
		auto list = new tsl::elm::List();

		list->addItem(new tsl::elm::CustomDrawer([](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
			if (!SaltySD) {
				renderer->drawString("SaltyNX가 작동하지 않습니다!", false, x, y+20, 20, renderer->a(0xF33F));
			}
			else if (!plugin) {
				renderer->drawString("SD 카드의 NX-FPS 플러그인 감지 실패!", false, x, y+20, 20, renderer->a(0xF33F));
			}
			else if (!check) {
				renderer->drawString("실행된 게임이 없습니다!", false, x, y+20, 19, renderer->a(0xF33F));
			}
		}), 30);

		auto *clickableListItem2 = new tsl::elm::ListItem("게임 리스트");
		clickableListItem2->setClickListener([this](u64 keys) { 
			if (keys & HidNpadButton_A) {
				tsl::changeTo<NoGame2>(this -> rc, 2, true);
				return true;
			}
			return false;
		});

		list->addItem(clickableListItem2);

		auto *clickableListItem3 = new tsl::elm::ListItem("디스플레이 설정", "\uE151");
		clickableListItem3->setClickListener([](u64 keys) { 
			if (keys & HidNpadButton_A) {
				tsl::changeTo<WarningDisplayGui>();
				return true;
			}
			return false;
		});

		list->addItem(clickableListItem3);


		frame->setContent(list);

		return frame;
	}

	virtual void update() override {}

	// Called once every frame to handle inputs not handled by other UI elements
	virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
		return false;   // Return true here to singal the inputs have been consumed
	}
};

class GuiTest : public tsl::Gui {
public:
	GuiTest(u8 arg1, u8 arg2, bool arg3) { }

	// Called when this Gui gets loaded to create the UI
	// Allocate all elements on the heap. libtesla will make sure to clean them up when not needed anymore
	virtual tsl::elm::Element* createUI() override {
		// A OverlayFrame is the base element every overlay consists of. This will draw the default Title and Subtitle.
		// If you need more information in the header or want to change it's look, use a HeaderOverlayFrame.
		auto frame = new tsl::elm::OverlayFrame("FPSLocker", APP_VERSION"-ASAP");

		// A list that can contain sub elements and handles scrolling
		auto list = new tsl::elm::List();
		
		list->addItem(new tsl::elm::CustomDrawer([](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
			if (!SaltySD) {
				renderer->drawString("SaltyNX가 작동하지 않습니다!", false, x, y+50, 20, renderer->a(0xF33F));
			}
			else if (!plugin) {
				renderer->drawString("SD 카드의 NX-FPS 플러그인 감지 실패!", false, x, y+50, 20, renderer->a(0xF33F));
			}
			else if (!check) {
				if (closed) {
					renderer->drawString("게임 종료, 오버레이 비활성화 됨!", false, x, y+20, 19, renderer->a(0xF33F));
				}
				else {
					renderer->drawString("실행된 게임 없음, 오버레이 비활성화 됨!", false, x, y+20, 19, renderer->a(0xF33F));
				}
			}
			else if (!PluginRunning) {
				renderer->drawString("게임 실행중", false, x, y+20, 20, renderer->a(0xFFFF));
				renderer->drawString("NX-FPS가 작동하지 않습니다!", false, x, y+40, 20, renderer->a(0xF33F));
			}
			else if (!*pluginActive) {
				renderer->drawString("NX-FPS 실행중이지만, 프레임 처리 없음", false, x, y+20, 20, renderer->a(0xF33F));
				renderer->drawString("오버레이를 다시 시작하십시오", false, x, y+50, 20, renderer->a(0xFFFF));
			}
			else {
				renderer->drawString("NX-FPS 실행중", false, x, y+20, 20, renderer->a(0xFFFF));
				if ((*API_shared > 0) && (*API_shared <= 2))
					renderer->drawString(FPSMode_c, false, x, y+40, 20, renderer->a(0xFFFF));
				renderer->drawString(FPSTarget_c, false, x, y+60, 20, renderer->a(0xFFFF));
				renderer->drawString(PFPS_c, false, x+290, y+48, 50, renderer->a(0xFFFF));
			}
		}), 90);

		if (PluginRunning && *pluginActive) {
			auto *clickableListItem = new tsl::elm::ListItem("FPS UP");
			clickableListItem->setClickListener([](u64 keys) { 
				if ((keys & HidNpadButton_A) && PluginRunning) {
					if (*FPSmode_shared == 2 && !*FPSlocked_shared) {
						*FPSlocked_shared = 35;
					}
					else if (!*FPSlocked_shared) {
						*FPSlocked_shared = 60;
					}
					else if (*FPSlocked_shared < 60) {
						*FPSlocked_shared += 5;
					}
					if (!isDocked && !oldSalty && displaySync) {
						if (R_SUCCEEDED(SaltySD_Connect())) {
							if (*FPSlocked_shared >= 40) {
								SaltySD_SetDisplayRefreshRate(*FPSlocked_shared);
								refreshRate_g = *FPSlocked_shared;
							}
							else {
								SaltySD_SetDisplayRefreshRate(60);
								refreshRate_g = 60;
							}
							*displaySync_shared = refreshRate_g;
							SaltySD_Term();
						}
					}
					return true;
				}
				return false;
			});

			list->addItem(clickableListItem);
			
			auto *clickableListItem2 = new tsl::elm::ListItem("FPS DOWN");
			clickableListItem2->setClickListener([](u64 keys) { 
				if ((keys & HidNpadButton_A) && PluginRunning) {
					if (*FPSmode_shared < 2 && !*FPSlocked_shared) {
						*FPSlocked_shared = 55;
					}
					else if (!*FPSlocked_shared) {
						*FPSlocked_shared = 25;
					}
					else if (*FPSlocked_shared > 15) {
						*FPSlocked_shared -= 5;
					}
					if (!isDocked && !oldSalty && displaySync) {
						if (R_SUCCEEDED(SaltySD_Connect())) {
							if (*FPSlocked_shared >= 40) {
								SaltySD_SetDisplayRefreshRate(*FPSlocked_shared);
								refreshRate_g = *FPSlocked_shared;
							}
							else {
								SaltySD_SetDisplayRefreshRate(60);
								refreshRate_g = 60;
							}
							SaltySD_Term();
							*displaySync_shared = refreshRate_g;
						}
					}
					return true;
				}
				return false;
			});
			list->addItem(clickableListItem2);

			auto *clickableListItem4 = new tsl::elm::ListItem("커스텀 FPS 비활성화");
			clickableListItem4->setClickListener([](u64 keys) { 
				if ((keys & HidNpadButton_A) && PluginRunning) {
					if (*FPSlocked_shared) {
						*FPSlocked_shared = 0;
					}
					if (displaySync) {
						if (!oldSalty && R_SUCCEEDED(SaltySD_Connect())) {
							SaltySD_SetDisplayRefreshRate(60);
							*displaySync_shared = 0;
							SaltySD_Term();
						}
					}
					return true;
				}
				return false;
			});
			list->addItem(clickableListItem4);

			auto *clickableListItem3 = new tsl::elm::ListItem("고급 설정");
			clickableListItem3->setClickListener([](u64 keys) { 
				if ((keys & HidNpadButton_A) && PluginRunning) {
					tsl::changeTo<AdvancedGui>();
					return true;
				}
				return false;
			});
			list->addItem(clickableListItem3);

			auto *clickableListItem5 = new tsl::elm::ListItem("설정 저장");
			clickableListItem5->setClickListener([](u64 keys) { 
				if ((keys & HidNpadButton_A) && PluginRunning) {
					if (!*FPSlocked_shared && !*ZeroSync_shared && !SetBuffers_save) {
						remove(savePath);
					}
					else {
						DIR* dir = opendir("sdmc:/SaltySD/plugins/");
						if (!dir) {
							mkdir("sdmc:/SaltySD/plugins/", 777);
						}
						else closedir(dir);
						dir = opendir("sdmc:/SaltySD/plugins/FPSLocker/");
						if (!dir) {
							mkdir("sdmc:/SaltySD/plugins/FPSLocker/", 777);
						}
						else closedir(dir);
						FILE* file = fopen(savePath, "wb");
						if (file) {
							fwrite(FPSlocked_shared, 1, 1, file);
							if (SetBuffers_save > 2 || (!SetBuffers_save && *Buffers_shared > 2)) {
								*ZeroSync_shared = 0;
							}
							fwrite(ZeroSync_shared, 1, 1, file);
							if (SetBuffers_save) {
								fwrite(&SetBuffers_save, 1, 1, file);
							}
							fclose(file);
						}
					}
					return true;
				}
				return false;
			});
			list->addItem(clickableListItem5);
		}

		if (!isOLED && SaltySD) {
			auto *clickableListItem6 = new tsl::elm::ListItem("디스플레이 설정", "\uE151");
			clickableListItem6->setClickListener([](u64 keys) { 
				if (keys & HidNpadButton_A) {
					tsl::changeTo<WarningDisplayGui>();
					return true;
				}
				return false;
			});
			list->addItem(clickableListItem6);
		}

		// Add the list to the frame for it to be drawn
		frame->setContent(list);
		
		// Return the frame to have it become the top level element of this Gui
		return frame;
	}

	// Called once every frame to update values
	virtual void update() override {
		static uint8_t i = 10;

		if (PluginRunning) {
			if (i > 9) {
				apmGetPerformanceMode(&performanceMode);
				if (performanceMode == ApmPerformanceMode_Boost) {
					isDocked = true;
					refreshRate_g = 60;
				}
				else if (performanceMode == ApmPerformanceMode_Normal) {
					isDocked = false;
				}
				switch (*FPSmode_shared) {
					case 0:
						//This is usually a sign that game doesn't use interval
						sprintf(FPSMode_c, "인터벌 모드: 0 (미사용)");
						break;
					case 1 ... 5:
						sprintf(FPSMode_c, "인터벌 모드: %d (%d FPS)", *FPSmode_shared, refreshRate_g / *FPSmode_shared);
						break;
					default:
						sprintf(FPSMode_c, "인터벌 모드: %d (잘못됨)", *FPSmode_shared);
				}
				if (!*FPSlocked_shared) {
					sprintf(FPSTarget_c, "커스텀 FPS: 비활성화");
				}
				else sprintf(FPSTarget_c, "커스텀 FPS: %d", *FPSlocked_shared);
				sprintf(PFPS_c, "%d", *FPS_shared);
				i = 0;
			}
			else i++;
		}
	}

	// Called once every frame to handle inputs not handled by other UI elements
	virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
		return false;   // Return true here to singal the inputs have been consumed
	}
};

class OverlayTest : public tsl::Overlay {
public:
	// libtesla already initialized fs, hid, pl, pmdmnt, hid:sys and set:sys
	virtual void initServices() override {

		tsl::hlp::doWithSmSession([]{
			
			apmInitialize();
			setsysInitialize();
			SetSysProductModel model;
			if (R_SUCCEEDED(setsysGetProductModel(&model))) {
				if (model == SetSysProductModel_Aula) {
					isOLED = true;
					remove("sdmc:/SaltySD/flags/displaysync.flag");
				}
			}
			setsysExit();
			fsdevMountSdmc();
			FILE* file = fopen("sdmc:/SaltySD/flags/displaysync.flag", "rb");
			if (file) {
				displaySync = true;
				fclose(file);
			}
			SaltySD = CheckPort();
			if (!SaltySD) return;

			if (R_SUCCEEDED(SaltySD_Connect())) {
				if (R_FAILED(SaltySD_GetDisplayRefreshRate(&refreshRate_g))) {
					refreshRate_g = 60;
					oldSalty = true;
				}
				svcSleepThread(100'000);
				SaltySD_Term();
			}

			if (R_FAILED(pmdmntGetApplicationProcessId(&PID))) return;
			check = true;
			
			if(!LoadSharedMemory()) return;
			
			uintptr_t base = (uintptr_t)shmemGetAddr(&_sharedmemory);
			ptrdiff_t rel_offset = searchSharedMemoryBlock(base);
			if (rel_offset > -1)
				displaySync_shared = (uint8_t*)(base + rel_offset + 59);

			if (!PluginRunning) {
				if (rel_offset > -1) {
					pminfoInitialize();
					pminfoGetProgramId(&TID, PID);
					pminfoExit();
					BID = getBID();
					sprintf(&patchPath[0], "sdmc:/SaltySD/plugins/FPSLocker/patches/%016lX/%016lX.bin", TID, BID);
					sprintf(&configPath[0], "sdmc:/SaltySD/plugins/FPSLocker/patches/%016lX/%016lX.yaml", TID, BID);
					sprintf(&savePath[0], "sdmc:/SaltySD/plugins/FPSLocker/%016lX.dat", TID);

					FPS_shared = (uint8_t*)(base + rel_offset + 4);
					pluginActive = (bool*)(base + rel_offset + 9);
					FPSlocked_shared = (uint8_t*)(base + rel_offset + 10);
					FPSmode_shared = (uint8_t*)(base + rel_offset + 11);
					ZeroSync_shared = (uint8_t*)(base + rel_offset + 12);
					patchApplied_shared = (uint8_t*)(base + rel_offset + 13);
					API_shared = (uint8_t*)(base + rel_offset + 14);
					Buffers_shared = (uint8_t*)(base + rel_offset + 55);
					SetBuffers_shared = (uint8_t*)(base + rel_offset + 56);
					ActiveBuffers_shared = (uint8_t*)(base + rel_offset + 57);
					SetActiveBuffers_shared = (uint8_t*)(base + rel_offset + 58);
					SetBuffers_save = *SetBuffers_shared;
					PluginRunning = true;
					threadCreate(&t0, loopThread, NULL, NULL, 0x1000, 0x20, 0);
					threadStart(&t0);
				}		
			}
		
		});
	
	}  // Called at the start to initialize all services necessary for this Overlay
	
	virtual void exitServices() override {
		threadActive = false;
		threadWaitForExit(&t0);
		threadClose(&t0);
		shmemClose(&_sharedmemory);
		nsExit();
		fsdevUnmountDevice("sdmc");
		apmExit();
	}  // Callet at the end to clean up all services previously initialized

	virtual void onShow() override {}    // Called before overlay wants to change from invisible to visible state
	
	virtual void onHide() override {}    // Called before overlay wants to change from visible to invisible state

	virtual std::unique_ptr<tsl::Gui> loadInitialGui() override {
		if (SaltySD && check && plugin) {
			return initially<GuiTest>(1, 2, true);  // Initial Gui to load. It's possible to pass arguments to it's constructor like this
		}
		else {
			tsl::hlp::doWithSmSession([]{
				nsInitialize();
			});
			Result rc = getTitles(32);
			if (oldSalty || isOLED || !SaltySD)
				return initially<NoGame2>(rc, 2, true);
			else return initially<NoGame>(rc, 2, true);
		}
	}
};

int main(int argc, char **argv) {
	return tsl::loop<OverlayTest>(argc, argv);
}
