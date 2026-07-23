#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>
#include <stdio.h>
#include <string.h>
#include "chip8.h"

// Menu & Command Identifiers
#define ID_FILE_OPEN       1001
#define ID_FILE_RESET      1002
#define ID_FILE_EXIT       1003
#define ID_EMU_PAUSE       1004
#define ID_EMU_STEP        1005
#define ID_SPEED_SLOW      1010
#define ID_SPEED_NORMAL    1011
#define ID_SPEED_FAST      1012
#define ID_SPEED_TURBO     1013
#define ID_PALETTE_CLASSIC 1020
#define ID_PALETTE_GREEN   1021
#define ID_PALETTE_AMBER   1022
#define ID_PALETTE_CYBER   1023
#define ID_HELP_CONTROLS   1030
#define ID_HELP_ABOUT      1031

// Color Palette Definition
struct ColorPalette {
    COLORREF bg;
    COLORREF fg;
    const char* name;
};

static const ColorPalette PALETTES[] = {
    { RGB(15,  15,  20),  RGB(240, 240, 245), "Classic White" },
    { RGB(10,  26,  12),  RGB(57,  255, 20),  "Green Phosphor" },
    { RGB(28,  16,  0),   RGB(255, 176, 0),   "Amber CRT" },
    { RGB(18,  9,   36),  RGB(0,   240, 255), "Cyberpunk Cyan" }
};

// Global Emulator State
static chip8 g_chip8;
static bool g_romLoaded = false;
static bool g_isPaused = false;
static char g_romPath[MAX_PATH] = { 0 };
static char g_romFileName[256] = "No ROM Loaded";

static int g_targetHz = 500;            // Cycles per second
static int g_currentPaletteIndex = 0;   // Selected color palette
static int g_scaleFactor = 12;          // 64x32 scaled to 768x384

// Windows GDI Pixel Buffer (64 x 32)
static DWORD g_pixelBuffer[64 * 32];
static BITMAPINFO g_bmi;

// Keymap Translation: Virtual Key Code -> CHIP-8 Keypad Index (0x0 - 0xF)
static int MapVkToChip8Key(WPARAM vk) {
    switch (vk) {
        // Row 1: 1, 2, 3, 4 -> CHIP-8 1, 2, 3, C
        case '1': return 0x1;
        case '2': return 0x2;
        case '3': return 0x3;
        case '4': return 0xC;

        // Row 2: Q, W, E, R -> CHIP-8 4, 5, 6, D
        case 'Q': return 0x4;
        case 'W': return 0x5;
        case 'E': return 0x6;
        case 'R': return 0x7;

        // Row 3: A, S, D, F -> CHIP-8 7, 8, 9, E
        case 'A': return 0x8;
        case 'S': return 0x9;
        case 'D': return 0xD;
        case 'F': return 0xE;

        // Row 4: Z, X, C, V -> CHIP-8 A, 0, B, F
        case 'Z': return 0xA;
        case 'X': return 0x0;
        case 'C': return 0xB;
        case 'V': return 0xF;

        default: return -1;
    }
}

// Extract filename from full path
static void ExtractFileName(const char* fullPath, char* dest, size_t destSize) {
    const char* lastSlash = strrchr(fullPath, '\\');
    if (!lastSlash) lastSlash = strrchr(fullPath, '/');
    if (lastSlash) {
        strncpy(dest, lastSlash + 1, destSize - 1);
    } else {
        strncpy(dest, fullPath, destSize - 1);
    }
    dest[destSize - 1] = '\0';
}

// Load ROM into Chip8 engine
static void LoadRomFile(HWND hWnd, const char* filePath) {
    g_chip8.initialize();
    g_chip8.loadGame(filePath);

    strncpy(g_romPath, filePath, MAX_PATH - 1);
    ExtractFileName(filePath, g_romFileName, sizeof(g_romFileName));
    g_romLoaded = true;
    g_isPaused = false;

    char title[300];
    snprintf(title, sizeof(title), "CHIP-8 Emulator - [%s]", g_romFileName);
    SetWindowTextA(hWnd, title);

    InvalidateRect(hWnd, NULL, FALSE);
}

// Native File Chooser Dialog
static void PromptOpenFileDialog(HWND hWnd) {
    char szFile[MAX_PATH] = { 0 };

    OPENFILENAMEA ofn = { 0 };
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFilter = "CHIP-8 ROM Files (*.ch8;*.rom;*.bin)\0*.ch8;*.rom;*.bin\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrTitle = "Select CHIP-8 ROM File";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&ofn)) {
        LoadRomFile(hWnd, szFile);
    }
}

// Create Application Menu Bar
static HMENU CreateAppMenu() {
    HMENU hMenu = CreateMenu();

    // File Menu
    HMENU hFileMenu = CreatePopupMenu();
    AppendMenuA(hFileMenu, MF_STRING, ID_FILE_OPEN, "Open ROM...\tCtrl+O");
    AppendMenuA(hFileMenu, MF_STRING, ID_FILE_RESET, "Reset ROM\tCtrl+R");
    AppendMenuA(hFileMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuA(hFileMenu, MF_STRING, ID_FILE_EXIT, "Exit");
    AppendMenuA(hMenu, MF_POPUP, (UINT_PTR)hFileMenu, "&File");

    // Emulation Menu
    HMENU hEmuMenu = CreatePopupMenu();
    AppendMenuA(hEmuMenu, MF_STRING, ID_EMU_PAUSE, "Pause / Resume\tSpace");
    AppendMenuA(hEmuMenu, MF_STRING, ID_EMU_STEP, "Step Single Cycle\tF10");
    AppendMenuA(hMenu, MF_POPUP, (UINT_PTR)hEmuMenu, "&Emulation");

    // Speed Submenu
    HMENU hSpeedMenu = CreatePopupMenu();
    AppendMenuA(hSpeedMenu, MF_STRING, ID_SPEED_SLOW, "300 Hz (Slow)");
    AppendMenuA(hSpeedMenu, MF_STRING | MF_CHECKED, ID_SPEED_NORMAL, "500 Hz (Normal)");
    AppendMenuA(hSpeedMenu, MF_STRING, ID_SPEED_FAST, "800 Hz (Fast)");
    AppendMenuA(hSpeedMenu, MF_STRING, ID_SPEED_TURBO, "1200 Hz (Turbo)");
    AppendMenuA(hMenu, MF_POPUP, (UINT_PTR)hSpeedMenu, "&Speed");

    // Palette Submenu
    HMENU hPaletteMenu = CreatePopupMenu();
    AppendMenuA(hPaletteMenu, MF_STRING | MF_CHECKED, ID_PALETTE_CLASSIC, "Classic White");
    AppendMenuA(hPaletteMenu, MF_STRING, ID_PALETTE_GREEN, "Green Phosphor");
    AppendMenuA(hPaletteMenu, MF_STRING, ID_PALETTE_AMBER, "Amber CRT");
    AppendMenuA(hPaletteMenu, MF_STRING, ID_PALETTE_CYBER, "Cyberpunk Cyan");
    AppendMenuA(hMenu, MF_POPUP, (UINT_PTR)hPaletteMenu, "&Palette");

    // Help Menu
    HMENU hHelpMenu = CreatePopupMenu();
    AppendMenuA(hHelpMenu, MF_STRING, ID_HELP_CONTROLS, "Keypad Controls...");
    AppendMenuA(hHelpMenu, MF_STRING, ID_HELP_ABOUT, "About...");
    AppendMenuA(hMenu, MF_POPUP, (UINT_PTR)hHelpMenu, "&Help");

    return hMenu;
}

// Update menu checkmarks for Speed & Palette
static void UpdateMenuChecks(HWND hWnd) {
    HMENU hMenu = GetMenu(hWnd);
    if (!hMenu) return;

    // Speed checks
    CheckMenuItem(hMenu, ID_SPEED_SLOW,   (g_targetHz == 300)  ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, ID_SPEED_NORMAL, (g_targetHz == 500)  ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, ID_SPEED_FAST,   (g_targetHz == 800)  ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, ID_SPEED_TURBO,  (g_targetHz == 1200) ? MF_CHECKED : MF_UNCHECKED);

    // Palette checks
    CheckMenuItem(hMenu, ID_PALETTE_CLASSIC, (g_currentPaletteIndex == 0) ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, ID_PALETTE_GREEN,   (g_currentPaletteIndex == 1) ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, ID_PALETTE_AMBER,   (g_currentPaletteIndex == 2) ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, ID_PALETTE_CYBER,   (g_currentPaletteIndex == 3) ? MF_CHECKED : MF_UNCHECKED);
}

// Render CHIP-8 GFX buffer to GDI pixel buffer
static void UpdatePixelBuffer() {
    const ColorPalette& pal = PALETTES[g_currentPaletteIndex];

    // Convert COLORREF to 0x00RRGGBB format for GDI StretchDIBits
    DWORD fgColor = ((GetRValue(pal.fg) << 16) | (GetGValue(pal.fg) << 8) | GetBValue(pal.fg));
    DWORD bgColor = ((GetRValue(pal.bg) << 16) | (GetGValue(pal.bg) << 8) | GetBValue(pal.bg));

    for (int i = 0; i < 2048; i++) {
        g_pixelBuffer[i] = (g_chip8.gfx[i] != 0) ? fgColor : bgColor;
    }
}

// Render Welcome Overlay if no ROM loaded
static void RenderWelcomeScreen(HDC hdc, int clientWidth, int clientHeight) {
    const ColorPalette& pal = PALETTES[g_currentPaletteIndex];

    HBRUSH bgBrush = CreateSolidBrush(pal.bg);
    RECT clientRect = { 0, 0, clientWidth, clientHeight };
    FillRect(hdc, &clientRect, bgBrush);
    DeleteObject(bgBrush);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, pal.fg);

    HFONT hTitleFont = CreateFontA(28, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                  OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                  DEFAULT_PITCH | FF_DONTCARE, "Consolas");
    HFONT hSubFont   = CreateFontA(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                  OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                  DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");

    HFONT hOldFont = (HFONT)SelectObject(hdc, hTitleFont);

    RECT rect1 = { 0, clientHeight / 2 - 50, clientWidth, clientHeight / 2 - 10 };
    DrawTextA(hdc, "CHIP-8 EMULATOR", -1, &rect1, DT_CENTER | DT_SINGLELINE | DT_VCENTER);

    SelectObject(hdc, hSubFont);
    RECT rect2 = { 0, clientHeight / 2, clientWidth, clientHeight / 2 + 30 };
    DrawTextA(hdc, "Click File -> Open ROM (Ctrl+O) or Drag & Drop a .ch8 file here", -1, &rect2, DT_CENTER | DT_SINGLELINE | DT_VCENTER);

    RECT rect3 = { 0, clientHeight / 2 + 35, clientWidth, clientHeight / 2 + 65 };
    DrawTextA(hdc, "Keypad Layout: 1-4, Q-R, A-F, Z-V", -1, &rect3, DT_CENTER | DT_SINGLELINE | DT_VCENTER);

    SelectObject(hdc, hOldFont);
    DeleteObject(hTitleFont);
    DeleteObject(hSubFont);
}

// Window Procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            DragAcceptFiles(hWnd, TRUE);
            break;
        }

        case WM_DROPFILES: {
            HDROP hDrop = (HDROP)wParam;
            char droppedFile[MAX_PATH];
            if (DragQueryFileA(hDrop, 0, droppedFile, MAX_PATH)) {
                LoadRomFile(hWnd, droppedFile);
            }
            DragFinish(hDrop);
            break;
        }

        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            switch (wmId) {
                case ID_FILE_OPEN:
                    PromptOpenFileDialog(hWnd);
                    break;

                case ID_FILE_RESET:
                    if (g_romPath[0] != '\0') {
                        LoadRomFile(hWnd, g_romPath);
                    } else {
                        PromptOpenFileDialog(hWnd);
                    }
                    break;

                case ID_FILE_EXIT:
                    DestroyWindow(hWnd);
                    break;

                case ID_EMU_PAUSE:
                    g_isPaused = !g_isPaused;
                    InvalidateRect(hWnd, NULL, FALSE);
                    break;

                case ID_EMU_STEP:
                    if (g_romLoaded) {
                        g_chip8.emulateCycle();
                        InvalidateRect(hWnd, NULL, FALSE);
                    }
                    break;

                case ID_SPEED_SLOW:   g_targetHz = 300;  UpdateMenuChecks(hWnd); break;
                case ID_SPEED_NORMAL: g_targetHz = 500;  UpdateMenuChecks(hWnd); break;
                case ID_SPEED_FAST:   g_targetHz = 800;  UpdateMenuChecks(hWnd); break;
                case ID_SPEED_TURBO:  g_targetHz = 1200; UpdateMenuChecks(hWnd); break;

                case ID_PALETTE_CLASSIC: g_currentPaletteIndex = 0; UpdateMenuChecks(hWnd); InvalidateRect(hWnd, NULL, FALSE); break;
                case ID_PALETTE_GREEN:   g_currentPaletteIndex = 1; UpdateMenuChecks(hWnd); InvalidateRect(hWnd, NULL, FALSE); break;
                case ID_PALETTE_AMBER:   g_currentPaletteIndex = 2; UpdateMenuChecks(hWnd); InvalidateRect(hWnd, NULL, FALSE); break;
                case ID_PALETTE_CYBER:   g_currentPaletteIndex = 3; UpdateMenuChecks(hWnd); InvalidateRect(hWnd, NULL, FALSE); break;

                case ID_HELP_CONTROLS:
                    MessageBoxA(hWnd,
                        "CHIP-8 Keypad to PC Keyboard Mapping:\n\n"
                        "CHIP-8 Keypad:          PC Keyboard:\n"
                        "  [1] [2] [3] [C]   ->    [1] [2] [3] [4]\n"
                        "  [4] [5] [6] [D]   ->    [Q] [W] [E] [R]\n"
                        "  [7] [8] [9] [E]   ->    [A] [S] [D] [F]\n"
                        "  [A] [0] [B] [F]   ->    [Z] [X] [C] [V]\n\n"
                        "Shortcuts:\n"
                        "  Ctrl+O : Open ROM file\n"
                        "  Ctrl+R : Reset current ROM\n"
                        "  Space  : Pause / Resume",
                        "Keypad Controls Guide", MB_OK | MB_ICONINFORMATION);
                    break;

                case ID_HELP_ABOUT:
                    MessageBoxA(hWnd,
                        "CHIP-8 Emulator GUI\n"
                        "Native Win32 Pixel Renderer & CHIP-8 Interpreter\n\n"
                        "Screen Resolution: 64 x 32\n"
                        "Features: File Picker, Palette Switcher, Drag & Drop",
                        "About CHIP-8 Emulator", MB_OK | MB_ICONINFORMATION);
                    break;
            }
            break;
        }

        case WM_KEYDOWN:
        case WM_SYSKEYDOWN: {
            if (wParam == 'O' && (GetKeyState(VK_CONTROL) & 0x8000)) {
                PromptOpenFileDialog(hWnd);
                return 0;
            }
            if (wParam == 'R' && (GetKeyState(VK_CONTROL) & 0x8000)) {
                if (g_romPath[0] != '\0') LoadRomFile(hWnd, g_romPath);
                return 0;
            }
            if (wParam == VK_SPACE) {
                g_isPaused = !g_isPaused;
                InvalidateRect(hWnd, NULL, FALSE);
                return 0;
            }

            int keyIndex = MapVkToChip8Key(wParam);
            if (keyIndex >= 0 && keyIndex < 16) {
                g_chip8.key[keyIndex] = 1;
            }
            break;
        }

        case WM_KEYUP:
        case WM_SYSKEYUP: {
            int keyIndex = MapVkToChip8Key(wParam);
            if (keyIndex >= 0 && keyIndex < 16) {
                g_chip8.key[keyIndex] = 0;
            }
            break;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            RECT clientRect;
            GetClientRect(hWnd, &clientRect);
            int width = clientRect.right - clientRect.left;
            int height = clientRect.bottom - clientRect.top;

            if (!g_romLoaded) {
                RenderWelcomeScreen(hdc, width, height);
            } else {
                UpdatePixelBuffer();

                SetStretchBltMode(hdc, COLORONCOLOR);
                StretchDIBits(
                    hdc,
                    0, 0, width, height,            // Destination rectangle
                    0, 0, 64, 32,                   // Source rectangle
                    g_pixelBuffer,
                    &g_bmi,
                    DIB_RGB_COLORS,
                    SRCCOPY
                );

                if (g_isPaused) {
                    SetBkMode(hdc, TRANSPARENT);
                    SetTextColor(hdc, RGB(255, 60, 60));
                    HFONT hPauseFont = CreateFontA(24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                                  OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                                  DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
                    HFONT hOldFont = (HFONT)SelectObject(hdc, hPauseFont);
                    TextOutA(hdc, 10, 10, "[PAUSED]", 8);
                    SelectObject(hdc, hOldFont);
                    DeleteObject(hPauseFont);
                }
            }

            EndPaint(hWnd, &ps);
            break;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Initialize Bitmap Header for 64x32 Top-Down Bitmap
    memset(&g_bmi, 0, sizeof(BITMAPINFO));
    g_bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    g_bmi.bmiHeader.biWidth = 64;
    g_bmi.bmiHeader.biHeight = -32; // Negative indicates top-down
    g_bmi.bmiHeader.biPlanes = 1;
    g_bmi.bmiHeader.biBitCount = 32;
    g_bmi.bmiHeader.biCompression = BI_RGB;

    // Register Window Class
    WNDCLASSEXA wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "Chip8EmulatorWindowClass";
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassExA(&wc)) {
        MessageBoxA(NULL, "Window Registration Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    // Calculate Window Size for 64x32 scaled to 768x384 plus menu bar
    int clientWidth = 64 * g_scaleFactor;
    int clientHeight = 32 * g_scaleFactor;

    RECT windowRect = { 0, 0, clientWidth, clientHeight };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, TRUE);

    HWND hWnd = CreateWindowExA(
        WS_EX_CLIENTEDGE,
        "Chip8EmulatorWindowClass",
        "CHIP-8 Emulator - No ROM Loaded",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        NULL, CreateAppMenu(), hInstance, NULL
    );

    if (!hWnd) {
        MessageBoxA(NULL, "Window Creation Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    // If ROM path was passed as command line argument
    if (lpCmdLine && strlen(lpCmdLine) > 0) {
        char cleanPath[MAX_PATH];
        strncpy(cleanPath, lpCmdLine, MAX_PATH - 1);
        cleanPath[MAX_PATH - 1] = '\0';
        // Strip surrounding quotes if present
        if (cleanPath[0] == '"') {
            size_t len = strlen(cleanPath);
            if (len > 1 && cleanPath[len - 1] == '"') {
                cleanPath[len - 1] = '\0';
                memmove(cleanPath, cleanPath + 1, len - 1);
            }
        }
        LoadRomFile(hWnd, cleanPath);
    }

    // High Precision Performance Timer for Emulation Loop
    LARGE_INTEGER perfFreq, lastTime, currentTime;
    QueryPerformanceFrequency(&perfFreq);
    QueryPerformanceCounter(&lastTime);

    double timerAccumulator = 0.0;
    double cycleAccumulator = 0.0;

    const double timerInterval = 1.0 / 60.0; // 60 Hz for CHIP-8 delay/sound timers

    MSG msg;
    bool running = true;

    while (running) {
        // Handle Windows Messages
        while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                running = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

        if (!running) break;

        // Calculate Delta Time
        QueryPerformanceCounter(&currentTime);
        double deltaTime = (double)(currentTime.QuadPart - lastTime.QuadPart) / (double)perfFreq.QuadPart;
        lastTime = currentTime;

        // Cap maximum frame delta time to prevent spiraling after pauses
        if (deltaTime > 0.1) deltaTime = 0.1;

        if (g_romLoaded && !g_isPaused) {
            timerAccumulator += deltaTime;
            cycleAccumulator += deltaTime;

            // Update 60 Hz Timers
            while (timerAccumulator >= timerInterval) {
                if (g_chip8.delay_timer > 0) --g_chip8.delay_timer;
                if (g_chip8.sound_timer > 0) --g_chip8.sound_timer;
                timerAccumulator -= timerInterval;
            }

            // Execute CHIP-8 CPU Cycles
            double cycleInterval = 1.0 / (double)g_targetHz;
            while (cycleAccumulator >= cycleInterval) {
                g_chip8.emulateCycle();
                cycleAccumulator -= cycleInterval;
            }

            // Redraw window when drawFlag is set
            if (g_chip8.drawFlag) {
                InvalidateRect(hWnd, NULL, FALSE);
                g_chip8.drawFlag = false;
            }
        } else {
            Sleep(10); // Sleep briefly when paused or waiting for ROM
        }
    }

    return (int)msg.wParam;
}
