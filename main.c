#include <windows.h>
#include <process.h>
#define LABEL_USERNAME 1001
#define LABEL_PASSWORD 1002
#define SQL_FILE_PATH 1003
#define CHOICE_SQL_FILE 1004
#define START_IMPORT_SQL_DATA 1005
#define END_IMPORT_SQL_DATA 1006
#define CHECK_MYSQL_ENVIRONMENT 1007
#define LOG_LISTBOX_ID 1008
HWND LogList = NULL;

int GeSQLtFilePath(HWND hwnd, wchar_t *file_path, DWORD max_len);
wchar_t GetCleanUserName();
wchar_t GetCleanPassword();
/* This is where all the input to the window goes to */
LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
	case WM_CREATE:
	{
		const int LABEL_WIDTH = 70;
		const int EDIT_WIDTH = 180;
		const int COMP_HEIGHT = 25;
		const int GAP_Y = 15;

		CreateWindowW(L"STATIC", L"用户名：",
					  WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP,
					  20, 20, LABEL_WIDTH, COMP_HEIGHT,
					  hwnd, (HMENU)NULL, GetModuleHandle(NULL), NULL);
		CreateWindowW(L"EDIT", L"",
					  WS_CHILD | WS_VISIBLE | ES_LEFT | WS_BORDER | WS_TABSTOP,
					  20 + LABEL_WIDTH, 20, EDIT_WIDTH, COMP_HEIGHT,
					  hwnd, (HMENU)LABEL_USERNAME, GetModuleHandle(NULL), NULL);
		CreateWindowW(L"STATIC", L"密  码：",
					  WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP,
					  20, 60, LABEL_WIDTH, COMP_HEIGHT,
					  hwnd, (HMENU)NULL, GetModuleHandle(NULL), NULL);
		CreateWindowW(L"EDIT", L"",
					  WS_CHILD | WS_VISIBLE | ES_LEFT | WS_BORDER | ES_PASSWORD | WS_TABSTOP,
					  20 + LABEL_WIDTH, 60, EDIT_WIDTH, COMP_HEIGHT,
					  hwnd, (HMENU)LABEL_PASSWORD, GetModuleHandle(NULL), NULL);
		CreateWindowW(L"STATIC", L"SQL文件：",
					  WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP,
					  20, 100, LABEL_WIDTH, COMP_HEIGHT,
					  hwnd, (HMENU)NULL, GetModuleHandle(NULL), NULL);
		CreateWindowW(L"EDIT", L"尚未选择任何文件...",
					  WS_CHILD | WS_VISIBLE | ES_LEFT | WS_BORDER | ES_READONLY,
					  20 + LABEL_WIDTH, 100, EDIT_WIDTH - 85, COMP_HEIGHT,
					  hwnd, (HMENU)SQL_FILE_PATH, GetModuleHandle(NULL), NULL);
		CreateWindowW(L"BUTTON", L"浏览...",
					  WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
					  20 + LABEL_WIDTH + (EDIT_WIDTH - 80), 100, 80, COMP_HEIGHT,
					  hwnd, (HMENU)CHOICE_SQL_FILE, GetModuleHandle(NULL), NULL);
		CreateWindowW(L"BUTTON", L"开始导入",
					  WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
					  20, 145, 80, 30,
					  hwnd, (HMENU)START_IMPORT_SQL_DATA, GetModuleHandle(NULL), NULL);
		CreateWindowW(L"BUTTON", L"结束导入",
					  WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
					  110, 145, 80, 30,
					  hwnd, (HMENU)END_IMPORT_SQL_DATA, GetModuleHandle(NULL), NULL);
		CreateWindowW(L"BUTTON", L"检测 MySQL 环境",
					  WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
					  200, 145, 120, 30,
					  hwnd, (HMENU)CHECK_MYSQL_ENVIRONMENT, GetModuleHandle(NULL), NULL);
		LogList = CreateWindowW(L"LISTBOX", L"",
								WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY,
								20, 195, 310, 120,
								hwnd, (HMENU)LOG_LISTBOX_ID, GetModuleHandle(NULL), NULL);
		break;
	}

	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
		case CHOICE_SQL_FILE:
		{
			/* code */
			wchar_t file_path[MAX_PATH] = {0};
			if (GeSQLtFilePath(hwnd, file_path, MAX_PATH))
			{
				HWND hEditFilePath = GetDlgItem(hwnd, SQL_FILE_PATH);
				SetWindowTextW(hEditFilePath, file_path);
				SendMessageW(LogList, LB_ADDSTRING, 0, (LPARAM)L"[提示] 成功选择 SQL 文件");
				MessageBoxW(hwnd, L"路径选择成功", L"提示", MB_OK);
			}
			else
			{
				MessageBoxW(hwnd, L"未选择文件", L"提示", MB_OK);
			}
			break;
		}
		case START_IMPORT_SQL_DATA:
		{
			wchar_t wait_clean_username[128] = {0};
			wchar_t wait_clean_password[128] = {0};
			wchar_t sql_file_path[MAX_PATH] = {0};

			HWND hEditUser = GetDlgItem(hwnd, LABEL_USERNAME);
			HWND hEditPass = GetDlgItem(hwnd, LABEL_PASSWORD);
			HWND hEditPath = GetDlgItem(hwnd, SQL_FILE_PATH);

			GetWindowTextW(hEditUser, wait_clean_username, 128);
			GetWindowTextW(hEditPass, wait_clean_password, 128);
			GetWindowTextW(hEditPath, sql_file_path, MAX_PATH);

			// TrimWCharString(wait_clean_username,wait_clean_password);

			SendMessageW(LogList, LB_ADDSTRING, 0, (LPARAM)L"[提示] 成功读取输入框物资！");

			// ImportParam* param = (ImportParam*)malloc(sizeof(ImportParam));

			break;
		}
		default:
			return 0;
			break;
		}
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

/**
 * 浏览 获取sql文件路径
 */
int GeSQLtFilePath(HWND hwnd, wchar_t *file_path, DWORD max_len)
{
	OPENFILENAMEW ofn;
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFile = file_path;
	ofn.nMaxFile = max_len;
	ofn.lpstrFilter = L"SQL Files (*.sql)\0*.sql\0All Files (*.*)\0*.*\0";
	ofn.nFilterIndex = 1;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	if (GetOpenFileNameW(&ofn) == TRUE)
	{
		return 1;
	}
	return 0;
}

/**
 * 处理database用户名
 */
wchar_t GetCleanUserName()
{
}
/**
 * 处理database密码
 */
wchar_t GetCleanPassword()
{
}