#include <Windows.h>
#include <shellapi.h>
#include <iostream>

// tray bar menu options identifiers
#define EXIT_ID 1001 // "exit" button ID
#define OPEN_TOOL_MANAGER_ID 1002 // "open tool manager" button ID

// tray bar menu icon options
NOTIFYICONDATA icon_data = {};

bool alwaysOnTopActivated = false;


// this function creates the popup menu from the tray bar menu icon
void CreateTrayMenu(HWND hwnd) {
    HMENU popupMenu = CreatePopupMenu();
    AppendMenu(popupMenu, MF_STRING, EXIT_ID, TEXT("Exit")); // create "Exit" option
    AppendMenu(popupMenu, MF_STRING, OPEN_TOOL_MANAGER_ID, TEXT("Open Tool Manager")); // create "Open Tool Manager" option

    // make the "exit" button and the "Open Tool Manager" button clickable
    POINT pt;
    GetCursorPos(&pt);
    SetForegroundWindow(hwnd);
    TrackPopupMenu(popupMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, NULL);
    PostMessage(hwnd, WM_NULL, 0, 0);
}

// this function assigns actions to tray bar menu options 
void HandleTrayMenu(HWND hwnd, WPARAM wParam) {
    int exit_choice = 0;
    switch (LOWORD(wParam)) {
        case EXIT_ID: // "Exit" button
            exit_choice = MessageBox(NULL, TEXT("Do you really want to exit?"), TEXT("Always On Top"), MB_YESNO | MB_ICONQUESTION);
            if (exit_choice == IDYES) { // Exit? Yes
                DestroyWindow(hwnd); // kill the application process
            } else if (exit_choice == IDNO) { // Exit? No
                MessageBox(NULL, TEXT("Operation cancelled."), TEXT("Always On Top"), MB_OK | MB_ICONINFORMATION);
            }
            break;
        case OPEN_TOOL_MANAGER_ID: // "Open Tool Manager" button
            MessageBox(NULL, TEXT("Tool Manager will be implemented soon..."), TEXT("Always On Top"), MB_OK | MB_ICONWARNING);
            break;
    }
}

// called from WinMain() when setting window class wc (to create the tray bar menu icon)
LRESULT CALLBACK TraybarIcon(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            icon_data.cbSize = sizeof(NOTIFYICONDATA);
            icon_data.hWnd = hwnd;
            icon_data.uID = 1;
            icon_data.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
            icon_data.uCallbackMessage = WM_USER + 1;

            // load the icon from the folder /graphics
            icon_data.hIcon = (HICON)LoadImage(NULL, TEXT("Icon\\Icon.ico"), IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
            if (!Shell_NotifyIcon(NIM_ADD, &icon_data)) {
                DWORD error = GetLastError();
                std::cerr << "Error while loading the tray bar menu icon: " << error << std::endl;
            }
            break;
        case WM_USER + 1:
            switch (lParam) {
                case WM_RBUTTONDOWN:
                case WM_CONTEXTMENU:
                    CreateTrayMenu(hwnd); // call the function to create the tray menu icon
                    break;
            }
            break;
        case WM_COMMAND:
            HandleTrayMenu(hwnd, wParam); // call the function to handle actions from the tray menu icon
            break;
        case WM_DESTROY:
            Shell_NotifyIcon(NIM_DELETE, &icon_data);
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// Toggle Always On Top status
void ToggleWindowAlwaysOnTop(HWND hwnd) {
    if (alwaysOnTopActivated) { // DISABLE Always On Top mode
        SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        alwaysOnTopActivated = false;
    } else { // ENABLE Always On Top mode
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        alwaysOnTopActivated = true;
    }
    InvalidateRect(hwnd, NULL, TRUE);
}

// background function of the thread (to read from keyboard and enable/disable Always On Top)
DWORD WINAPI BackgroundThread(LPVOID lpParam) {
    while (true) {
        // toggle Always On Top to the current window -> WIN + CTRL + T
        if ((GetAsyncKeyState(VK_LWIN) & 0x8000) && (GetAsyncKeyState(VK_CONTROL) & 0x8000) && (GetAsyncKeyState('T') & 0x8000)) {
            HWND hwnd = GetForegroundWindow();
            if (hwnd != NULL) {
                ToggleWindowAlwaysOnTop(hwnd);
            } else {
                std::cerr << "Unable to get active window handle..." << std::endl;
            }
            Sleep(300); // debounce to prevent multiple toggles from one keypress
        }
        Sleep(50);
    }

    return 0;
}

// MAIN function
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nShowCmd) {
    MessageBox(NULL, TEXT("Press WIN + CTRL + T to use!\nClick on the Traybar menu icon for Settings or to Exit :)"), TEXT("Always On Top"), MB_OK | MB_ICONINFORMATION);

    HANDLE hThread; // to manage the thread
    hThread = CreateThread(NULL, 0, BackgroundThread, NULL, 0, NULL); // create the thread in background (call the function BackgroundThread())
    if (hThread == NULL) {
        std::cerr << "Error while creating the thread...." << GetLastError() << std::endl;
        return 1;
    }

    // window class manage the tray bar menu icon and a lot of stuff 
    WNDCLASS wc = {};
    wc.lpfnWndProc = TraybarIcon; // see LRESULT CALLBACK TraybarIcon()
    wc.hInstance = hInstance;
    wc.lpszClassName = TEXT("TrayWindowClass");
    RegisterClass(&wc);

    // place the icon in the traybar menu (creating an invisible window)
    HWND g_hWnd = CreateWindow(wc.lpszClassName, TEXT("Tray Window"), 0, 0, 0, 0, 0, nullptr, nullptr, wc.hInstance, nullptr);
    if (!g_hWnd) {
        std::cerr << "Unable to create the icon..." << std::endl;
        return 1;
    }

    MSG msg;
    BOOL bRet;
    while ((bRet = GetMessage(&msg, nullptr, 0, 0)) != 0) {
        if (bRet == -1) {
            std::cerr << "Failure to receive the message..." << std::endl;
            break;
        } else {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return 0;
}