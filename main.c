#include <windows.h>
#include <process.h>
#include <stdio.h>
#include <tlhelp32.h>
#define LABEL_USERNAME 1001
#define LABEL_PASSWORD 1002
#define SQL_FILE_PATH 1003
#define CHOICE_SQL_FILE 1004
#define START_IMPORT_SQL_DATA 1005
#define END_IMPORT_SQL_DATA 1006
#define CHECK_MYSQL_ENVIRONMENT 10071	  // 检查mysql env
#define CHECK_SQLSERVER_ENVIRONMENT 10072 // 检查mssql env
#define LOG_LISTBOX_ID 1008
#define LABEL_DBNAME 1009
#define IDC_RADIO_MYSQL 1010	 // mysql
#define IDC_RADIO_SQLSERVER 1011 // sql server
HWND LogList = NULL;
DWORD g_CmdProcessId = 0;	// cmd.exe  PID
DWORD g_MySqlProcessId = 0; // mysql.exe PID
DWORD g_MsSqlProcessId = 0; // mssql.exe PID
int g_SelectedDbType = 1;	// 数据库类型选择项

typedef struct
{
	HWND hwndParent;
	wchar_t dbName[128];
	wchar_t userName[128];
	wchar_t passWord[128];
	wchar_t filePath[MAX_PATH];
	int dbType;

} ImportParam;

int GeSQLtFilePath(HWND hwnd, wchar_t *file_path, DWORD max_len);
void CleanSpace(wchar_t *str);
void Kill_Mysql_Process(DWORD sign);
void Check_Mysql_Env();
void Check_Mssql_Env();
DWORD WINAPI BackgroundImportThread(LPVOID lpParam)
{
	ImportParam *param = (ImportParam *)lpParam;
	int dbtype = param->dbType;
	HWND hwnd = param->hwndParent;
	CleanSpace(param->dbName);
	CleanSpace(param->userName);
	CleanSpace(param->passWord);
	CleanSpace(param->filePath);
	SendMessageW(LogList, LB_ADDSTRING, 0, (LPARAM)L"数据清洗完成，正在生成 CMD 命令...");
	char cUser[128], cPass[128], cPath[MAX_PATH], cDB[128];
	WideCharToMultiByte(CP_ACP, 0, param->dbName, -1, cDB, sizeof(cDB), NULL, NULL);
	WideCharToMultiByte(CP_ACP, 0, param->userName, -1, cUser, sizeof(cUser), NULL, NULL);
	WideCharToMultiByte(CP_ACP, 0, param->passWord, -1, cPass, sizeof(cPass), NULL, NULL);
	WideCharToMultiByte(CP_ACP, 0, param->filePath, -1, cPath, sizeof(cPath), NULL, NULL);
	char cmdLine[1024];
	if (dbtype == 1)
	{
		snprintf(cmdLine, sizeof(cmdLine), "cmd.exe /c \"mysql -u %s -p%s %s < \"%s\"\"", cUser, cPass, cDB, cPath);
	}
	else if (dbtype == 2)
	{
		snprintf(cmdLine, sizeof(cmdLine), "cmd.exe /c \"sqlcmd -S localhost -U %s -P %s -d %s -i \"%s\" -b\"", cUser, cPass, cDB, cPath);
	}
	else
	{
		SendMessageW(LogList, LB_ADDSTRING, 0, (LPARAM) "数据库类型错误");
		return 0;
	}

	SendMessageW(LogList, LB_ADDSTRING, 0, (LPARAM)L"正在链接数据库，导入中...");
	STARTUPINFOA si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	si.dwFlags |= STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;
	ZeroMemory(&pi, sizeof(pi));
	if (CreateProcessA(NULL, cmdLine, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
	{
		g_CmdProcessId = pi.dwProcessId;
		HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (hSnapshot != INVALID_HANDLE_VALUE)
		{
			PROCESSENTRY32W pe;
			pe.dwSize = sizeof(PROCESSENTRY32W);
			if (Process32FirstW(hSnapshot, &pe) && dbtype == 1)
			{
				do
				{
					if (_wcsicmp(pe.szExeFile, L"mysql.exe") == 0 && pe.th32ParentProcessID == g_CmdProcessId)
					{
						g_MySqlProcessId = pe.th32ProcessID;
						break;
					}
				} while (Process32NextW(hSnapshot, &pe));
			}
			else
			{
				do
				{
					if (_wcsicmp(pe.szExeFile, L"sqlcmd.exe") == 0 && pe.th32ParentProcessID == g_CmdProcessId)
					{
						g_MsSqlProcessId = pe.th32ProcessID;
						break;
					}
				} while (Process32NextW(hSnapshot, &pe));
			}
			CloseHandle(hSnapshot);
		}
		if (dbtype == 1 && g_MySqlProcessId != 0)
		{
			wchar_t logBuf[128] = {0};
			swprintf_s(logBuf, 128, L"锁定目标 mysql.exe，PID: %lu", g_MySqlProcessId);
			SendMessageW(hwnd, LB_ADDSTRING, 0, (LPARAM)logBuf);
		}
		else
		{
			wchar_t logBuf[128] = {0};
			swprintf_s(logBuf, 128, L"锁定目标 sqlcmd.exe，PID: %lu", g_MsSqlProcessId);
			SendMessageW(hwnd, LB_ADDSTRING, 0, (LPARAM)logBuf);
		}

		WaitForSingleObject(pi.hProcess, INFINITE);
		DWORD exitCode;
		GetExitCodeProcess(pi.hProcess, &exitCode);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		if (exitCode == 0)
		{
			SendMessageW(LogList, LB_ADDSTRING, 0, (LPARAM)L"SQL文件已导入数据库");
			MessageBoxW(hwnd, L"SQL 数据库文件导入成功", L"恭喜", MB_OK | MB_ICONINFORMATION);
		}
		else
		{
			SendMessageW(LogList, LB_ADDSTRING, 0, (LPARAM)L"[失败,请检查用户名、密码或 数据库 状态");
			MessageBoxW(hwnd, L"导入失败！请检查配置或数据库环境。", L"错误", MB_OK | MB_ICONERROR);
		}
	}
	else
	{
		SendMessageW(LogList, LB_ADDSTRING, 0, (LPARAM)L"错误,无法调用系统 CMD");
	}
	free(param);
	return 0;
}
/* This is where all the input to the window goes to */
LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{

	case WM_CREATE:
	{
		const int START_X = 25;		// 整体左边距
		const int START_Y = 30;		// 整体上边距
		const int LABEL_WIDTH = 90; // 左侧标签宽度
		const int EDIT_WIDTH = 250; // 右侧输入框宽度
		const int COMP_HEIGHT = 28; // 控件标准高度
		const int LINE_SPACE = 44;	// 纵向行间距

		int currentY = START_Y;

		CreateWindowW(L"BUTTON", L" 数据库配置选项 ",
					  WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
					  START_X - 12, START_Y - 18, LABEL_WIDTH + EDIT_WIDTH + 24, (LINE_SPACE * 5) + 12,
					  hwnd, (HMENU)NULL, GetModuleHandle(NULL), NULL);

		// 数据库名
		CreateWindowW(L"EDIT", L"  数据库名：", WS_CHILD | WS_VISIBLE | ES_LEFT | ES_READONLY,
					  START_X, currentY, LABEL_WIDTH, COMP_HEIGHT, hwnd, (HMENU)NULL, NULL, NULL);
		CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_LEFT | WS_BORDER | WS_TABSTOP,
					  START_X + LABEL_WIDTH, currentY, EDIT_WIDTH, COMP_HEIGHT, hwnd, (HMENU)LABEL_DBNAME, NULL, NULL);

		// 用户名
		currentY += LINE_SPACE;
		CreateWindowW(L"EDIT", L"  用 户 名：", WS_CHILD | WS_VISIBLE | ES_LEFT | ES_READONLY,
					  START_X, currentY, LABEL_WIDTH, COMP_HEIGHT, hwnd, (HMENU)NULL, NULL, NULL);
		CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_LEFT | WS_BORDER | WS_TABSTOP,
					  START_X + LABEL_WIDTH, currentY, EDIT_WIDTH, COMP_HEIGHT, hwnd, (HMENU)LABEL_USERNAME, NULL, NULL);

		// 密码
		currentY += LINE_SPACE;
		CreateWindowW(L"EDIT", L"  密    码：", WS_CHILD | WS_VISIBLE | ES_LEFT | ES_READONLY,
					  START_X, currentY, LABEL_WIDTH, COMP_HEIGHT, hwnd, (HMENU)NULL, NULL, NULL);
		CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_LEFT | WS_BORDER | ES_PASSWORD | WS_TABSTOP,
					  START_X + LABEL_WIDTH, currentY, EDIT_WIDTH, COMP_HEIGHT, hwnd, (HMENU)LABEL_PASSWORD, NULL, NULL);

		// SQL 文件选择
		currentY += LINE_SPACE;
		CreateWindowW(L"EDIT", L"  SQL 文件：", WS_CHILD | WS_VISIBLE | ES_LEFT | ES_READONLY,
					  START_X, currentY, LABEL_WIDTH, COMP_HEIGHT, hwnd, (HMENU)NULL, NULL, NULL);
		CreateWindowW(L"EDIT", L"尚未选择任何文件...", WS_CHILD | WS_VISIBLE | ES_LEFT | WS_BORDER | ES_READONLY,
					  START_X + LABEL_WIDTH, currentY, EDIT_WIDTH - 80, COMP_HEIGHT, hwnd, (HMENU)SQL_FILE_PATH, NULL, NULL);
		CreateWindowW(L"BUTTON", L"浏览...", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
					  START_X + LABEL_WIDTH + (EDIT_WIDTH - 75), currentY, 75, COMP_HEIGHT, hwnd, (HMENU)CHOICE_SQL_FILE, NULL, NULL);

		// 数据库单选按钮
		currentY += LINE_SPACE;
		CreateWindowW(L"EDIT", L"  驱动类型：", WS_VISIBLE | WS_CHILD | ES_LEFT | ES_READONLY,
					  START_X, currentY, LABEL_WIDTH, COMP_HEIGHT, hwnd, (HMENU)NULL, NULL, NULL);

		CreateWindowW(L"BUTTON", L"MySQL", WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | WS_GROUP,
					  START_X + LABEL_WIDTH + 15, currentY, 90, COMP_HEIGHT, hwnd, (HMENU)IDC_RADIO_MYSQL, NULL, NULL);
		CreateWindowW(L"BUTTON", L"SQL Server", WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON,
					  START_X + LABEL_WIDTH + 115, currentY, 120, COMP_HEIGHT, hwnd, (HMENU)IDC_RADIO_SQLSERVER, NULL, NULL);

		// 默认MySQL
		CheckRadioButton(hwnd, IDC_RADIO_MYSQL, IDC_RADIO_SQLSERVER, IDC_RADIO_MYSQL);

		currentY += LINE_SPACE + 25;

		CreateWindowW(L"BUTTON", L"开始导入", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
					  START_X, currentY, 100, 35, hwnd, (HMENU)START_IMPORT_SQL_DATA, NULL, NULL);

		CreateWindowW(L"BUTTON", L"结束导入", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
					  START_X + 115, currentY, 100, 35, hwnd, (HMENU)END_IMPORT_SQL_DATA, NULL, NULL);

		CreateWindowW(L"BUTTON", L"检测MYSQL环境", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
					  START_X + 230, currentY, 110, 35, hwnd, (HMENU)CHECK_MYSQL_ENVIRONMENT, NULL, NULL);

		CreateWindowW(L"BUTTON", L"检测MSSQL环境", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
					  START_X + 360, currentY, 110, 35, hwnd, (HMENU)CHECK_SQLSERVER_ENVIRONMENT, NULL, NULL);

		currentY += 55;
		CreateWindowW(L"STATIC", L"运行状态日志：", WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP,
					  START_X, currentY, LABEL_WIDTH + 100, 20, hwnd, (HMENU)NULL, NULL, NULL);

		currentY += 25;
		LogList = CreateWindowW(L"LISTBOX", L"",
								WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY,
								START_X, currentY, LABEL_WIDTH + EDIT_WIDTH, 140,
								hwnd, (HMENU)LOG_LISTBOX_ID, NULL, NULL);

		HFONT hFont = CreateFontW(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
								  GB2312_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
								  CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei");

		if (hFont)
		{
			HWND hChild = GetWindow(hwnd, GW_CHILD);
			while (hChild)
			{
				SendMessageW(hChild, WM_SETFONT, (WPARAM)hFont, TRUE);
				hChild = GetWindow(hChild, GW_HWNDNEXT);
			}
		}

		break;
	}

	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
		case IDC_RADIO_MYSQL:
		{
			g_SelectedDbType = 1;
			SendMessageW(LogList, LB_ADDSTRING, 0, (LPARAM)L"当前选择为Mysql数据库导入");
			break;
		}
		case IDC_RADIO_SQLSERVER:
		{
			g_SelectedDbType = 2;
			SendMessageW(LogList, LB_ADDSTRING, 0, (LPARAM)L"当前选择为SqlServer数据库导入");
			break;
		}
		case CHOICE_SQL_FILE:
		{
			/* code */
			wchar_t file_path[MAX_PATH] = {0};
			if (GeSQLtFilePath(hwnd, file_path, MAX_PATH))
			{
				HWND hEditFilePath = GetDlgItem(hwnd, SQL_FILE_PATH);
				SetWindowTextW(hEditFilePath, file_path);
				SendMessageW(LogList, LB_ADDSTRING, 0, (LPARAM)L"[提示] 成功选择 SQL 文件");
				MessageBoxW(hwnd, L"成功选择 SQL 文件", L"提示", MB_OK);
			}
			else
			{
				SendMessageW(LogList, LB_ADDSTRING, 0, (LPARAM)L"[提示]未选择文件");
				MessageBoxW(hwnd, L"未选择文件", L"提示", MB_OK);
			}
			break;
		}
		case START_IMPORT_SQL_DATA:
		{
			wchar_t wait_clean_dbname[128] = {0};
			wchar_t wait_clean_username[128] = {0};
			wchar_t wait_clean_password[128] = {0};
			wchar_t sql_file_path[MAX_PATH] = {0};

			HWND hEditDB = GetDlgItem(hwnd, LABEL_DBNAME);
			HWND hEditUser = GetDlgItem(hwnd, LABEL_USERNAME);
			HWND hEditPass = GetDlgItem(hwnd, LABEL_PASSWORD);
			HWND hEditPath = GetDlgItem(hwnd, SQL_FILE_PATH);

			GetWindowTextW(hEditDB, wait_clean_dbname, 128);
			GetWindowTextW(hEditUser, wait_clean_username, 128);
			GetWindowTextW(hEditPass, wait_clean_password, 128);
			GetWindowTextW(hEditPath, sql_file_path, MAX_PATH);

			if (wcslen(sql_file_path) == 0 || wcscmp(sql_file_path, L"尚未选择任何文件...") == 0)
			{
				MessageBoxW(hwnd, L"请先选择要导入的 SQL 文件", L"提示", MB_OK | MB_ICONWARNING);
				break;
			}
			SendMessageW(LogList, LB_ADDSTRING, 0, (LPARAM)L"[提示] 主线程就绪");

			ImportParam *param = (ImportParam *)malloc(sizeof(ImportParam));
			param->hwndParent = hwnd;
			param->dbType = g_SelectedDbType;
			wcscpy_s(param->dbName, 128, wait_clean_dbname);
			wcscpy_s(param->userName, 128, wait_clean_username);
			wcscpy_s(param->passWord, 128, wait_clean_password);
			wcscpy_s(param->filePath, MAX_PATH, sql_file_path);

			HANDLE hThread = CreateThread(NULL, 0, BackgroundImportThread, param, 0, NULL);

			if (hThread)
			{
				CloseHandle(hThread);
			}

			break;
		}
		case END_IMPORT_SQL_DATA:
		{
			Kill_Mysql_Process(g_MySqlProcessId);
			Kill_Mysql_Process(g_CmdProcessId);
			g_MySqlProcessId = 0;
			g_CmdProcessId = 0;
			SendMessageW(LogList, LB_ADDSTRING, 0, (LPARAM)L"已结束导入");
			break;
		}
		case CHECK_MYSQL_ENVIRONMENT:
		{
			Check_Mysql_Env();
			break;
		}
		case CHECK_SQLSERVER_ENVIRONMENT:
		{
			Check_Mssql_Env();
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

	hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, "WindowClass", "mysql-fast-importer", WS_VISIBLE | WS_OVERLAPPEDWINDOW,
						  CW_USEDEFAULT, /* x */
						  CW_USEDEFAULT, /* y */
						  640,			 /* width */
						  530,			 /* height */
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
 * Browse to get the SQL file path.
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
 * 清洗数据格式
 * Data cleaning format
 */
void CleanSpace(wchar_t *str)
{
	if (!str || *str == L'\0')
		return;
	wchar_t *start = str;
	while (*start && iswspace(*start))
		start++;
	int len = wcslen(start);
	wchar_t *end = start + len - 1;
	while (end >= start && iswspace(*end))
	{
		*end = L'\0';
		end--;
	}
	if (start != str)
		memmove(str, start, (wcslen(start) + 1) * sizeof(wchar_t));
}
/**
 * 结束导入
 * Finsh Importing
 */
void Kill_Mysql_Process(DWORD sign)
{
	if (sign == 0)
	{
		SendMessageW(LogList, LB_ADDSTRING, 0, (LPARAM)L"结束导入失败");
		return;
	}
	HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, sign);
	if (hProcess != NULL)
	{
		TerminateProcess(hProcess, 0);
		CloseHandle(hProcess);
	}
}
/**
 * MYSQL环境监测
 * MySQL Environment Monitoring
 */
void Check_Mysql_Env()
{
	SendMessageW(LogList, LB_ADDSTRING, 0, (LPARAM)L"开始检测MySQL环境");
	STARTUPINFOW si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	si.dwFlags |= STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;
	ZeroMemory(&pi, sizeof(pi));
	wchar_t cmd[] = L"cmd.exe /c \"mysql --version\"";
	if (CreateProcessW(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
	{
		DWORD waitResult = WaitForSingleObject(pi.hProcess, 2000);
		DWORD exitCode = 1;
		GetExitCodeProcess(pi.hProcess, &exitCode);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		if (waitResult == WAIT_OBJECT_0 && exitCode == 0)
		{
			SendMessageW(LogList, LB_ADDSTRING, 0, (LPARAM)L"本地 MySQL 环境配置正常");
		}
		else
		{
			SendMessageW(LogList, LB_ADDSTRING, 0, (LPARAM)L"未检测到 mysql 命令,请检查 PATH 环境变量");
		}
	}
}
/**
 * SQL SERVER环境监测
 * SQL SERVER Environment Monitoring
 */
void Check_Mssql_Env()
{
	SendMessageW(LogList, LB_ADDSTRING, 0, (LPARAM)L"开始检测SqlServer环境");
	STARTUPINFOW si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;
	ZeroMemory(&pi, sizeof(pi));
	wchar_t cmd[] = L"cmd.exe /c \"sqlcmd --version\"";
	if (CreateProcessW(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
	{
		DWORD waitResult = WaitForSingleObject(pi.hProcess, 2000);
		DWORD exitCode = 1;
		if (waitResult == WAIT_OBJECT_0 && exitCode == 0)
		{
			SendMessageW(LogList, LB_ADDSTRING, 0, (LPARAM)L"本地 SqlServer 环境配置正常");
		}
		else
		{
			SendMessageW(LogList, LB_ADDSTRING, 0, (LPARAM)L"未检测到 SqlServer 命令,请检查 PATH 环境变量");
		}
	}
}