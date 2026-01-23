#include <Windows.h>
#include <iostream>
#include <unordered_map>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

using namespace std;


// if Windows 11 22H2+ (use DWMWA Border Color)
#ifndef DWMWA_BORDER_COLOR
#define DWMWA_BORDER_COLOR 34
#endif

// icon identifier (from resources)
constexpr auto IDI_APPICON1 = 101;

// tray menu IDs
constexpr int EXIT_ID = 1001; // "Exit" button ID
constexpr int OPEN_TOOL_MANAGER_ID = 1002; // "Open Tool Manager" button ID
constexpr int TOOL_COLOR_BASE_ID = 4000; // base ID for color buttons

NOTIFYICONDATA icon_data = {}; // tray bar icon data
unordered_map<HWND, COLORREF> alwaysOnTopWindows; // map of windows that are Always On Top and their border color
COLORREF currentBorderColor = RGB(0, 200, 0); // default color: green

// border color options
struct ColorOption {
    COLORREF color;
    const wchar_t* name;
};

ColorOption colorOptions[] = {
    { RGB(0, 200, 0),   L"Green" },
    { RGB(200, 0, 0),   L"Red" },
    { RGB(0, 120, 215), L"Blue" },
    { RGB(255, 255, 0), L"Yellow" },
    { RGB(255, 165, 0), L"Orange" },
    { RGB(128, 0, 128), L"Purple" },
    { RGB(0, 255, 255), L"Cyan" },
    { RGB(255, 192, 203), L"Pink" },
    { RGB(128, 128, 128), L"Gray" },
    { RGB(255, 255, 255), L"White" }
};


// set coloured borders for Always On Top windows
static void SetAlwaysOnTopBorder(HWND hwnd, bool enable, COLORREF color) {
    if (!hwnd) return;

    COLORREF c = enable ? color : RGB(0, 0, 0); // black if disabling
    DwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR, &c, sizeof(c));
}

// toggle Always On Top for a window
static void ToggleWindowAlwaysOnTop(HWND hwnd) {
    if (!hwnd) return;

    auto it = alwaysOnTopWindows.find(hwnd);

    if (it != alwaysOnTopWindows.end()) {
        // if the window is already topmost then remove Always On Top
        SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        SetAlwaysOnTopBorder(hwnd, false, it->second);
        alwaysOnTopWindows.erase(it);
    }
    else {
        // set window Always On Top
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        SetAlwaysOnTopBorder(hwnd, true, currentBorderColor);
        alwaysOnTopWindows.emplace(hwnd, currentBorderColor);
    }
}

// tool manager procedure
static LRESULT CALLBACK ToolManagerProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            int btnWidth = 80;
            int btnHeight = 25;
            int spacing = 10;

            int numCols = 2; // coloumns number
            int numRows = (sizeof(colorOptions) / sizeof(ColorOption) + numCols - 1) / numCols;

            // get window size
            RECT rcClient;
            GetClientRect(hwnd, &rcClient);
            int clientWidth = rcClient.right - rcClient.left;

            // total length with buttons
            int totalBtnWidth = numCols * btnWidth + (numCols - 1) * spacing;

            // distance from borders
            int marginX = (clientWidth - totalBtnWidth) / 2;
            int marginY = 20;

            // create buttons
            for (int i = 0; i < 10; ++i) {
                int x = marginX + (i % numCols) * (btnWidth + spacing);
                int y = marginY + (i / numCols) * (btnHeight + spacing);
                CreateWindow(TEXT("BUTTON"), colorOptions[i].name, WS_CHILD | WS_VISIBLE,
                    x, y, btnWidth, btnHeight, hwnd, (HMENU)(UINT_PTR)(TOOL_COLOR_BASE_ID + i), NULL, NULL);
            }
            break;
        }
        case WM_COMMAND: {
            int id = LOWORD(wParam);
            if (id >= TOOL_COLOR_BASE_ID && id < TOOL_COLOR_BASE_ID + 10) {
                int index = id - TOOL_COLOR_BASE_ID;
                currentBorderColor = colorOptions[index].color;

                // update all Always On Top windows
                for (auto& pair : alwaysOnTopWindows)
                    SetAlwaysOnTopBorder(pair.first, true, currentBorderColor);
            }
            break;
        }
        case WM_CLOSE: {
            DestroyWindow(hwnd);
            break;
        }
        case WM_DESTROY: {
            break;
        }
        default: {
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }
    }
    return 0;
}

// create the tool manager window
static void OpenToolManager(HWND parent) {
    WNDCLASS wc = {};
    wc.lpfnWndProc = ToolManagerProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = TEXT("ToolManagerWindowClass");
    RegisterClass(&wc);

    int width = 300;
    int height = 200;
    HWND hWnd = CreateWindow(wc.lpszClassName, TEXT("Tool Manager"),
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_SIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        parent, NULL, wc.hInstance, NULL);
    ShowWindow(hWnd, SW_SHOW);
}

// create tray menu
static void CreateTrayMenu(HWND hwnd) {
    HMENU popupMenu = CreatePopupMenu();

    // "Open Tool Manager" option
    AppendMenu(popupMenu, MF_STRING, OPEN_TOOL_MANAGER_ID, TEXT("Open Tool Manager"));

    // separator
    AppendMenu(popupMenu, MF_SEPARATOR, 0, NULL);

    // "Exit" option
    AppendMenu(popupMenu, MF_STRING, EXIT_ID, TEXT("Exit"));

    // show the popup menu at cursor position
    POINT pt;
    GetCursorPos(&pt);
    SetForegroundWindow(hwnd);
    TrackPopupMenu(popupMenu, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, NULL);
    PostMessage(hwnd, WM_NULL, 0, 0);
}

// handle tray menu actions
static void HandleTrayMenu(HWND hwnd, WPARAM wParam) {
    switch (LOWORD(wParam)) {
    case EXIT_ID: {
        int exit_choice = MessageBox(NULL, TEXT("Do you really want to exit?"), TEXT("Always On Top"), MB_YESNO | MB_ICONQUESTION);
        if (exit_choice == IDYES) DestroyWindow(hwnd);
        else MessageBox(NULL, TEXT("Operation cancelled."), TEXT("Always On Top"), MB_OK | MB_ICONINFORMATION);
        break;
    }
    case OPEN_TOOL_MANAGER_ID:
        OpenToolManager(hwnd);
        break;
    }
}

// tray icon window procedure
static LRESULT CALLBACK TraybarIcon(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        // set up the tray icon
        icon_data.cbSize = sizeof(NOTIFYICONDATA);
        icon_data.hWnd = hwnd;
        icon_data.uID = 1;
        icon_data.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        icon_data.uCallbackMessage = WM_USER + 1;
        // icon_data.hIcon = (HICON)LoadImage(NULL, TEXT("Graphics\\Icon.ico"), IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
        icon_data.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_APPICON1)); // load icon from resources

        if (!Shell_NotifyIcon(NIM_ADD, &icon_data))
            cerr << "Error while loading the tray icon: " << GetLastError() << endl;
        break;

    case WM_USER + 1:
        // if right click on tray icon, show menu
        if (lParam == WM_RBUTTONDOWN || lParam == WM_CONTEXTMENU)
            CreateTrayMenu(hwnd);
        break;

    case WM_COMMAND:
        // handle menu actions
        HandleTrayMenu(hwnd, wParam);
        break;

    case WM_DESTROY:
        // remove tray icon on exit
        Shell_NotifyIcon(NIM_DELETE, &icon_data);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// background thread: detect WIN + CTRL + T to toggle Always On Top
static DWORD WINAPI BackgroundThread(LPVOID lpParam) {
    while (true) {
        if ((GetAsyncKeyState(VK_LWIN) & 0x8000) &&
            (GetAsyncKeyState(VK_CONTROL) & 0x8000) &&
            (GetAsyncKeyState('T') & 0x8000)) {
            HWND hwnd = GetForegroundWindow();
            if (hwnd) ToggleWindowAlwaysOnTop(hwnd);
            Sleep(300);
        }
        Sleep(50);
    }
    return 0;
}

// MAIN
int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nCmdShow
) {
    MessageBox(NULL, TEXT("Press WIN + CTRL + T to toggle Always On Top!\nRight-click the tray icon for Settings or Exit."), TEXT("Always On Top"), MB_OK | MB_ICONINFORMATION);

    // create background thread for keyboard detection
    HANDLE hThread = CreateThread(NULL, 0, BackgroundThread, NULL, 0, NULL);
    if (!hThread) {
        cerr << "Failed to create background thread: " << GetLastError() << endl;
        return 1;
    }

    // register window class for tray icon
    WNDCLASS wc = {};
    wc.lpfnWndProc = TraybarIcon;
    wc.hInstance = hInstance;
    wc.lpszClassName = TEXT("TrayWindowClass");
    RegisterClass(&wc);

    // create invisible window to host the tray icon
    HWND g_hWnd = CreateWindow(wc.lpszClassName, TEXT("Tray Window"), 0, 0, 0, 0, 0, nullptr, nullptr, hInstance, nullptr);
    if (!g_hWnd) {
        cerr << "Failed to create tray window." << endl;
        return 1;
    }

    // message loop
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}