// RunExeRemotelyGUI.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "common.h"
#include "Resource.h"
#include "RunExeRemotely.h"

// Global Variables
GlobalVariables global;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
void                CalculateWindowSizes(HWND window);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    global.window_w = 640;
    global.window_h = 480;
    global.user_input = USER_INPUT_NONE;

	// Get system font
	NONCLIENTMETRICS ncMetrics = {0};
	ncMetrics.cbSize = sizeof(NONCLIENTMETRICS);
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncMetrics, 0);
	global.system_font = CreateFontIndirect(&ncMetrics.lfMessageFont);

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, global.szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_RUNEXEREMOTELYGUI, global.szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_RUNEXEREMOTELYGUI));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex = {0};

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_RUNEXEREMOTELYGUI));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_RUNEXEREMOTELYGUI);
    wcex.lpszClassName  = global.szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    global.hInst = hInstance; // Store instance handle in our global variable

	RECT desktop;
	GetWindowRect(GetDesktopWindow(), &desktop);
	int x = ((desktop.right - global.window_w) / 2);
	int y = ((desktop.bottom - global.window_h) / 2);

    global.window_handle = CreateWindowW(global.szWindowClass, global.szTitle, WS_OVERLAPPEDWINDOW, x, y, global.window_w, global.window_h, nullptr, nullptr, hInstance, nullptr);
    if (!global.window_handle) {
        return FALSE;
    }

    ShowWindow(global.window_handle, nCmdShow);
    UpdateWindow(global.window_handle);

    return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    #define EXE_TEXT_ID 300

    switch (message) {
        case WM_CREATE: {
            CalculateWindowSizes(hWnd);

			struct WindowTemplate {
				wchar_t type[32];
				wchar_t title[32];
				DWORD style;
			};

			WindowTemplate list[CONTROLS_MAX] = {};
			#define initWT(ID, TYPE, TITLE, STYLE) wcscpy_s(list[ID].type, 32, TYPE); wcscpy_s(list[ID].title, 32, TITLE); list[ID].style=STYLE;

            initWT(GRAY_BACKGROUND, L"Static", L"", SS_LEFT);
      		initWT(TXT_EXE_NAME, L"Static", L"Exe name:", SS_CENTERIMAGE);
			initWT(INPUT_EXE_NAME, L"Edit", L"", WS_DLGFRAME | ES_AUTOHSCROLL | WS_TABSTOP);
			initWT(TXT_URL, L"Static", L"URL:", SS_CENTERIMAGE);
			initWT(INPUT_URL, L"Edit", L"", WS_DLGFRAME | ES_AUTOHSCROLL | WS_TABSTOP);
			initWT(TXT_SLEEP, L"Static", L"Sleep time:", SS_CENTERIMAGE);
			initWT(INPUT_SLEEP, L"Edit", L"", WS_DLGFRAME | ES_AUTOHSCROLL | WS_TABSTOP);
            initWT(BUTTON_UPDATE, L"Button", L"Update Config", WS_TABSTOP);
            initWT(BUTTON_ABORT, L"Button", L"Abort Shutdown", WS_TABSTOP);
			initWT(LOG_GENERAL, L"Edit", L"", SS_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | ES_READONLY | WS_BORDER | WS_VSCROLL);

			for (int i=0; i<CONTROLS_MAX; i++) {
				global.controls[i] = CreateWindow(list[i].type, list[i].title, WS_CHILD | WS_VISIBLE | list[i].style, global.controls_pos[i].left, global.controls_pos[i].top, global.controls_pos[i].right, global.controls_pos[i].bottom, hWnd, (HMENU)(UINT_PTR)(ID_BASE+i), NULL, NULL);
				SendMessage(global.controls[i], WM_SETFONT, (WPARAM)global.system_font, TRUE);
			}

            EnableWindow(global.controls[BUTTON_ABORT], 0);

			DWORD threadID1 = 0;
			CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)RunExeRemotelyMain, (LPVOID)&global, 0,&threadID1);
        } break;

		case WM_SIZE: {
			CalculateWindowSizes(hWnd);

			for (int i=0; i<CONTROLS_MAX; i++)
				SetWindowPos(global.controls[i], NULL, global.controls_pos[i].left, global.controls_pos[i].top, global.controls_pos[i].right, global.controls_pos[i].bottom, SWP_NOZORDER);
		} break;

        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId) {
                case (ID_BASE+BUTTON_UPDATE):
                    global.user_input |= USER_INPUT_UPDATE_CONFIG;
                    EnableWindow(global.controls[BUTTON_UPDATE], 0);
                    break;

                case (ID_BASE+BUTTON_ABORT):
                    global.user_input |= USER_INPUT_ABORT_SHUTDOWN;
                    EnableWindow(global.controls[BUTTON_ABORT], 0);
                    break;

                case IDM_ABOUT:
                    DialogBox(global.hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                    break;

                case IDM_EXIT:
                    DestroyWindow(hWnd);
                    break;

                default:
                    return DefWindowProc(hWnd, message, wParam, lParam);
            }
        } break;

    /*case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;*/
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

void CalculateWindowSizes(HWND window)
{
	RECT dialogspace;
	GetClientRect(window, &dialogspace);

	global.controls_pos[GRAY_BACKGROUND] = dialogspace;

	global.controls_pos[TXT_EXE_NAME].left   = 5;
	global.controls_pos[TXT_EXE_NAME].top    = 5;
	global.controls_pos[TXT_EXE_NAME].right  = 80;
	global.controls_pos[TXT_EXE_NAME].bottom = 20;

	global.controls_pos[INPUT_EXE_NAME].left   = global.controls_pos[TXT_EXE_NAME].left + global.controls_pos[TXT_EXE_NAME].right;
	global.controls_pos[INPUT_EXE_NAME].top    = global.controls_pos[TXT_EXE_NAME].top;
	global.controls_pos[INPUT_EXE_NAME].right  = dialogspace.right - global.controls_pos[INPUT_EXE_NAME].left - 20;
	global.controls_pos[INPUT_EXE_NAME].bottom = global.controls_pos[TXT_EXE_NAME].bottom;

	global.controls_pos[TXT_URL]      = global.controls_pos[TXT_EXE_NAME];
	global.controls_pos[TXT_URL].top += global.controls_pos[TXT_EXE_NAME].bottom;

	global.controls_pos[INPUT_URL]      = global.controls_pos[INPUT_EXE_NAME];
	global.controls_pos[INPUT_URL].top += global.controls_pos[INPUT_EXE_NAME].bottom;		

	global.controls_pos[TXT_SLEEP]      = global.controls_pos[TXT_URL];
	global.controls_pos[TXT_SLEEP].top += global.controls_pos[TXT_URL].bottom;

	global.controls_pos[INPUT_SLEEP]      = global.controls_pos[INPUT_URL];
	global.controls_pos[INPUT_SLEEP].top += global.controls_pos[INPUT_URL].bottom;
    
    global.controls_pos[BUTTON_UPDATE]       = global.controls_pos[INPUT_SLEEP];
    global.controls_pos[BUTTON_UPDATE].right = 100;
    global.controls_pos[BUTTON_UPDATE].top  += global.controls_pos[INPUT_URL].bottom + 5;
    
    global.controls_pos[BUTTON_ABORT]       = global.controls_pos[BUTTON_UPDATE];
    global.controls_pos[BUTTON_ABORT].left += global.controls_pos[BUTTON_UPDATE].right + 5;
    
	global.controls_pos[LOG_GENERAL]        = dialogspace;
	global.controls_pos[LOG_GENERAL].top    = global.controls_pos[BUTTON_UPDATE].top + global.controls_pos[BUTTON_UPDATE].bottom + 10;
	global.controls_pos[LOG_GENERAL].bottom = dialogspace.bottom - global.controls_pos[LOG_GENERAL].top;
}