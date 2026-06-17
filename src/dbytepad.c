#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <richedit.h>
#include <shellapi.h>
#include <stdio.h>

#define APP_NAME L"DBYTEPAD"
#define EDIT_CLASS MSFTEDIT_CLASS
#define MAX_PATH_CHARS 32768

#define IDM_FILE_NEW 1001
#define IDM_FILE_OPEN 1002
#define IDM_FILE_SAVE 1003
#define IDM_FILE_SAVE_AS 1004
#define IDM_FILE_EXIT 1005
#define IDM_EDIT_UNDO 2001
#define IDM_EDIT_CUT 2002
#define IDM_EDIT_COPY 2003
#define IDM_EDIT_PASTE 2004
#define IDM_EDIT_SELECT_ALL 2005

static HINSTANCE g_instance;
static HWND g_main;
static HWND g_edit;
static HWND g_status;
static WCHAR g_path[MAX_PATH_CHARS];
static int g_dirty;
static int g_loading;

static void die_last_error(HWND owner, const WCHAR *context) {
    WCHAR message[512];
    DWORD error = GetLastError();
    swprintf(message, 512, L"%ls failed. Win32 error: %lu", context, error);
    MessageBoxW(owner, message, APP_NAME, MB_OK | MB_ICONERROR);
}

static void set_dirty(int dirty) {
    g_dirty = dirty;

    WCHAR title[MAX_PATH_CHARS + 64];
    const WCHAR *name = g_path[0] ? g_path : L"Untitled";
    swprintf(title, MAX_PATH_CHARS + 64, L"%ls%ls - %ls", g_dirty ? L"*" : L"", name, APP_NAME);
    SetWindowTextW(g_main, title);
}

static void update_status(void) {
    DWORD start = 0;
    DWORD end = 0;
    SendMessageW(g_edit, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);

    int line = (int)SendMessageW(g_edit, EM_LINEFROMCHAR, start, 0);
    int line_index = (int)SendMessageW(g_edit, EM_LINEINDEX, line, 0);
    int col = (int)start - line_index;

    GETTEXTLENGTHEX length_query;
    length_query.flags = GTL_NUMCHARS;
    length_query.codepage = 1200;
    LRESULT chars = SendMessageW(g_edit, EM_GETTEXTLENGTHEX, (WPARAM)&length_query, 0);

    WCHAR status[128];
    swprintf(status, 128, L"Ln %d, Col %d    Chars %ld", line + 1, col + 1, (long)chars);
    SetWindowTextW(g_status, status);
}

static void resize_children(HWND hwnd) {
    RECT rc;
    GetClientRect(hwnd, &rc);

    int status_h = 24;
    MoveWindow(g_status, 0, rc.bottom - status_h, rc.right, status_h, TRUE);
    MoveWindow(g_edit, 0, 0, rc.right, rc.bottom - status_h, TRUE);
}

static int ask_save_if_dirty(HWND hwnd);

static int read_entire_file(HWND hwnd, const WCHAR *path, WCHAR **out_text) {
    *out_text = NULL;

    HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        die_last_error(hwnd, L"Open file");
        return 0;
    }

    LARGE_INTEGER size;
    if (!GetFileSizeEx(file, &size) || size.QuadPart > 256LL * 1024LL * 1024LL) {
        CloseHandle(file);
        MessageBoxW(hwnd, L"File is too large for this build.", APP_NAME, MB_OK | MB_ICONERROR);
        return 0;
    }

    DWORD bytes = (DWORD)size.QuadPart;
    BYTE *buffer = (BYTE *)HeapAlloc(GetProcessHeap(), 0, bytes + 2);
    if (!buffer) {
        CloseHandle(file);
        MessageBoxW(hwnd, L"Out of memory.", APP_NAME, MB_OK | MB_ICONERROR);
        return 0;
    }

    DWORD read = 0;
    if (!ReadFile(file, buffer, bytes, &read, NULL) || read != bytes) {
        HeapFree(GetProcessHeap(), 0, buffer);
        CloseHandle(file);
        die_last_error(hwnd, L"Read file");
        return 0;
    }
    CloseHandle(file);

    buffer[bytes] = 0;
    buffer[bytes + 1] = 0;

    int offset = 0;
    UINT codepage = CP_UTF8;
    if (bytes >= 2 && buffer[0] == 0xFF && buffer[1] == 0xFE) {
        offset = 2;
        codepage = 1200;
    } else if (bytes >= 3 && buffer[0] == 0xEF && buffer[1] == 0xBB && buffer[2] == 0xBF) {
        offset = 3;
        codepage = CP_UTF8;
    }

    WCHAR *wide = NULL;
    if (codepage == 1200) {
        int count = (int)((bytes - offset) / 2);
        wide = (WCHAR *)HeapAlloc(GetProcessHeap(), 0, ((SIZE_T)count + 1) * sizeof(WCHAR));
        if (wide) {
            CopyMemory(wide, buffer + offset, (SIZE_T)count * sizeof(WCHAR));
            wide[count] = 0;
        }
    } else {
        int need = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, (LPCCH)(buffer + offset), (int)(bytes - offset), NULL, 0);
        if (need <= 0) {
            need = MultiByteToWideChar(CP_ACP, 0, (LPCCH)buffer, (int)bytes, NULL, 0);
            if (need <= 0) {
                HeapFree(GetProcessHeap(), 0, buffer);
                MessageBoxW(hwnd, L"Could not decode file text.", APP_NAME, MB_OK | MB_ICONERROR);
                return 0;
            }
            wide = (WCHAR *)HeapAlloc(GetProcessHeap(), 0, ((SIZE_T)need + 1) * sizeof(WCHAR));
            if (wide) {
                MultiByteToWideChar(CP_ACP, 0, (LPCCH)buffer, (int)bytes, wide, need);
                wide[need] = 0;
            }
        } else {
            wide = (WCHAR *)HeapAlloc(GetProcessHeap(), 0, ((SIZE_T)need + 1) * sizeof(WCHAR));
            if (wide) {
                MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, (LPCCH)(buffer + offset), (int)(bytes - offset), wide, need);
                wide[need] = 0;
            }
        }
    }

    HeapFree(GetProcessHeap(), 0, buffer);

    if (!wide) {
        MessageBoxW(hwnd, L"Out of memory.", APP_NAME, MB_OK | MB_ICONERROR);
        return 0;
    }

    *out_text = wide;
    return 1;
}

static int write_text_file(HWND hwnd, const WCHAR *path) {
    GETTEXTLENGTHEX length_query;
    length_query.flags = GTL_NUMCHARS;
    length_query.codepage = 1200;
    LRESULT chars = SendMessageW(g_edit, EM_GETTEXTLENGTHEX, (WPARAM)&length_query, 0);
    if (chars < 0) {
        MessageBoxW(hwnd, L"Could not get text length.", APP_NAME, MB_OK | MB_ICONERROR);
        return 0;
    }

    WCHAR *wide = (WCHAR *)HeapAlloc(GetProcessHeap(), 0, ((SIZE_T)chars + 1) * sizeof(WCHAR));
    if (!wide) {
        MessageBoxW(hwnd, L"Out of memory.", APP_NAME, MB_OK | MB_ICONERROR);
        return 0;
    }

    GETTEXTEX text_query;
    text_query.cb = (DWORD)(((SIZE_T)chars + 1) * sizeof(WCHAR));
    text_query.flags = GT_USECRLF;
    text_query.codepage = 1200;
    text_query.lpDefaultChar = NULL;
    text_query.lpUsedDefChar = NULL;
    SendMessageW(g_edit, EM_GETTEXTEX, (WPARAM)&text_query, (LPARAM)wide);

    int bytes_needed = WideCharToMultiByte(CP_UTF8, 0, wide, -1, NULL, 0, NULL, NULL);
    if (bytes_needed <= 0) {
        HeapFree(GetProcessHeap(), 0, wide);
        MessageBoxW(hwnd, L"Could not encode text as UTF-8.", APP_NAME, MB_OK | MB_ICONERROR);
        return 0;
    }

    char *utf8 = (char *)HeapAlloc(GetProcessHeap(), 0, (SIZE_T)bytes_needed);
    if (!utf8) {
        HeapFree(GetProcessHeap(), 0, wide);
        MessageBoxW(hwnd, L"Out of memory.", APP_NAME, MB_OK | MB_ICONERROR);
        return 0;
    }

    WideCharToMultiByte(CP_UTF8, 0, wide, -1, utf8, bytes_needed, NULL, NULL);
    HeapFree(GetProcessHeap(), 0, wide);

    HANDLE file = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        HeapFree(GetProcessHeap(), 0, utf8);
        die_last_error(hwnd, L"Create file");
        return 0;
    }

    DWORD to_write = (DWORD)(bytes_needed - 1);
    DWORD written = 0;
    int ok = WriteFile(file, utf8, to_write, &written, NULL) && written == to_write;
    CloseHandle(file);
    HeapFree(GetProcessHeap(), 0, utf8);

    if (!ok) {
        die_last_error(hwnd, L"Write file");
        return 0;
    }

    return 1;
}

static int choose_open_file(HWND hwnd, WCHAR *path, DWORD path_count) {
    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    path[0] = 0;

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = path;
    ofn.nMaxFile = path_count;
    ofn.lpstrFilter = L"Text Files\0*.txt;*.md;*.c;*.h;*.asm;*.bat\0All Files\0*.*\0";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

    return GetOpenFileNameW(&ofn) != 0;
}

static int choose_save_file(HWND hwnd, WCHAR *path, DWORD path_count) {
    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = path;
    ofn.nMaxFile = path_count;
    ofn.lpstrFilter = L"Text Files\0*.txt\0All Files\0*.*\0";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;

    return GetSaveFileNameW(&ofn) != 0;
}

static void do_new(HWND hwnd) {
    if (!ask_save_if_dirty(hwnd)) return;

    g_loading = 1;
    SetWindowTextW(g_edit, L"");
    g_loading = 0;
    g_path[0] = 0;
    set_dirty(0);
    update_status();
}

static int open_path(HWND hwnd, const WCHAR *path) {
    if (!ask_save_if_dirty(hwnd)) return 0;

    WCHAR *text = NULL;
    if (!read_entire_file(hwnd, path, &text)) return 0;

    g_loading = 1;
    SetWindowTextW(g_edit, text);
    g_loading = 0;
    HeapFree(GetProcessHeap(), 0, text);

    lstrcpynW(g_path, path, MAX_PATH_CHARS);
    set_dirty(0);
    update_status();
    return 1;
}

static void do_open(HWND hwnd) {
    WCHAR path[MAX_PATH_CHARS];
    if (choose_open_file(hwnd, path, MAX_PATH_CHARS)) {
        open_path(hwnd, path);
    }
}

static int do_save_as(HWND hwnd) {
    WCHAR path[MAX_PATH_CHARS];
    lstrcpynW(path, g_path, MAX_PATH_CHARS);

    if (!choose_save_file(hwnd, path, MAX_PATH_CHARS)) return 0;

    if (!write_text_file(hwnd, path)) return 0;

    lstrcpynW(g_path, path, MAX_PATH_CHARS);
    set_dirty(0);
    return 1;
}

static int do_save(HWND hwnd) {
    if (!g_path[0]) return do_save_as(hwnd);
    if (!write_text_file(hwnd, g_path)) return 0;
    set_dirty(0);
    return 1;
}

static int ask_save_if_dirty(HWND hwnd) {
    if (!g_dirty) return 1;

    int answer = MessageBoxW(hwnd, L"Save changes?", APP_NAME, MB_YESNOCANCEL | MB_ICONQUESTION);
    if (answer == IDCANCEL) return 0;
    if (answer == IDYES) return do_save(hwnd);
    return 1;
}

static HMENU make_menu(void) {
    HMENU menu = CreateMenu();
    HMENU file = CreatePopupMenu();
    HMENU edit = CreatePopupMenu();

    AppendMenuW(file, MF_STRING, IDM_FILE_NEW, L"New");
    AppendMenuW(file, MF_STRING, IDM_FILE_OPEN, L"Open...");
    AppendMenuW(file, MF_STRING, IDM_FILE_SAVE, L"Save");
    AppendMenuW(file, MF_STRING, IDM_FILE_SAVE_AS, L"Save As...");
    AppendMenuW(file, MF_SEPARATOR, 0, NULL);
    AppendMenuW(file, MF_STRING, IDM_FILE_EXIT, L"Exit");

    AppendMenuW(edit, MF_STRING, IDM_EDIT_UNDO, L"Undo");
    AppendMenuW(edit, MF_SEPARATOR, 0, NULL);
    AppendMenuW(edit, MF_STRING, IDM_EDIT_CUT, L"Cut");
    AppendMenuW(edit, MF_STRING, IDM_EDIT_COPY, L"Copy");
    AppendMenuW(edit, MF_STRING, IDM_EDIT_PASTE, L"Paste");
    AppendMenuW(edit, MF_SEPARATOR, 0, NULL);
    AppendMenuW(edit, MF_STRING, IDM_EDIT_SELECT_ALL, L"Select All");

    AppendMenuW(menu, MF_POPUP, (UINT_PTR)file, L"File");
    AppendMenuW(menu, MF_POPUP, (UINT_PTR)edit, L"Edit");
    return menu;
}

static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
    case WM_CREATE:
        g_edit = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            EDIT_CLASS,
            L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_WANTRETURN | ES_NOHIDESEL | ES_AUTOHSCROLL | ES_AUTOVSCROLL,
            0, 0, 0, 0,
            hwnd,
            NULL,
            g_instance,
            NULL);

        g_status = CreateWindowExW(0, L"STATIC", L"Ln 1, Col 1", WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP, 0, 0, 0, 0, hwnd, NULL, g_instance, NULL);

        if (!g_edit || !g_status) return -1;

        SendMessageW(g_edit, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
        SendMessageW(g_status, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
        SendMessageW(g_edit, EM_SETLIMITTEXT, 0, 0);
        DragAcceptFiles(hwnd, TRUE);
        set_dirty(0);
        update_status();
        return 0;

    case WM_SIZE:
        resize_children(hwnd);
        return 0;

    case WM_DROPFILES: {
        HDROP drop = (HDROP)wparam;
        WCHAR path[MAX_PATH_CHARS];
        if (DragQueryFileW(drop, 0, path, MAX_PATH_CHARS)) {
            open_path(hwnd, path);
        }
        DragFinish(drop);
        return 0;
    }

    case WM_COMMAND:
        if ((HWND)lparam == g_edit && HIWORD(wparam) == EN_CHANGE) {
            if (!g_loading) set_dirty(1);
            update_status();
            return 0;
        }

        switch (LOWORD(wparam)) {
        case IDM_FILE_NEW: do_new(hwnd); return 0;
        case IDM_FILE_OPEN: do_open(hwnd); return 0;
        case IDM_FILE_SAVE: do_save(hwnd); return 0;
        case IDM_FILE_SAVE_AS: do_save_as(hwnd); return 0;
        case IDM_FILE_EXIT: SendMessageW(hwnd, WM_CLOSE, 0, 0); return 0;
        case IDM_EDIT_UNDO: SendMessageW(g_edit, EM_UNDO, 0, 0); return 0;
        case IDM_EDIT_CUT: SendMessageW(g_edit, WM_CUT, 0, 0); return 0;
        case IDM_EDIT_COPY: SendMessageW(g_edit, WM_COPY, 0, 0); return 0;
        case IDM_EDIT_PASTE: SendMessageW(g_edit, WM_PASTE, 0, 0); return 0;
        case IDM_EDIT_SELECT_ALL: SendMessageW(g_edit, EM_SETSEL, 0, -1); return 0;
        }
        break;

    case WM_NOTIFY:
    case WM_KEYUP:
    case WM_LBUTTONUP:
        update_status();
        break;

    case WM_CLOSE:
        if (ask_save_if_dirty(hwnd)) DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previous, PWSTR command_line, int show) {
    (void)previous;
    (void)command_line;

    g_instance = instance;

    HMODULE rich = LoadLibraryW(L"Msftedit.dll");
    if (!rich) {
        MessageBoxW(NULL, L"Could not load Msftedit.dll.", APP_NAME, MB_OK | MB_ICONERROR);
        return 1;
    }

    WNDCLASSW wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc = window_proc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(NULL, IDC_IBEAM);
    wc.hIcon = LoadIconW(NULL, IDI_APPLICATION);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"DBYTEPAD_WINDOW";

    if (!RegisterClassW(&wc)) {
        die_last_error(NULL, L"Register window class");
        return 1;
    }

    g_main = CreateWindowExW(
        0,
        wc.lpszClassName,
        APP_NAME,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        900,
        650,
        NULL,
        make_menu(),
        instance,
        NULL);

    if (!g_main) {
        die_last_error(NULL, L"Create main window");
        return 1;
    }

    ShowWindow(g_main, show);
    UpdateWindow(g_main);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (int)msg.wParam;
}
