#include <windows.h>
#include <math.h>
#include <iostream>
#include <Imagehlp.h>
#include <tuple>

#include "filter.h"
#include "resource.h"
#include "load_ini.h"
#include <string>

//---------------------------------------------------------------------
//		フィルタ構造体定義
//---------------------------------------------------------------------
#define	TRACK_N	0		//	トラックバーの数
#define	CHECK_N	0		//	チェックボックスの数

FILTER_DLL filter = {
	FILTER_FLAG_PRIORITY_HIGHEST | FILTER_FLAG_AUDIO_FILTER | FILTER_FLAG_NO_CONFIG,
	0,0,
	"解像度選択プラグイン",
	TRACK_N,NULL,NULL,
	NULL,NULL,
	NULL,NULL,NULL,
	func_proc,
	func_init,
	NULL,
	NULL,
	NULL,
	NULL,NULL,
	NULL,
	NULL,
	"解像度選択プラグイン version 0.1 by たくんま",
	NULL,
	NULL,
};


//---------------------------------------------------------------------
//		フィルタ構造体のポインタを渡す関数
//---------------------------------------------------------------------
EXTERN_C FILTER_DLL __declspec(dllexport)* __stdcall GetFilterTable(void)
{
	return &filter;
}


//---------------------------------------------------------------------
//		フィルタ処理関数
//---------------------------------------------------------------------
BOOL func_proc(FILTER* fp, FILTER_PROC_INFO* fpip){
	return TRUE;
}

// フック用関数のプロトタイプ宣言
INT_PTR CALLBACK MyDialogBoxParamA(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);

BOOL func_init(FILTER* fp) {
	// printf("test");
	// exedit.aufのDialogBoxParamAをフックする
	HMODULE exeditModule = GetModuleHandleA("exedit.auf");
	if (exeditModule) {
		FARPROC origDialogBoxParamA = GetProcAddress(GetModuleHandleA("user32.dll"), "DialogBoxParamA");
		if (origDialogBoxParamA) {
			// exedit.aufのIATからDialogBoxParamAを探して書き換える
#pragma comment(lib, "Imagehlp.lib")
			ULONG size = 0;
			PIMAGE_IMPORT_DESCRIPTOR importDesc = (PIMAGE_IMPORT_DESCRIPTOR)ImageDirectoryEntryToData(
				exeditModule, TRUE, IMAGE_DIRECTORY_ENTRY_IMPORT, &size);
			if (importDesc) {
				for (; importDesc->Name; importDesc++) {
					LPCSTR dllName = (LPCSTR)((BYTE*)exeditModule + importDesc->Name);
					if (_stricmp(dllName, "user32.dll") == 0) {
						PIMAGE_THUNK_DATA thunk = (PIMAGE_THUNK_DATA)((BYTE*)exeditModule + importDesc->FirstThunk);
						for (; thunk->u1.Function; thunk++) {
							FARPROC* funcAddr = (FARPROC*)&thunk->u1.Function;
							if (*funcAddr == origDialogBoxParamA) {
								DWORD oldProtect;
								VirtualProtect(funcAddr, sizeof(FARPROC), PAGE_EXECUTE_READWRITE, &oldProtect);
								*funcAddr = (FARPROC)MyDialogBoxParamA; // MyDialogBoxParamAは自作のフック関数
								VirtualProtect(funcAddr, sizeof(FARPROC), oldProtect, &oldProtect);
								break;
							}
						}
						break;
					}
				}
			}
		}
	}
	return TRUE;
}

// フック用関数の定義
// グローバル変数で元のDialogBoxParamAのアドレスを保持
FARPROC g_origDialogBoxParamA = nullptr;

// グローバル変数で保持
HMODULE g_hInst = NULL;
LPARAM dwInitParam_exedit = NULL;

// 独自ダイアログプロシージャ
INT_PTR CALLBACK CustomDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	static LPARAM s_dwInitParam = 0;
	static resolution_info* resolutions_ini = NULL;
	HWND hCombo;
	switch (message) {
	case WM_INITDIALOG:
		s_dwInitParam = lParam;
		SetDlgItemInt(hDlg, IDC_WIDTH_EDIT, 1920, FALSE); //よこ
		SetDlgItemInt(hDlg, IDC_HEIGHT_EDIT, 1080, FALSE); //たて
		SetDlgItemInt(hDlg, IDC_HZ_EDIT, 44100, FALSE); //音
		SetDlgItemTextA(hDlg, IDC_FPS_EDIT, "60"); //fps

		hCombo = GetDlgItem(hDlg, IDC_RESOLUTION_COMBO);

		resolutions_ini = LoadResolutionsFromFile("resolutions.ini");
		SendMessageA(hCombo, CB_ADDSTRING, 0, (LPARAM)"costom");
        for (size_t i = 0; i < 128; ++i) {  
			if (resolutions_ini[i].width <= 0 && resolutions_ini[i].height <= 0) break;
			if (resolutions_ini[i].comment == "unload") break;
            char buf[128];
            snprintf(buf, sizeof(buf), "%d x %d %s",  
                    (resolutions_ini[i].width),  
                    (resolutions_ini[i].height),  
                    (resolutions_ini[i].comment).c_str());
            SendMessageA(hCombo, CB_ADDSTRING, 0, (LPARAM)buf);  
        }
		SendMessageA(hCombo, CB_SETCURSEL, 0, 0);  // 最初の項目(costom)を選択

		return TRUE;

	
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK) { //okボタン
			HWND hCombo = GetDlgItem(hDlg, IDC_RESOLUTION_COMBO);
			int index = (int)SendMessageA(hCombo, CB_GETCURSEL, 0, 0);
			// printf("解像度選択: %d", index);
			DWORD width = GetDlgItemInt(hDlg, IDC_WIDTH_EDIT, NULL, FALSE);
			DWORD height = GetDlgItemInt(hDlg, IDC_HEIGHT_EDIT, NULL, FALSE);

			/* 数字入力欄自体も更新するようにしたためコメントアウト
			if ((index != 0 || index != -1)){ //costom または 無効 ではない
				width = resolutions_ini[index - 1].width;
				height = resolutions_ini[index - 1].height;
			}
			*/
		
			DWORD Hz = GetDlgItemInt(hDlg, IDC_HZ_EDIT, NULL, FALSE);
			char fpsBuffer[32] = {0};
			GetDlgItemTextA(hDlg, IDC_FPS_EDIT, fpsBuffer, sizeof(fpsBuffer));
			// printf("dwInitParam_exedit: 0x%p\n", (void*)dwInitParam_exedit);

			HMODULE exeditModule = GetModuleHandleA("exedit.auf");
			if (exeditModule) {
				DWORD* pWidth = (DWORD*)((BYTE*)exeditModule + 0x001790D4);
				DWORD* pheight = (DWORD*)((BYTE*)exeditModule + 0x00178E34);
				DWORD* phz = (DWORD*)((BYTE*)exeditModule + 0x00177A3C);
				char* pfps = (char*)((BYTE*)exeditModule + 0x00153838);
				if (pWidth && pheight && phz && pfps) {
					*pWidth = width;
					*pheight = height;
					*phz = Hz;
					strncpy_s(pfps, 64, fpsBuffer, _TRUNCATE); // 最大64文字までコピー
				}
			}

			EndDialog(hDlg, IDOK);
			return TRUE;
		}
		if (HIWORD(wParam) == CBN_SELCHANGE && LOWORD(wParam) == IDC_RESOLUTION_COMBO) { // コンボボックス更新
			HWND hCombo = GetDlgItem(hDlg, IDC_RESOLUTION_COMBO);
			int index = (int)SendMessageA(hCombo, CB_GETCURSEL, 0, 0);
			if (index == 0) {
				
			} else if (index > 0 && resolutions_ini) {
				SetDlgItemInt(hDlg, IDC_WIDTH_EDIT, resolutions_ini[index - 1].width, FALSE);
				SetDlgItemInt(hDlg, IDC_HEIGHT_EDIT, resolutions_ini[index - 1].height, FALSE);
			}
			return TRUE;
		}
		if (LOWORD(wParam) == IDCANCEL) { //キャンセル
			EndDialog(hDlg, IDCANCEL);
			return TRUE;
		}
	}
	return FALSE;
}

// DllMainでセット
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        g_hInst = hModule;
    }
    return TRUE;
}

INT_PTR CALLBACK MyDialogBoxParamA(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam) {
	// exedit.aufが特定のダイアログを開こうとした場合のみ自作ダイアログを表示
	if (strcmp(lpTemplateName, "NEW_FILE") == 0) {
		// printf("新規プロジェクトの作成");
		dwInitParam_exedit = dwInitParam;
		// printf("%u", g_hInst);
		// "NEW_FILE_ALT"は自作リソーステンプレート名（リソースIDの場合はMAKEINTRESOURCEA(NEW_FILE_ALT)）
		return DialogBoxParamA(g_hInst, MAKEINTRESOURCEA(IDC_NEW_FILE_ALT), hWndParent, CustomDialogProc, dwInitParam);
	}

	// それ以外は元のDialogBoxParamAを呼ぶ
	if (!g_origDialogBoxParamA) {
		g_origDialogBoxParamA = GetProcAddress(GetModuleHandleA("user32.dll"), "DialogBoxParamA");
	}
	using DialogBoxParamA_t = INT_PTR (WINAPI*)(HINSTANCE, LPCSTR, HWND, DLGPROC, LPARAM);
	return ((DialogBoxParamA_t)g_origDialogBoxParamA)(hInstance, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);
}