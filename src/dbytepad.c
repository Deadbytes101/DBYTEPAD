#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <richedit.h>
#include <shellapi.h>
#include <stdio.h>

#define APP_NAME L"DBYTEPAD"
#define EDIT_CLASS MSFTEDIT_CLASS
#define MAX_PATH_CHARS 32768
#define TOP_BAR_H 48
#define STATUS_H 28
#define GUTTER_W 54

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

#define ID_BAR 3001
#define ID_GUTTER 3002
#define ID_STATUS 3003
#define ID_EDIT 3004

#define CLASS_MAIN L"DBYTEPAD_WINDOW"
#define CLASS_BAR L"DBYTEPAD_BAR"
#define CLASS_GUTTER L"DBYTEPAD_GUTTER"
#define CLASS_STATUS L"DBYTEPAD_STATUS"

#define COLOR_BACK RGB(18, 18, 20)
#define COLOR_PANEL RGB(26, 27, 31)
#define COLOR_EDGE RGB(48, 51, 58)
#define COLOR_TEXT RGB(230, 232, 236)
#define COLOR_MUTED RGB(144, 151, 160)
#define COLOR_HOT RGB(94, 126, 255)
#define COLOR_EDIT_BACK RGB(250, 250, 247)
#define COLOR_EDIT_TEXT RGB(22, 24, 28)

typedef struct ButtonSpec {
    int id;
    int x;
    int w;
    const WCHAR *label;
} ButtonSpec;

static const ButtonSpec g_buttons[] = {
    { IDM_FILE_NEW, 132, 58, L"NEW" },
    { IDM_FILE_OPEN, 198, 70, L"OPEN" },
    { IDM_FILE_SAVE, 276, 70, L"SAVE" },
    { IDM_FILE_SAVE_AS, 354, 92, L"SAVE AS" },
};

static HINSTANCE g_instance;
static HWND g_main;
static HWND g_bar;
static HWND g_gutter;
static HWND g_edit;
static HWND g_status;
static HFONT g_font_ui;
static HFONT g_font_mono;
static HBRUSH g_brush_back;
static HBRUSH g_brush_panel;
static HBRUSH g_brush_edit;
static WCHAR g_path[MAX_PATH_CHARS];
static WCHAR g_status_text[160];
static int g_dirty;
static int g_loading;

static int ask_save_if_dirty(HWND hwnd);
static int do_save(HWND hwnd);
static int do_save_as(HWND hwnd);
static void do_open(HWND hwnd);
static void do_new(HWND hwnd);

static const WCHAR *file_name_of(const WCHAR *path) {
    const WCHAR *name = path;
    const WCHAR *p = path;

    while (*p) {
        if (*p == L'\\' || *p == L'/') name = p + 1;
        p++;
    }

    return name;
}

static void die_last_error(HWND owner, const WCHAR *context) {
    WCHAR message[512];
    DWORD error = GetLastError();
    swprintf(message, 512, L"%ls failed. Win32 error: %lu", context, error);
    MessageBoxW(owner, message, APP_NAME, MB_OK | MB_ICONERROR);
}

static void repaint_chrome(void) {
    if (g_bar) InvalidateRect(g_bar, NULL, TRUE);
    if (g_gutter) InvalidateRect(g_gutter, NULL, TRUE);
    if (g_status) InvalidateRect(g_status, NULL, TRUE);
}

static void set_title(void) {
    WCHAR title[MAX_PATH_CHARS + 64];
    const WCHAR *name = g_path[0] ? file_name_of(g_path) : L"Untitled";
    swprintf(title, MAX_PATH_CHARS + 64, L"%ls%ls - %ls", g_dirty ? L"*" : L"", name, APP_NAME);
    SetWindowTextW(g_main, title);
}

static void set_dirty(int dirty) {
    g_dirty = dirty;
    set_title();
    repaint_chrome();
}

static void update_status(void) {
    DWORD start = 0;
    DWORD end = 0;
    SendMessageW(g_edit, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);

    int line = (int)SendMessageW(g_edit, EM_LINEFROMCHAR, start, 0);
    int line_index = (int)SendMessageW(g_edit, EM_LINEINDEX, (WPARAM)line, 0);
    int col = (int)start - line_index;

    GETTEXTLENGTHEX length_query;
    length_query.flags = GTL_NUMCHARS;
    length_query.codepage = 1200;
    LRESULT chars = SendMessageW(g_edit, EM_GETTEXTLENGTHEX, (WPARAM)&length_query, 0);

    swprintf(g_status_text, 160, L"Ln %d    Col %d    Chars %ld    %ls", line + 1, col + 1, (long)chars, g_dirty ? L"DIRTY" : L"CLEAN");
    repaint_chrome();
}

static void resize_children(HWND hwnd) {
    RECT rc;
    int edit_h;

    GetClientRect(hwnd, &rc);
    edit_h = rc.bottom - TOP_BAR_H - STATUS_H;
    if (edit_h < 0) edit_h = 0;

    MoveWindow(g_bar, 0, 0, rc.right, TOP_BAR_H, TRUE);
    MoveWindow(g_gutter, 0, TOP_BAR_H, GUTTER_W, edit_h, TRUE);
    MoveWindow(g_edit, GUTTER_W, TOP_BAR_H, rc.right - GUTTER_W, edit_h, TRUE);
    MoveWindow(g_status, 0, rc.bottom - STATUS_H, rc.right, STATUS_H, TRUE);
}

static void set_edit_face(void) {
    CHARFORMAT2W cf;
    ZeroMemory(&cf, sizeof(cf));
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_COLOR | CFM_FACE | CFM_SIZE;
    cf.crTextColor = COLOR_EDIT_TEXT;
    cf.yHeight = 210;
    lstrcpynW(cf.szFaceName, L"Cascadia Mono", LF_FACESIZE);

    SendMessageW(g_edit, EM_SETBKGNDCOLOR, 0, COLOR_EDIT_BACK);
    SendMessageW(g_edit, EM_SETCHARFORMAT, SCF_DEFAULT, (LPARAM)&cf);
    SendMessageW(g_edit, EM_EXLIMITTEXT, 0, 0x7FFFFFFE);
}

static int read_entire_file(HWND hwnd, const WCHAR *path, WCHAR **out_text) {
    HANDLE file;
    LARGE_INTEGER size;
    DWORD bytes;
    BYTE *buffer;
    DWORD read;
    int offset;
    UINT codepage;
    WCHAR *wide;

    *out_text = NULL;

    file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        die_last_error(hwnd, L"Open file");
        return 0;
    }

    if (!GetFileSizeEx(file, &size) || size.QuadPart > 256LL * 1024LL * 1024LL) {
        CloseHandle(file);
        MessageBoxW(hwnd, L"File is too large for this build.", APP_NAME, MB_OK | MB_ICONERROR);
        return 0;
    }

    bytes = (DWORD)size.QuadPart;
    buffer = (BYTE *)HeapAlloc(GetProcessHeap(), 0, (SIZE_T)bytes + 2);
    if (!buffer) {
        CloseHandle(file);
        MessageBoxW(hwnd, L"Out of memory.", APP_NAME, MB_OK | MB_ICONERROR);
        return 0;
    }

    read = 0;
    if (!ReadFile(file, buffer, bytes, &read, NULL) || read != bytes) {
        HeapFree(GetProcessHeap(), 0, buffer);
        CloseHandle(file);
        die_last_error(hwnd, L"Read file");
        return 0;
    }
    CloseHandle(file);

    buffer[bytes] = 0;
    buffer[bytes + 1] = 0;

    offset = 0;
    codepage = CP_UTF8;
    if (bytes >= 2 && buffer[0] == 0xFF && buffer[1] == 0xFE) {
        offset = 2;
        codepage = 1200;
    } else if (bytes >= 3 && buffer[0] == 0xEF && buffer[1] == 0xBB && buffer[2] == 0xBF) {
        offset = 3;
        codepage = CP_UTF8;
    }

    wide = NULL;
    if (codepage == 1200) {
        int count = (int)((bytes - (DWORD)offset) / 2);
        wide = (WCHAR *)HeapAlloc(GetProcessHeap(), 0, ((SIZE_T)count + 1) * sizeof(WCHAR));
        if (wide) {
            CopyMemory(wide, buffer + offset, (SIZE_T)count * sizeof(WCHAR));
            wide[count] = 0;
        }
    } else {
        int need = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, (LPCCH)(buffer + offset), (int)(bytes - (DWORD)offset), NULL, 0);
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
                MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, (LPCCH)(buffer + offset), (int)(bytes - (DWORD)offset), wide, need);
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
    LRESULT chars;
    WCHAR *wide;
    GETTEXTEX text_query;
    int bytes_needed;
    char *utf8;
    HANDLE file;
    DWORD to_write;
    DWORD written;
    int ok;

    length_query.flags = GTL_NUMCHARS;
    length_query.codepage = 1200;
    chars = SendMessageW(g_edit, EM_GETTEXTLENGTHEX, (WPARAM)&length_query, 0);
    if (chars < 0) {
        MessageBoxW(hwnd, L"Could not get text length.", APP_NAME, MB_OK | MB_ICONERROR);
        return 0;
    }

    wide = (WCHAR *)HeapAlloc(GetProcessHeap(), 0, ((SIZE_T)chars + 1) * sizeof(WCHAR));
    if (!wide) {
        MessageBoxW(hwnd, L"Out of memory.", APP_NAME, MB_OK | MB_ICONERROR);
        return 0;
    }

    text_query.cb = (DWORD)(((SIZE_T)chars + 1) * sizeof(WCHAR));
    text_query.flags = GT_USECRLF;
    text_query.codepage = 1200;
    text_query.lpDefaultChar = NULL;
    text_query.lpUsedDefChar = NULL;
    SendMessageW(g_edit, EM_GETTEXTEX, (WPARAM)&text_query, (LPARAM)wide);

    bytes_needed = WideCharToMultiByte(CP_UTF8, 0, wide, -1, NULL, 0, NULL, NULL);
    if (bytes_needed <= 0) {
        HeapFree(GetProcessHeap(), 0, wide);
        MessageBoxW(hwnd, L"Could not encode text as UTF-8.", APP_NAME, MB_OK | MB_ICONERROR);
        return 0;
    }

    utf8 = (char *)HeapAlloc(GetProcessHeap(), 0, (SIZE_T)bytes_needed);
    if (!utf8) {
        HeapFree(GetProcessHeap(), 0, wide);
        MessageBoxW(hwnd, L"Out of memory.", APP_NAME, MB_OK | MB_ICONERROR);
        return 0;
    }

    WideCharToMultiByte(CP_UTF8, 0, wide, -1, utf8, bytes_needed, NULL, NULL);
    HeapFree(GetProcessHeap(), 0, wide);

    file = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        HeapFree(GetProcessHeap(), 0, utf8);
        die_last_error(hwnd, L"Create file");
        return 0;
    }

    to_write = (DWORD)(bytes_needed - 1);
    written = 0;
    ok = WriteFile(file, utf8, to_write, &written, NULL) && written == to_write;
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
    WCHAR *text;

    if (!ask_save_if_dirty(hwnd)) return 0;

    text = NULL;
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
    update_status();
    return 1;
}

static int do_save(HWND hwnd) {
    if (!g_path[0]) return do_save_as(hwnd);
    if (!write_text_file(hwnd, g_path)) return 0;
    set_dirty(0);
    update_status();
    return 1;
}

static int ask_save_if_dirty(HWND hwnd) {
    int answer;

    if (!g_dirty) return 1;

    answer = MessageBoxW(hwnd, L"Save changes?", APP_NAME, MB_YESNOCANCEL | MB_ICONQUESTION);
    if (answer == IDCANCEL) return 0;
    if (answer == IDYES) return do_save(hwnd);
    return 1;
}

static void run_command(HWND hwnd, int id) {
    switch (id) {
    case IDM_FILE_NEW: do_new(hwnd); return;
    case IDM_FILE_OPEN: do_open(hwnd); return;
    case IDM_FILE_SAVE: do_save(hwnd); return;
    case IDM_FILE_SAVE_AS: do_save_as(hwnd); return;
    case IDM_FILE_EXIT: SendMessageW(hwnd, WM_CLOSE, 0, 0); return;
    case IDM_EDIT_UNDO: SendMessageW(g_edit, EM_UNDO, 0, 0); return;
    case IDM_EDIT_CUT: SendMessageW(g_edit, WM_CUT, 0, 0); return;
    case IDM_EDIT_COPY: SendMessageW(g_edit, WM_COPY, 0, 0); return;
    case IDM_EDIT_PASTE: SendMessageW(g_edit, WM_PASTE, 0, 0); return;
    case IDM_EDIT_SELECT_ALL: SendMessageW(g_edit, EM_SETSEL, 0, -1); return;
    default: return;
    }
}

static int bar_button_at(int x, int y) {
    int i;

    if (y < 8 || y > 40) return 0;

    for (i = 0; i < (int)(sizeof(g_buttons) / sizeof(g_buttons[0])); i++) {
        int left = g_buttons[i].x;
        int right = left + g_buttons[i].w;
        if (x >= left && x < right) return g_buttons[i].id;
    }

    return 0;
}

static void paint_bar(HWND hwnd) {
    PAINTSTRUCT ps;
    HDC dc;
    RECT rc;
    RECT brand;
    RECT path_rc;
    HGDIOBJ old_font;
    int i;
    const WCHAR *name;

    dc = BeginPaint(hwnd, &ps);
    GetClientRect(hwnd, &rc);
    FillRect(dc, &rc, g_brush_back);

    SetBkMode(dc, TRANSPARENT);
    old_font = SelectObject(dc, g_font_ui);

    brand.left = 14;
    brand.top = 11;
    brand.right = 40;
    brand.bottom = 37;
    FillRect(dc, &brand, g_brush_panel);
    FrameRect(dc, &brand, g_brush_panel);

    SetTextColor(dc, RGB(255, 218, 88));
    TextOutW(dc, 22, 15, L"D", 1);

    SetTextColor(dc, COLOR_TEXT);
    TextOutW(dc, 50, 15, L"DBYTEPAD", 8);

    for (i = 0; i < (int)(sizeof(g_buttons) / sizeof(g_buttons[0])); i++) {
        RECT button;
        button.left = g_buttons[i].x;
        button.top = 9;
        button.right = g_buttons[i].x + g_buttons[i].w;
        button.bottom = 39;
        FillRect(dc, &button, g_brush_panel);
        SetTextColor(dc, COLOR_TEXT);
        DrawTextW(dc, g_buttons[i].label, -1, &button, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
    }

    name = g_path[0] ? g_path : L"Untitled buffer";
    path_rc.left = 464;
    path_rc.top = 0;
    path_rc.right = rc.right - 18;
    path_rc.bottom = TOP_BAR_H;
    SetTextColor(dc, g_dirty ? RGB(255, 218, 88) : COLOR_MUTED);
    DrawTextW(dc, name, -1, &path_rc, DT_SINGLELINE | DT_RIGHT | DT_VCENTER | DT_END_ELLIPSIS);

    SelectObject(dc, old_font);
    EndPaint(hwnd, &ps);
}

static void paint_gutter(HWND hwnd) {
    PAINTSTRUCT ps;
    HDC dc;
    RECT rc;
    HGDIOBJ old_font;
    WCHAR lines_text[32];
    int lines;

    dc = BeginPaint(hwnd, &ps);
    GetClientRect(hwnd, &rc);
    FillRect(dc, &rc, g_brush_panel);

    old_font = SelectObject(dc, g_font_ui);
    SetBkMode(dc, TRANSPARENT);

    SetTextColor(dc, COLOR_HOT);
    TextOutW(dc, 13, 14, L"DBP", 3);

    lines = (int)SendMessageW(g_edit, EM_GETLINECOUNT, 0, 0);
    swprintf(lines_text, 32, L"%d", lines);

    SetTextColor(dc, COLOR_MUTED);
    TextOutW(dc, 12, 48, L"LINES", 5);
    SetTextColor(dc, COLOR_TEXT);
    TextOutW(dc, 12, 68, lines_text, (int)lstrlenW(lines_text));

    SetTextColor(dc, COLOR_MUTED);
    TextOutW(dc, 12, 104, L"MODE", 4);
    SetTextColor(dc, COLOR_TEXT);
    TextOutW(dc, 12, 124, L"TEXT", 4);

    SelectObject(dc, old_font);
    EndPaint(hwnd, &ps);
}

static void paint_status(HWND hwnd) {
    PAINTSTRUCT ps;
    HDC dc;
    RECT rc;
    HGDIOBJ old_font;
    const WCHAR *right_text;

    dc = BeginPaint(hwnd, &ps);
    GetClientRect(hwnd, &rc);
    FillRect(dc, &rc, g_brush_back);

    old_font = SelectObject(dc, g_font_ui);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, COLOR_MUTED);
    TextOutW(dc, 14, 7, g_status_text, (int)lstrlenW(g_status_text));

    right_text = L"native win32";
    SetTextColor(dc, COLOR_TEXT);
    TextOutW(dc, rc.right - 122, 7, right_text, (int)lstrlenW(right_text));

    SelectObject(dc, old_font);
    EndPaint(hwnd, &ps);
}

static LRESULT CALLBACK bar_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    int x;
    int y;
    int id;

    switch (msg) {
    case WM_PAINT:
        paint_bar(hwnd);
        return 0;

    case WM_LBUTTONUP:
        x = (short)LOWORD(lparam);
        y = (short)HIWORD(lparam);
        id = bar_button_at(x, y);
        if (id) run_command(g_main, id);
        return 0;

    case WM_SETCURSOR:
        SetCursor(LoadCursorW(NULL, IDC_HAND));
        return TRUE;
    }

    (void)wparam;
    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

static LRESULT CALLBACK gutter_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
    case WM_PAINT:
        paint_gutter(hwnd);
        return 0;
    }

    (void)wparam;
    (void)lparam;
    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

static LRESULT CALLBACK status_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
    case WM_PAINT:
        paint_status(hwnd);
        return 0;
    }

    (void)wparam;
    (void)lparam;
    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
    case WM_CREATE:
        g_bar = CreateWindowExW(0, CLASS_BAR, L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd, (HMENU)(INT_PTR)ID_BAR, g_instance, NULL);
        g_gutter = CreateWindowExW(0, CLASS_GUTTER, L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd, (HMENU)(INT_PTR)ID_GUTTER, g_instance, NULL);
        g_edit = CreateWindowExW(
            0,
            EDIT_CLASS,
            L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_WANTRETURN | ES_NOHIDESEL | ES_AUTOVSCROLL,
            0, 0, 0, 0,
            hwnd,
            (HMENU)(INT_PTR)ID_EDIT,
            g_instance,
            NULL);
        g_status = CreateWindowExW(0, CLASS_STATUS, L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd, (HMENU)(INT_PTR)ID_STATUS, g_instance, NULL);

        if (!g_bar || !g_gutter || !g_edit || !g_status) return -1;

        SendMessageW(g_edit, WM_SETFONT, (WPARAM)g_font_mono, TRUE);
        SendMessageW(g_edit, EM_SETEVENTMASK, 0, ENM_CHANGE | ENM_SELCHANGE);
        set_edit_face();
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

        run_command(hwnd, LOWORD(wparam));
        return 0;

    case WM_NOTIFY:
        if (((NMHDR *)lparam)->hwndFrom == g_edit && ((NMHDR *)lparam)->code == EN_SELCHANGE) {
            update_status();
            return 0;
        }
        break;

    case WM_CTLCOLOREDIT:
        SetTextColor((HDC)wparam, COLOR_EDIT_TEXT);
        SetBkColor((HDC)wparam, COLOR_EDIT_BACK);
        return (LRESULT)g_brush_edit;

    case WM_CLOSE:
        if (ask_save_if_dirty(hwnd)) DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

static int register_class(const WCHAR *name, WNDPROC proc, HBRUSH brush, HCURSOR cursor) {
    WNDCLASSW wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc = proc;
    wc.hInstance = g_instance;
    wc.hCursor = cursor;
    wc.hIcon = LoadIconW(NULL, IDI_APPLICATION);
    wc.hbrBackground = brush;
    wc.lpszClassName = name;

    return RegisterClassW(&wc) != 0;
}

static int register_window_classes(void) {
    if (!register_class(CLASS_MAIN, window_proc, g_brush_back, LoadCursorW(NULL, IDC_ARROW))) return 0;
    if (!register_class(CLASS_BAR, bar_proc, g_brush_back, LoadCursorW(NULL, IDC_HAND))) return 0;
    if (!register_class(CLASS_GUTTER, gutter_proc, g_brush_panel, LoadCursorW(NULL, IDC_ARROW))) return 0;
    if (!register_class(CLASS_STATUS, status_proc, g_brush_back, LoadCursorW(NULL, IDC_ARROW))) return 0;
    return 1;
}

static void create_gdi_objects(void) {
    g_brush_back = CreateSolidBrush(COLOR_BACK);
    g_brush_panel = CreateSolidBrush(COLOR_PANEL);
    g_brush_edit = CreateSolidBrush(COLOR_EDIT_BACK);

    g_font_ui = CreateFontW(-14, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    g_font_mono = CreateFontW(-17, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FIXED_PITCH, L"Cascadia Mono");
}

static void destroy_gdi_objects(void) {
    if (g_font_ui) DeleteObject(g_font_ui);
    if (g_font_mono) DeleteObject(g_font_mono);
    if (g_brush_back) DeleteObject(g_brush_back);
    if (g_brush_panel) DeleteObject(g_brush_panel);
    if (g_brush_edit) DeleteObject(g_brush_edit);
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previous, PWSTR command_line, int show) {
    HACCEL accel;
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
    MSG msg;

    (void)previous;
    (void)command_line;

    g_instance = instance;
    g_status_text[0] = 0;

    if (!LoadLibraryW(L"Msftedit.dll")) {
        MessageBoxW(NULL, L"Could not load Msftedit.dll.", APP_NAME, MB_OK | MB_ICONERROR);
        return 1;
    }

    create_gdi_objects();

    if (!register_window_classes()) {
        die_last_error(NULL, L"Register window class");
        destroy_gdi_objects();
        return 1;
    }

    g_main = CreateWindowExW(
        0,
        CLASS_MAIN,
        APP_NAME,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        980,
        680,
        NULL,
        NULL,
        instance,
        NULL);

    if (!g_main) {
        die_last_error(NULL, L"Create main window");
        destroy_gdi_objects();
        return 1;
    }

    accel = CreateAcceleratorTableW(accels, (int)(sizeof(accels) / sizeof(accels[0])));

    ShowWindow(g_main, show);
    UpdateWindow(g_main);

    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        if (!accel || !TranslateAcceleratorW(g_main, accel, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    if (accel) DestroyAcceleratorTable(accel);
    destroy_gdi_objects();
    return (int)msg.wParam;
}
