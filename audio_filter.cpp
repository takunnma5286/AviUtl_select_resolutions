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
//		�t�B���^�\���̒�`
//---------------------------------------------------------------------
#define	TRACK_N	0		//	�g���b�N�o�[�̐�
#define	CHECK_N	0		//	�`�F�b�N�{�b�N�X�̐�

FILTER_DLL filter = {
	FILTER_FLAG_PRIORITY_HIGHEST | FILTER_FLAG_AUDIO_FILTER | FILTER_FLAG_NO_CONFIG,
	0,0,
	"�𑜓x�I���v���O�C��",
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
	"�𑜓x�I���v���O�C�� version 0.1 by �������",
	NULL,
	NULL,
};


//---------------------------------------------------------------------
//		�t�B���^�\���̂̃|�C���^��n���֐�
//---------------------------------------------------------------------
EXTERN_C FILTER_DLL __declspec(dllexport)* __stdcall GetFilterTable(void)
{
	return &filter;
}


//---------------------------------------------------------------------
//		�t�B���^�����֐�
//---------------------------------------------------------------------
BOOL func_proc(FILTER* fp, FILTER_PROC_INFO* fpip){
	return TRUE;
}

// �t�b�N�p�֐��̃v���g�^�C�v�錾
INT_PTR CALLBACK MyDialogBoxParamA(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);

BOOL func_init(FILTER* fp) {
	// printf("test");
	// exedit.auf��DialogBoxParamA���t�b�N����
	HMODULE exeditModule = GetModuleHandleA("exedit.auf");
	if (exeditModule) {
		FARPROC origDialogBoxParamA = GetProcAddress(GetModuleHandleA("user32.dll"), "DialogBoxParamA");
		if (origDialogBoxParamA) {
			// exedit.auf��IAT����DialogBoxParamA��T���ď���������
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
								*funcAddr = (FARPROC)MyDialogBoxParamA; // MyDialogBoxParamA�͎���̃t�b�N�֐�
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

// �t�b�N�p�֐��̒�`
// �O���[�o���ϐ��Ō���DialogBoxParamA�̃A�h���X��ێ�
FARPROC g_origDialogBoxParamA = nullptr;

// �O���[�o���ϐ��ŕێ�
HMODULE g_hInst = NULL;
LPARAM dwInitParam_exedit = NULL;

// �Ǝ��_�C�A���O�v���V�[�W��
INT_PTR CALLBACK CustomDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	static LPARAM s_dwInitParam = 0;
	static resolution_info* resolutions_ini = NULL;
	HWND hCombo;
	switch (message) {
	case WM_INITDIALOG:
		s_dwInitParam = lParam;
		SetDlgItemInt(hDlg, IDC_WIDTH_EDIT, 1920, FALSE); //�悱
		SetDlgItemInt(hDlg, IDC_HEIGHT_EDIT, 1080, FALSE); //����
		SetDlgItemInt(hDlg, IDC_HZ_EDIT, 44100, FALSE); //��
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
		SendMessageA(hCombo, CB_SETCURSEL, 0, 0);  // �ŏ��̍���(costom)��I��

		return TRUE;

	
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK) { //ok�{�^��
			HWND hCombo = GetDlgItem(hDlg, IDC_RESOLUTION_COMBO);
			int index = (int)SendMessageA(hCombo, CB_GETCURSEL, 0, 0);
			// printf("�𑜓x�I��: %d", index);
			DWORD width = GetDlgItemInt(hDlg, IDC_WIDTH_EDIT, NULL, FALSE);
			DWORD height = GetDlgItemInt(hDlg, IDC_HEIGHT_EDIT, NULL, FALSE);

			/* �������͗����̂��X�V����悤�ɂ������߃R�����g�A�E�g
			if ((index != 0 || index != -1)){ //costom �܂��� ���� �ł͂Ȃ�
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
					strncpy_s(pfps, 64, fpsBuffer, _TRUNCATE); // �ő�64�����܂ŃR�s�[
				}
			}

			EndDialog(hDlg, IDOK);
			return TRUE;
		}
		if (HIWORD(wParam) == CBN_SELCHANGE && LOWORD(wParam) == IDC_RESOLUTION_COMBO) { // �R���{�{�b�N�X�X�V
			HWND hCombo = GetDlgItem(hDlg, IDC_RESOLUTION_COMBO);
			int index = (int)SendMessageA(hCombo, CB_GETCURSEL, 0, 0);
			if (index == 0) {
				
			} else if (index > 0 && resolutions_ini) {
				SetDlgItemInt(hDlg, IDC_WIDTH_EDIT, resolutions_ini[index - 1].width, FALSE);
				SetDlgItemInt(hDlg, IDC_HEIGHT_EDIT, resolutions_ini[index - 1].height, FALSE);
			}
			return TRUE;
		}
		if (LOWORD(wParam) == IDCANCEL) { //�L�����Z��
			EndDialog(hDlg, IDCANCEL);
			return TRUE;
		}
	}
	return FALSE;
}

// DllMain�ŃZ�b�g
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        g_hInst = hModule;
    }
    return TRUE;
}

INT_PTR CALLBACK MyDialogBoxParamA(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam) {
	// exedit.auf������̃_�C�A���O���J�����Ƃ����ꍇ�̂ݎ���_�C�A���O��\��
	if (strcmp(lpTemplateName, "NEW_FILE") == 0) {
		// printf("�V�K�v���W�F�N�g�̍쐬");
		dwInitParam_exedit = dwInitParam;
		// printf("%u", g_hInst);
		// "NEW_FILE_ALT"�͎��샊�\�[�X�e���v���[�g���i���\�[�XID�̏ꍇ��MAKEINTRESOURCEA(NEW_FILE_ALT)�j
		return DialogBoxParamA(g_hInst, MAKEINTRESOURCEA(IDC_NEW_FILE_ALT), hWndParent, CustomDialogProc, dwInitParam);
	}

	// ����ȊO�͌���DialogBoxParamA���Ă�
	if (!g_origDialogBoxParamA) {
		g_origDialogBoxParamA = GetProcAddress(GetModuleHandleA("user32.dll"), "DialogBoxParamA");
	}
	using DialogBoxParamA_t = INT_PTR (WINAPI*)(HINSTANCE, LPCSTR, HWND, DLGPROC, LPARAM);
	return ((DialogBoxParamA_t)g_origDialogBoxParamA)(hInstance, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);
}