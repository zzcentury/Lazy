// MyLoader.cpp : 定义应用程序的入口点。
//

#include "stdafx.h"
#include "MyLoader.h"
#include<stdio.h>
#include<Windows.h>
#include<fltUser.h>
#include <winioctl.h>
#include<Mmsystem.h>
#define MAX_LOADSTRING 100
#pragma warning(disable:4996)

#pragma comment(lib,"user32.lib")
#pragma comment(lib,"kernel32.lib")
#pragma comment(lib,"WINMM.lib")
#pragma comment(lib,"fltLib.lib")
// 全局变量: 
HBITMAP      hBitmap;
HBRUSH       hBrush;
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名
#define ID_SMALLER      1
#define ID_LARGER       2

#define BTN_WIDTH        (8 * cxChar)
#define BTN_HEIGHT       (4 * cyChar)

#define TYPE_RANSOMEINFO \
	(ULONG)CTL_CODE(\
	FILE_DEVICE_UNKNOWN,\
	0x900,METHOD_BUFFERED,\
	FILE_ANY_ACCESS)

#define TYPE_INJECT \
	(ULONG)CTL_CODE(\
	FILE_DEVICE_UNKNOWN,\
    0x901,METHOD_BUFFERED,\
	FILE_ANY_ACCESS)

#define TYPE_INJECT_RESULT \
	(ULONG)CTL_CODE(\
	FILE_DEVICE_UNKNOWN,\
    0x902,METHOD_BUFFERED,\
	FILE_ANY_ACCESS)

#define MAXLEN 256
typedef struct _NOTIFICATION {
	HANDLE hProcess;
}NOTIFICATION, *PNOTIFICATION;

typedef struct _INJECT_INFO {
	HANDLE injectedProcess;//被注入的进程
	HANDLE injectProcess;//执行注入操作的进程
}INJECT_INFO, *PINJECT_INFO;

typedef struct _MESSAGE {
	FILTER_MESSAGE_HEADER MessageHeader;
	NOTIFICATION notification;
}MESSAGE, *PMESSAGE;

HANDLE g_port;
HANDLE hFlt, hLazy;
#define FLT_NAME L"FsFilter"
#define FLT_PORT_NAME L"\\FsFilterPort"

BOOL CreateTrapFile();
DWORD WINAPI CommunicationWithFlt(
	_In_ LPVOID lpParameter
);
DWORD WINAPI InjectMonitor(
	_In_ LPVOID lpParameter
);

DWORD WINAPI Lazy(_In_ LPVOID lpParameter);
// 此代码模块中包含的函数的前向声明: 
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // 初始化全局字符串
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_MYLOADER, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 执行应用程序初始化: 
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MYLOADER));

    MSG msg;

    // 主消息循环: 
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
	DeleteObject(hBrush);
    return (int) msg.wParam;
}



//
//  函数: MyRegisterClass()
//
//  目的: 注册窗口类。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;
	
	// TODO: 在此放置代码。
	hBitmap = LoadBitmap(hInstance, TEXT("Bricks"));
	hBrush = CreatePatternBrush(hBitmap);
	DeleteObject(hBitmap);
    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, (LPCTSTR)(IDI_ME)); 
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_MYLOADER);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, (LPCTSTR)(IDI_ME));

    return RegisterClassExW(&wcex);
}

//
//   函数: InitInstance(HINSTANCE, int)
//
//   目的: 保存实例句柄并创建主窗口
//
//   注释: 
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // 将实例句柄存储在全局变量中

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX ,
	   600, 30, 250, 300, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的:    处理主窗口的消息。
//
//  WM_COMMAND  - 处理应用程序菜单
//  WM_PAINT    - 绘制主窗口
//  WM_DESTROY  - 发送退出消息并返回
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HWND	mybutton;
	HANDLE hInject;
	DWORD tId;
	static int	cxClient, cyClient, cxChar, cyChar;
    switch (message)
    {
	case WM_CREATE:
		LONG dwStyle, dwNewStyle, dwExStyle, dwNewExStyle;
		dwStyle = GetWindowLong(hWnd, GWL_STYLE);//获取旧样式
		dwNewStyle = WS_OVERLAPPED | WS_VISIBLE | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
		dwNewStyle &= dwStyle;//按位与将旧样式去掉
		SetWindowLong(hWnd, GWL_STYLE, dwNewStyle);//设置成新的样式
		dwExStyle = GetWindowLong(hWnd, GWL_EXSTYLE);//获取旧扩展样式
		dwNewExStyle = WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR;
		dwNewExStyle &= dwExStyle;//按位与将旧扩展样式去掉
		SetWindowLong(hWnd, GWL_EXSTYLE, dwNewExStyle);//设置新的扩展样式
		SetWindowPos(hWnd, NULL, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
		SetWindowLong(hWnd, GWL_EXSTYLE, GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);
		SetLayeredWindowAttributes(hWnd, RGB(255, 255, 255), 255, LWA_ALPHA | LWA_COLORKEY);

		cxChar = LOWORD(GetDialogBaseUnits());
		cyChar = HIWORD(GetDialogBaseUnits());
		
		/*mybutton = CreateWindow(TEXT("button"), TEXT(""),
			WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			0, 0, BTN_WIDTH, BTN_HEIGHT,
			hWnd, (HMENU)ID_SMALLER, hInst, NULL);*/
		return 0;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 分析菜单选择: 
            switch (wmId)
            {
			case ID_SMALLER:
				DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
				break;
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
			case IDM_OPEN:
				//Lazy(hWnd);
				hInject = CreateThread(NULL,0,Lazy,NULL,0,&tId);
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: 在此处添加使用 hdc 的任何绘图代码...
            EndPaint(hWnd, &ps);
        }
        break;
	case WM_SIZE:
		cxClient = LOWORD(lParam);
		cyClient = HIWORD(lParam);

		// Move the buttons to the new center

		MoveWindow(mybutton, cxClient / 2 - 3 * BTN_WIDTH / 2,
			cyClient / 2 - BTN_HEIGHT / 2,
			BTN_WIDTH, BTN_HEIGHT, TRUE);
		return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// “关于”框的消息处理程序。
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

DWORD WINAPI Lazy(_In_ LPVOID lpParameter)
{
	DWORD tId;
	BOOL bResult;
	HANDLE hInject;

	bResult = CreateTrapFile();
	system("net start Lazy");
	hLazy = CreateFile(L"\\\\.\\Lazy", GENERIC_READ | GENERIC_WRITE, 0, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_SYSTEM, NULL);
	if (hLazy == INVALID_HANDLE_VALUE)
	{
		////printf("[Loader] open Lazy Device failed.\n");
		return -1;
	}
	//printf("[Loader] open Lazy Device successfully.\n");

	hInject = CreateThread(NULL, 0, InjectMonitor, NULL, 0, &tId);
	if (hInject == NULL)
	{
		//printf("[Loader] create inject monitor thread failed.\n");
		return -1;
	}
	bResult = CreateTrapFile();

	system("net start FsFilter");
	////printf("last error:%d\n", GetLastError());

	hFlt = CreateThread(NULL, 0, CommunicationWithFlt, NULL, 0, &tId);
	if (hFlt == NULL)
	{
		//printf("[Loader] Create communicationWithFlt thread fail\n");
		//LogError("[Loader] Create communicationWithFlt thread fail\n");
	}
	//system("pause");
	while (1)
	{
		Sleep(1000);
	}
}

BOOL CreateTrapFile()
{
	WCHAR driveName[5] = { 0 }, dirName[MAXLEN] = { 0 }, fileName[MAXLEN] = { 0 };
	DWORD cnt = 0;
	DWORD type, tmp;
	HANDLE hFile;
	CHAR buffer[20] = "PK1234567890";
	CHAR userName[MAXLEN] = { 0 };
	DWORD userSize = 256;

	for (cnt = 0; cnt < 26; cnt++)
	{
		wsprintf(driveName, L"%c:\\", 'A' + cnt);
		type = GetDriveType(driveName);

		if (type == DRIVE_FIXED)
		{
			wsprintf(dirName, L"%c:\\###000trap\\", 'A' + cnt);
			CreateDirectory(dirName, NULL);
			wsprintf(fileName, L"%c:\\###000trap\\tmplazy.txt", 'A' + cnt);
			hFile = CreateFileW(fileName, GENERIC_ALL, FILE_SHARE_READ,
				NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

			if (hFile == INVALID_HANDLE_VALUE)
			{
				//printf("create trap fail\n");
				//LogError("[Loader] CreateTrapFile Fail\n");

				return FALSE;
			}
			WriteFile(hFile, buffer, 20, &tmp, NULL);
			CloseHandle(hFile);
		}
	}

	if (GetUserName(LPWSTR(userName), &userSize))
	{
		wsprintf(dirName, L"C:\\Users\\%s\\Desktop\\###000trap\\", userName);
		CreateDirectory(dirName, NULL);
		wcscpy(fileName, dirName);
		wcscat(fileName, L"tmplazy.txt");

		hFile = CreateFileW(fileName, GENERIC_ALL, FILE_SHARE_READ,
			NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

		if (hFile == INVALID_HANDLE_VALUE)
		{
			//printf("create trap file in desktop fail\n");
			//LogError("[Loader] CreateTrapFile Fail\n");

			return FALSE;
		}
		WriteFile(hFile, buffer, 20, &tmp, NULL);
		CloseHandle(hFile);
	}
	else
	{
		//printf("[Loader] create trap in desktop failed.\n");
	}
	return TRUE;
}

DWORD WINAPI CommunicationWithFlt(
	_In_ LPVOID lpParameter
)
{
	HRESULT hResult;
	MESSAGE message = { 0 };
	CHAR resultBuf[5] = { 0 };
	DWORD ret_len = 0;
	HANDLE hProc;

	hResult = FilterConnectCommunicationPort(FLT_PORT_NAME, 0,
		NULL, 0, NULL, &g_port);

	if (hResult != S_OK)
	{
		//printf("Connect fail\n");
		//LogError("[Loader] FilterConnectCommunicationPort fail\n");
	}

	while (TRUE)
	{
		WCHAR buffer[256];
		FilterGetMessage(g_port, &(message.MessageHeader), sizeof(MESSAGE), NULL);
		//printf("[Loader]find ransomeware!!!processs handle:%d\n", message.notification.hProcess);
		wsprintf(buffer,_T("Find ransomeware!!!processs handle : %d\n"), message.notification.hProcess);
		MessageBoxW(NULL, (LPCWSTR)buffer,_T("Waring"), 0);
		hProc = OpenProcess(PROCESS_ALL_ACCESS, NULL, DWORD(message.notification.hProcess));
		if (TerminateProcess(hProc, 0))
		{
			MessageBox(NULL, _T("Stop ransomeware successfully."), _T("Success"), 0);
			//DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), 0, About);
			//printf("[Loader] stop ransomeware successfully.\n");
		}
		else
		{
			//printf("[Loader] stop ransomeware failed.\n");
		}
		/*if (DeviceIoControl(hLazy, TYPE_RANSOMEINFO, &(message.notification), sizeof(NOTIFICATION),
		resultBuf, sizeof(resultBuf), &ret_len, 0))
		{
		if (!strncmp(resultBuf, "YES", 3))
		{
		//printf("[Loader] stop ransomeware successfully.\n");
		}
		else
		{
		//printf("[Loader] stop ransomeware failed.\n");
		}
		}
		else
		{
		//printf("[Loader] DeviceIoControl failed.\n");
		}
		*/
	}
	return 0;
}

DWORD WINAPI InjectMonitor(
	_In_ LPVOID lpParameter
)
{
	DWORD ret_len = 0;
	INJECT_INFO injectInfo = { 0 };
	HANDLE hEvent = NULL;

	hEvent = CreateEvent(NULL, FALSE, FALSE, L"INJECT");
	if (hEvent == NULL)
	{
		//printf("[Loader] Create inject event failed.\n");
	}
	while (TRUE)
	{
		if (DeviceIoControl(hLazy, TYPE_INJECT, &hEvent, sizeof(HANDLE), NULL,
			0, &ret_len, NULL))
		{
			WaitForSingleObject(hEvent, INFINITE);
			if (DeviceIoControl(hLazy, TYPE_INJECT_RESULT, NULL, 0, (PVOID)&injectInfo,
				sizeof(INJECT_INFO), &ret_len, NULL))
			{
				
				//printf("[loader] process:%d inject process :%d.\n", injectInfo.injectProcess,
				//	injectInfo.injectedProcess);
			}
		}
	}
}