#pragma once

enum WINDOW_CONTROLS {
	GRAY_BACKGROUND,
	TXT_EXE_NAME,
	INPUT_EXE_NAME,
	TXT_URL,
	INPUT_URL,
	TXT_SLEEP,
	INPUT_SLEEP,
	BUTTON_UPDATE,
	BUTTON_ABORT,
	LOG_GENERAL,
	CONTROLS_MAX
};

#define MAX_LOADSTRING 100
#define ID_BASE 200
#define LEADING_ZERO true

enum USER_INPUT {
	USER_INPUT_NONE,
	USER_INPUT_UPDATE_CONFIG,
	USER_INPUT_ABORT_SHUTDOWN
};

struct GlobalVariables {
	HINSTANCE hInst;                                // current instance
	WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
	WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
	HFONT system_font;
	int window_w;
	int window_h;
	HWND window_handle;
	RECT controls_pos[CONTROLS_MAX];
	HWND controls[CONTROLS_MAX];
	int user_input = USER_INPUT_NONE;
};