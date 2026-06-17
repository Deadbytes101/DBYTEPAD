#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <richedit.h>
#include <shellapi.h>
#include <strsafe.h>

#define APP_NAME L"DBYTEPAD"
#define EDIT_CLASS MSFTEDIT_CLASS
#define MAIN_CLASS L"DBYTEPAD_WINDOW"
#define MAX_PATH_CHARS 32768
#define STATUS_H 22

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
#define IDM_VIEW_WORD_WRAP 3001

static HINSTANCE g_instance;
static HWND g_main;
static HWND g_edit;
static HWND g_status;
static WCHAR g_path[MAX_PATH_CHARS];
static int g_dirty;
static int g_loading;
static int g_word_wrap = 1;

static int ask_save_if_dirty(HWND hwnd);
static int save_file(HWND hwnd);
static int save_file_as(HWND hwnd);

static const WCHAR *base_name(const WCHAR *path) {
    const WCHAR *name = path;
    const WCHAR *p = path;

    while (*p) {
        if (*p == L'\\' || *p == L'/') name = p + 1;
        p++;
    }

    return name;
}

static void show_last_error(HWND hwnd, const WCHAR *what) {
    WCHAR text[512];
    DWORD err = GetLastError();
    StringCchPrintfW(text, 512, L"%ls failed. Win32 error %lu.", what, err);
    MessageBoxW(hwnd, text, APP_NAME, MB_OK | MB_ICONERROR);
}

static void set_title(void) {
    WCHAR title[MAX_PATH_CHARS + 64];
    const WCHAR *name = g_path[0] ? base_name(g_path) : L"Untitled";
    StringCchPrintfW(title, MAX_PATH_CHARS + 64, L"%ls%ls - %ls", g_dirty ? L"*" : L"", name, APP_NAME);
    SetWindowTextW(g_main, title);
}

static void set_dirty(int dirty) {
    g_dirty = dirty;
    set_title();
}

static LRESULT text_chars(void) {
    GETTEXTLENGTHEX q;
    q.flags = GTL_NUMCHARS;
    q.codepage = 1200;
    return SendMessageW(g_edit, EM_GETTEXTLENGTHEX, (WPARAM)&q, 0);
}

static int utf8_byte_count(void) {
    LRESULT chars = text_chars();
    WCHAR *wide;
    int bytes;
    GETTEXTEX q;

    if (chars <= 0) return 0;

    wide = (WCHAR *)HeapAlloc(GetProcessHeap(), 0, ((SIZE_T)chars + 1) * sizeof(WCHAR));
    if (!wide) return 0;

    q.cb = (DWORD)(((SIZE_T)chars + 1) * sizeof(WCHAR));
    q.flags = GT_USECRLF;
    q.codepage = 1200;
    q.lpDefaultChar = NULL;
    q.lpUsedDefChar = NULL;
    SendMessageW(g_edit, EM_GETTEXTEX, (WPARAM)&q, (LPARAM)wide);

    bytes = WideCharToMultiByte(CP_UTF8, 0, wide, -1, NULL, 0, NULL, NULL);
    HeapFree(GetProcessHeap(), 0, wide);

    return bytes > 0 ? bytes - 1 : 0;
}

static void update_status(void) {
    DWORD start = 0;
    DWORD end = 0;
    int line;
    int line_start;
    int col;
    WCHAR text[256];
    LRESULT chars;
    int bytes;

    SendMessageW(g_edit, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);

    line = (int)SendMessageW(g_edit, EM_LINEFROMCHAR, (WPARAM)start, 0);
    line_start = (int)SendMessageW(g_edit, EM_LINEINDEX, (WPARAM)line, 0);
    col = (int)start - line_start;
    chars = text_chars();
    bytes = utf8_byte_count();

    StringCchPrintfW(
        text,
        256,
        L"Ln %d, Col %d    Chars %lld    UTF-8 bytes %d    %ls",
        line + 1,
        col + 1,
        (long long)chars,
        bytes,
        g_dirty ? L"modified" : L"saved");

    SetWindowTextW(g_status, text);
}

static void resize_parts(HWND hwnd) {
    RECT rc;
    int h;

    GetClientRect(hwnd, &rc);
    h = rc.bottom - STATUS_H;
    if (h < 0) h = 0;

    MoveWindow(g_edit, 0, 0, rc.right, h, TRUE);
    MoveWindow(g_status, 0, h, rc.right, STATUS_H, TRUE);
}

static void set_edit_format(void) {
    CHARFORMAT2W cf;
    ZeroMemory(&cf, sizeof(cf));

    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_FACE | CFM_SIZE;
    cf.yHeight = 210;
    StringCchCopyW(cf.szFaceName, LF_FACESIZE, L"Cascadia Mono");

    SendMessageW(g_edit, EM_SETCHARFORMAT, SCF_DEFAULT, (LPARAM)&cf);
    SendMessageW(g_edit, EM_EXLIMITTEXT, 0, 0x7FFFFFFE);
}

static void apply_word_wrap(void) {
    DWORD style = (DWORD)GetWindowLongPtrW(g_edit, GWL_STYLE);

    if (g_word_wrap) {
        style &= ~(WS_HSCROLL | ES_AUTOHSCROLL);
    } else {
        style |= WS_HSCROLL | ES_AUTOHSCROLL;
    }

    SetWindowLongPtrW(g_edit, GWL_STYLE, (LONG_PTR)style);
    SetWindowPos(g_edit, NULL, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
}

static int read_text(HWND hwnd, const WCHAR *path, WCHAR **out) {
    HANDLE file;
    LARGE_INTEGER size;
    DWORD len;
    DWORD got;
    BYTE *bytes;
    WCHAR *wide;
    int off;
    UINT cp;

    *out = NULL;

    file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        show_last_error(hwnd, L"Open file");
        return 0;
    }

    if (!GetFileSizeEx(file, &size) || size.QuadPart > 256LL * 1024LL * 1024LL) {
        CloseHandle(file);
        MessageBoxW(hwnd, L"File is too large for this build.", APP_NAME, MB_OK | MB_ICONERROR);
        return 0;
    }

    len = (DWORD)size.QuadPart;
    if (len == 0) {
        CloseHandle(file);
        wide = (WCHAR *)HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR));
        if (!wide) {
            MessageBoxW(hwnd, L"Out of memory.", APP_NAME, MB_OK | MB_ICONERROR);
            return 0;
        }
        wide[0] = 0;
        *out = wide;
        return 1;
    }

    bytes = (BYTE *)HeapAlloc(GetProcessHeap(), 0, (SIZE_T)len + 2);
    if (!bytes) {
        CloseHandle(file);
        MessageBoxW(hwnd, L"Out of memory.", APP_NAME, MB_OK | MB_ICONERROR);
        return 0;
    }

    got = 0;
    if (!ReadFile(file, bytes, len, &got, NULL) || got != len) {
        HeapFree(GetProcessHeap(), 0, bytes);
        CloseHandle(file);
        show_last_error(hwnd, L"Read file");
        return 0;
    }
    CloseHandle(file);

    bytes[len] = 0;
    bytes[len + 1] = 0;

    off = 0;
    cp = CP_UTF8;

    if (len >= 2 && bytes[0] == 0xFF && bytes[1] == 0xFE) {
        off = 2;
        cp = 1200;
    } else if (len >= 3 && bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF) {
        off = 3;
    }

    wide = NULL;

    if (cp == 1200) {
        int count = (int)((len - (DWORD)off) / 2);
        wide = (WCHAR *)HeapAlloc(GetProcessHeap(), 0, ((SIZE_T)count + 1) * sizeof(WCHAR));
        if (wide) {
            CopyMemory(wide, bytes + off, (SIZE_T)count * sizeof(WCHAR));
            wide[count] = 0;
        }
    } else {
        int need = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, (LPCCH)(bytes + off), (int)(len - (DWORD)off), NULL, 0);
        if (need <= 0) {
            need = MultiByteToWideChar(CP_ACP, 0, (LPCCH)bytes, (int)len, NULL, 0);
            if (need > 0) {
                wide = (WCHAR *)HeapAlloc(GetProcessHeap(), 0, ((SIZE_T)need + 1) * sizeof(WCHAR));
                if (wide) {
                    MultiByteToWideChar(CP_ACP, 0, (LPCCH)bytes, (int)len, wide, need);
                    wide[need] = 0;
                }
            }
        } else {
            wide = (WCHAR *)HeapAlloc(GetProcessHeap(), 0, ((SIZE_T)need + 1) * sizeof(WCHAR));
            if (wide) {
                MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, (LPCCH)(bytes + off), (int)(len - (DWORD)off), wide, need);
                wide[need] = 0;
            }
        }
    }

    HeapFree(GetProcessHeap(), 0, bytes);

    if (!wide) {
        MessageBoxW(hwnd, L"Could not decode file text.", APP_NAME, MB_OK | MB_ICONERROR);
        return 0;
    }

    *out = wide;
    return 1;
}

static int write_text(HWND hwnd, const WCHAR *path) {
    LRESULT chars = text_chars();
    WCHAR *wide;
    char *utf8;
    int need;
    DWORD written;
    DWORD want;
    HANDLE file;
    GETTEXTEX q;
    int ok;

    if (chars < 0) return 0;

    wide = (WCHAR *)HeapAlloc(GetProcessHeap(), 0, ((SIZE_T)chars + 1) * sizeof(WCHAR));
    if (!wide) {
        MessageBoxW(hwnd, L"Out of memory.", APP_NAME, MB_OK | MB_ICONERROR);
        return 0;
    }

    q.cb = (DWORD)(((SIZE_T)chars + 1) * sizeof(WCHAR));
    q.flags = GT_USECRLF;
    q.codepage = 1200;
    q.lpDefaultChar = NULL;
    q.lpUsedDefChar = NULL;
    SendMessageW(g_edit, EM_GETTEXTEX, (WPARAM)&q, (LPARAM)wide);

    need = WideCharToMultiByte(CP_UTF8, 0, wide, -1, NULL, 0, NULL, NULL);
    if (need <= 0) {
        HeapFree(GetProcessHeap(), 0, wide);
        MessageBoxW(hwnd, L"Could not encode text as UTF-8.", APP_NAME, MB_OK | MB_ICONERROR);
        return 0;
    }

    utf8 = (char *)HeapAlloc(GetProcessHeap(), 0, (SIZE_T)need);
    if (!utf8) {
        HeapFree(GetProcessHeap(), 0, wide);
        MessageBoxW(hwnd, L"Out of memory.", APP_NAME, MB_OK | MB_ICONERROR);
        return 0;
    }

    WideCharToMultiByte(CP_UTF8, 0, wide, -1, utf8, need, NULL, NULL);
    HeapFree(GetProcessHeap(), 0, wide);

    file = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        HeapFree(GetProcessHeap(), 0, utf8);
        show_last_error(hwnd, L"Create file");
        return 0;
    }

    want = (DWORD)(need - 1);
    written = 0;
    ok = WriteFile(file, utf8, want, &written, NULL) && written == want;
    CloseHandle(file);
    HeapFree(GetProcessHeap(), 0, utf8);

    if (!ok) {
        show_last_error(hwnd, L"Write file");
        return 0;
    }

    return 1;
}

static int choose_open(HWND hwnd, WCHAR *path) {
    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    path[0] = 0;

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = path;
    ofn.nMaxFile = MAX_PATH_CHARS;
    ofn.lpstrFilter = L"Text files\0*.txt;*.md;*.c;*.h;*.asm;*.bat\0All files\0*.*\0";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

    return GetOpenFileNameW(&ofn) != 0;
}

static int choose_save(HWND hwnd, WCHAR *path) {
    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = path;
    ofn.nMaxFile = MAX_PATH_CHARS;
    ofn.lpstrFilter = L"Text files\0*.txt\0All files\0*.*\0";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;

    return GetSaveFileNameW(&ofn) != 0;
}

static void new_file(HWND hwnd) {
    if (!ask_save_if_dirty(hwnd)) return;

    g_loading = 1;
    SetWindowTextW(g_edit, L"");
    g_loading = 0;
    g_path[0] = 0;
    set_dirty(0);
    update_status();
}

static int open_path(HWND hwnd, const WCHAR *path) {
    WCHAR *text = NULL;

    if (!ask_save_if_dirty(hwnd)) return 0;
    if (!read_text(hwnd, path, &text)) return 0;

    g_loading = 1;
    SetWindowTextW(g_edit, text);
    g_loading = 0;
    HeapFree(GetProcessHeap(), 0, text);

    StringCchCopyW(g_path, MAX_PATH_CHARS, path);
    set_dirty(0);
    update_status();
    return 1;
}

static void open_file(HWND hwnd) {
    WCHAR path[MAX_PATH_CHARS];

    if (choose_open(hwnd, path)) {
        open_path(hwnd, path);
    }
}

static int save_file_as(HWND hwnd) {
    WCHAR path[MAX_PATH_CHARS];
    StringCchCopyW(path, MAX_PATH_CHARS, g_path);

    if (!choose_save(hwnd, path)) return 0;
    if (!write_text(hwnd, path)) return 0;

    StringCchCopyW(g_path, MAX_PATH_CHARS, path);
    set_dirty(0);
    update_status();
    return 1;
}

static int save_file(HWND hwnd) {
    if (!g_path[0]) return save_file_as(hwnd);
    if (!write_text(hwnd, g_path)) return 0;

    set_dirty(0);
    update_status();
    return 1;
}

static int ask_save_if_dirty(HWND hwnd) {
    int answer;

    if (!g_dirty) return 1;

    answer = MessageBoxW(hwnd, L"Save changes?", APP_NAME, MB_YESNOCANCEL | MB_ICONQUESTION);
    if (answer == IDCANCEL) return 0;
    if (answer == IDYES) return save_file(hwnd);
    return 1;
}

static HMENU make_menu(void) {
    HMENU menu = CreateMenu();
    HMENU file = CreatePopupMenu();
    HMENU edit = CreatePopupMenu();
    HMENU view = CreatePopupMenu();

    AppendMenuW(file, MF_STRING, IDM_FILE_NEW, L"New\tCtrl+N");
    AppendMenuW(file, MF_STRING, IDM_FILE_OPEN, L"Open...\tCtrl+O");
    AppendMenuW(file, MF_STRING, IDM_FILE_SAVE, L"Save\tCtrl+S");
    AppendMenuW(file, MF_STRING, IDM_FILE_SAVE_AS, L"Save As...\tCtrl+Shift+S");
    AppendMenuW(file, MF_SEPARATOR, 0, NULL);
    AppendMenuW(file, MF_STRING, IDM_FILE_EXIT, L"Exit");

    AppendMenuW(edit, MF_STRING, IDM_EDIT_UNDO, L"Undo\tCtrl+Z");
    AppendMenuW(edit, MF_SEPARATOR, 0, NULL);
    AppendMenuW(edit, MF_STRING, IDM_EDIT_CUT, L"Cut\tCtrl+X");
    AppendMenuW(edit, MF_STRING, IDM_EDIT_COPY, L"Copy\tCtrl+C");
    AppendMenuW(edit, MF_STRING, IDM_EDIT_PASTE, L"Paste\tCtrl+V");
    AppendMenuW(edit, MF_SEPARATOR, 0, NULL);
    AppendMenuW(edit, MF_STRING, IDM_EDIT_SELECT_ALL, L"Select All\tCtrl+A");

    AppendMenuW(view, MF_CHECKED | MF_STRING, IDM_VIEW_WORD_WRAP, L"Word Wrap");

    AppendMenuW(menu, MF_POPUP, (UINT_PTR)file, L"File");
    AppendMenuW(menu, MF_POPUP, (UINT_PTR)edit, L"Edit");
    AppendMenuW(menu, MF_POPUP, (UINT_PTR)view, L"View");
    return menu;
}

static void run_command(HWND hwnd, WORD id) {
    switch (id) {
    case IDM_FILE_NEW: new_file(hwnd); break;
    case IDM_FILE_OPEN: open_file(hwnd); break;
    case IDM_FILE_SAVE: save_file(hwnd); break;
    case IDM_FILE_SAVE_AS: save_file_as(hwnd); break;
    case IDM_FILE_EXIT: SendMessageW(hwnd, WM_CLOSE, 0, 0); break;
    case IDM_EDIT_UNDO: SendMessageW(g_edit, EM_UNDO, 0, 0); break;
    case IDM_EDIT_CUT: SendMessageW(g_edit, WM_CUT, 0, 0); break;
    case IDM_EDIT_COPY: SendMessageW(g_edit, WM_COPY, 0, 0); break;
    case IDM_EDIT_PASTE: SendMessageW(g_edit, WM_PASTE, 0, 0); break;
    case IDM_EDIT_SELECT_ALL: SendMessageW(g_edit, EM_SETSEL, 0, -1); break;
    case IDM_VIEW_WORD_WRAP:
        g_word_wrap = !g_word_wrap;
        CheckMenuItem(GetMenu(hwnd), IDM_VIEW_WORD_WRAP, g_word_wrap ? MF_CHECKED : MF_UNCHECKED);
        apply_word_wrap();
        resize_parts(hwnd);
        break;
    default: break;
    }
}

static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
    case WM_CREATE:
        g_edit = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            EDIT_CLASS,
            L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_WANTRETURN | ES_NOHIDESEL | ES_AUTOVSCROLL,
            0, 0, 0, 0,
            hwnd,
            NULL,
            g_instance,
            NULL);

        g_status = CreateWindowExW(
            0,
            L"STATIC",
            L"",
            WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP,
            0, 0, 0, 0,
            hwnd,
            NULL,
            g_instance,
            NULL);

        if (!g_edit || !g_status) return -1;

        SendMessageW(g_edit, EM_SETEVENTMASK, 0, ENM_CHANGE | ENM_SELCHANGE);
        set_edit_format();
        DragAcceptFiles(hwnd, TRUE);
        set_dirty(0);
        update_status();
        return 0;

    case WM_SIZE:
        resize_parts(hwnd);
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

        run_command(hwnd, LOWORD(wparam));
        return 0;

    case WM_NOTIFY:
        if (((NMHDR *)lparam)->hwndFrom == g_edit && ((NMHDR *)lparam)->code == EN_SELCHANGE) {
            update_status();
            return 0;
        }
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

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE prev, PWSTR cmd, int show) {
    WNDCLASSW wc;
    HWND hwnd;
    HACCEL accel;
    MSG msg;
    ACCEL accels[] = {
        { FVIRTKEY | FCONTROL, 'N', IDM_FILE_NEW },
        { FVIRTKEY | FCONTROL, 'O', IDM_FILE_OPEN },
        { FVIRTKEY | FCONTROL, 'S', IDM_FILE_SAVE },
        { FVIRTKEY | FCONTROL | FSHIFT, 'S', IDM_FILE_SAVE_AS },
        { FVIRTKEY | FCONTROL, 'A', IDM_EDIT_SELECT_ALL },
        { FVIRTKEY | FCONTROL, 'Z', IDM_EDIT_UNDO },
        { FVIRTKEY | FCONTROL, 'X', IDM_EDIT_CUT },
        { FVIRTKEY | FCONTROL, 'C', IDM_EDIT_COPY },
        { FVIRTKEY | FCONTROL, 'V', IDM_EDIT_PASTE },
    };

    (void)prev;
    (void)cmd;

    g_instance = instance;

    if (!LoadLibraryW(L"Msftedit.dll")) {
        MessageBoxW(NULL, L"Could not load Msftedit.dll.", APP_NAME, MB_OK | MB_ICONERROR);
        return 1;
    }

    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc = window_proc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(NULL, IDC_IBEAM);
    wc.hIcon = LoadIconW(NULL, IDI_APPLICATION);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = MAIN_CLASS;

    if (!RegisterClassW(&wc)) {
        show_last_error(NULL, L"RegisterClass");
        return 1;
    }

    hwnd = CreateWindowExW(
        0,
        MAIN_CLASS,
        APP_NAME,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        900,
        620,
        NULL,
        make_menu(),
        instance,
        NULL);

    if (!hwnd) {
        show_last_error(NULL, L"CreateWindow");
        return 1;
    }

    g_main = hwnd;
    accel = CreateAcceleratorTableW(accels, (int)(sizeof(accels) / sizeof(accels[0])));

    ShowWindow(hwnd, show);
    UpdateWindow(hwnd);

    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        if (!accel || !TranslateAcceleratorW(hwnd, accel, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    if (accel) DestroyAcceleratorTable(accel);
    return (int)msg.wParam;
}
