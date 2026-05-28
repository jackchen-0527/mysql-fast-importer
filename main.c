#include <windows.h>

#define LABEL_USERNAME 1001
#define LABEL_PASSWORD 1002
#define SQL_FILE_PATH 1003
#define CHOICE_SQL_FILE 1003
#define START_IMPORT_SQL_DATA 1005
#define END_IMPORT_SQL_DATA 1006
#define CHECK_MYSQL_ENVIRONMENT 1007
/* This is where all the input to the window goes to */
LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
	case WM_CREATE:
	{
		CreateWindowW(L"STATIC", L"用户名:",
					  WS_CHILD | WS_VISIBLE | SS_CENTER,
					  20, 20, 80, 25,
					  hwnd, (HMENU)NULL, GetModuleHandle(NULL), NULL);
		CreateWindowW(L"EDIT", L"",
					  WS_CHILD | WS_VISIBLE | SS_LEFT | WS_BORDER,
					  100, 20, 150, 25,
					  hwnd, (HMENU)LABEL_USERNAME, GetModuleHandle(NULL), NULL);

		CreateWindowW(L"STATIC", L"密码",
					  WS_CHILD | WS_VISIBLE | SS_CENTER,
					  20, 60, 80, 25,
					  hwnd, (HMENU)NULL, GetModuleHandle(NULL), NULL);
		CreateWindowW(L"EDIT", L"",
					  WS_CHILD | WS_VISIBLE | ES_LEFT | WS_BORDER,
					  100, 60, 150, 25,
					  hwnd, (HMENU)LABEL_PASSWORD, GetModuleHandle(NULL), NULL);
		CreateWindowW(L"EDIT", L"",
					  WS_CHILD | WS_VISIBLE | WS_BORDER | SS_LEFT | WS_DISABLED,
					  20, 100, 100, 25,
					  hwnd, (HMENU)SQL_FILE_PATH, GetModuleHandle(NULL), NULL);
		CreateWindowW(L"BUTTON", L"选择文件",
					  WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
					  120, 100, 100, 25,
					  hwnd, (HMENU)CHOICE_SQL_FILE, GetModuleHandle(NULL), NULL);
		CreateWindowW(L"BUTTON", L"开始导入",
					  WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
					  20, 140, 100, 25,
					  hwnd, (HMENU)START_IMPORT_SQL_DATA, GetModuleHandle(NULL), NULL);
		CreateWindowW(L"BUTTON", L"结束导入",
					  WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
					  130, 140, 100, 25,
					  hwnd, (HMENU)END_IMPORT_SQL_DATA, GetModuleHandle(NULL), NULL);
		CreateWindowW(L"BUTTON", L"检测mysql环境",
					  WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
					  250, 140, 130, 25,
					  hwnd, (HMENU)CHECK_MYSQL_ENVIRONMENT, GetModuleHandle(NULL), NULL);

		break;
	}

	/* Upon destruction, tell the main thread to stop */
	case WM_DESTROY:
	{
		PostQuitMessage(0);
		break;
	}

	/* All other messages (a lot of them) are processed using default procedures */
	default:
		return DefWindowProc(hwnd, Message, wParam, lParam);
	}
	return 0;
}

/* The 'main' function of Win32 GUI programs: this is where execution starts */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	WNDCLASSEX wc; /* A properties struct of our window */
	HWND hwnd;	   /* A 'HANDLE', hence the H, or a pointer to our window */
	MSG msg;	   /* A temporary location for all messages */

	/* zero out the struct and set the stuff we want to modify */
	memset(&wc, 0, sizeof(wc));
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.lpfnWndProc = WndProc; /* This is where we will send messages to */
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);

	/* White, COLOR_WINDOW is just a #define for a system color, try Ctrl+Clicking it */
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszClassName = "WindowClass";
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);	  /* Load a standard icon */
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION); /* use the name "A" to use the project icon */

	if (!RegisterClassEx(&wc))
	{
		MessageBox(NULL, "Window Registration Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, "WindowClass", "SQL快速导入工具(mysql-fast-importer)", WS_VISIBLE | WS_OVERLAPPEDWINDOW,
						  CW_USEDEFAULT, /* x */
						  CW_USEDEFAULT, /* y */
						  640,			 /* width */
						  480,			 /* height */
						  NULL, NULL, hInstance, NULL);

	if (hwnd == NULL)
	{
		MessageBox(NULL, "Window Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	/*
		This is the heart of our program where all input is processed and
		sent to WndProc. Note that GetMessage blocks code flow until it receives something, so
		this loop will not produce unreasonably high CPU usage
	*/
	while (GetMessage(&msg, NULL, 0, 0) > 0)
	{							/* If no error is received... */
		TranslateMessage(&msg); /* Translate key codes to chars if present */
		DispatchMessage(&msg);	/* Send it to WndProc */
	}
	return msg.wParam;
}
