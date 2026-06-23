#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define APP_NAME "DBYTEPAD Micro"
#define EDIT_ID 1001

static HWND g_main;
static HWND g_edit;
static char g_path[MAX_PATH];
static int g_dirty;

static void set_title(void) {
    char title[MAX_PATH + 32];
    const char *name = g_path[0] ? g_path : "Untitled";
    wsprintfA(title, "%s%s - DBMICRO", g_dirty ? "*" : "", name);
    SetWindowTextA(g_main, title);
}

static int read_whole_file(const char *path, char **out, DWORD *out_len) {
    HANDLE f;
    DWORD size;
    DWORD read;
    char *buf;

    *out = 0;
    *out_len = 0;

    f = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (f == INVALID_HANDLE_VALUE) return 0;

    size = GetFileSize(f, 0);
    if (size == INVALID_FILE_SIZE || size > 1024 * 1024) {
        CloseHandle(f);
        return 0;
    }

    buf = (char *)GlobalAlloc(GMEM_FIXED, size + 1);
    if (!buf) {
        CloseHandle(f);
        return 0;
    }

    if (!ReadFile(f, buf, size, &read, 0)) {
        GlobalFree(buf);
        CloseHandle(f);
        return 0;
    }

    CloseHandle(f);
    buf[read] = 0;
    *out = buf;
    *out_len = read;
    return 1;
}

static int write_whole_file(const char *path, const char *buf, DWORD len) {
    HANDLE f;
    DWORD written;
    int ok;

    f = CreateFileA(path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (f == INVALID_HANDLE_VALUE) return 0;

    ok = WriteFile(f, buf, len, &written, 0) && written == len;
    CloseHandle(f);
    return ok;
}

static void open_path(const char *path) {
    char *buf;
    DWORD len;

    if (!read_whole_file(path, &buf, &len)) {
        MessageBoxA(g_main, "Open failed.", APP_NAME, MB_OK | MB_ICONERROR);
        return;
    }

    lstrcpynA(g_path, path, MAX_PATH);
    SetWindowTextA(g_edit, buf);
    GlobalFree(buf);
    g_dirty = 0;
    set_title();
}

static void save_back(void) {
    int len;
    char *buf;

    if (!g_path[0]) {
        MessageBoxA(g_main, "No input path. Start with: dbpadmicro.exe file.txt", APP_NAME, MB_OK | MB_ICONINFORMATION);
        return;
    }

    len = GetWindowTextLengthA(g_edit);
    buf = (char *)GlobalAlloc(GMEM_FIXED, (DWORD)len + 1);
    if (!buf) return;

    GetWindowTextA(g_edit, buf, len + 1);
    if (!write_whole_file(g_path, buf, (DWORD)len)) {
        MessageBoxA(g_main, "Save failed.", APP_NAME, MB_OK | MB_ICONERROR);
    } else {
        g_dirty = 0;
        set_title();
    }

    GlobalFree(buf);
}

static void check_save_hotkey(void) {
    if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) && (GetAsyncKeyState('S') & 1)) {
        save_back();
    }
}

static void parse_first_arg(void) {
    char *cmd = GetCommandLineA();
    char *out = g_path;
    int quoted;

    quoted = *cmd == '"';
    if (quoted) {
        cmd++;
        while (*cmd && *cmd != '"') cmd++;
        if (*cmd == '"') cmd++;
    } else {
        while (*cmd && *cmd != ' ' && *cmd != '\t') cmd++;
    }

    while (*cmd == ' ' || *cmd == '\t') cmd++;
    if (!*cmd) return;

    quoted = *cmd == '"';
    if (quoted) cmd++;

    while (*cmd && out < g_path + MAX_PATH - 1) {
        if (quoted) {
            if (*cmd == '"') break;
        } else if (*cmd == ' ' || *cmd == '\t') {
            break;
        }
        *out++ = *cmd++;
    }
    *out = 0;
}

static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE:
        g_edit = CreateWindowExA(0, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_WANTRETURN, 0, 0, 0, 0, hwnd, (HMENU)EDIT_ID, GetModuleHandleA(0), 0);
        SendMessageA(g_edit, EM_LIMITTEXT, 0, 0);
        if (g_path[0]) open_path(g_path);
        set_title();
        return 0;

    case WM_SIZE:
        MoveWindow(g_edit, 0, 0, LOWORD(lp), HIWORD(lp), TRUE);
        return 0;

    case WM_COMMAND:
        if (LOWORD(wp) == EDIT_ID && HIWORD(wp) == EN_CHANGE) {
            g_dirty = 1;
            set_title();
        }
        return 0;

    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmdline, int show) {
    WNDCLASSA wc;
    MSG msg;

    (void)prev;
    (void)cmdline;
    (void)show;

    parse_first_arg();

    wc.style = 0;
    wc.lpfnWndProc = wnd_proc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = inst;
    wc.hIcon = 0;
    wc.hCursor = LoadCursorA(0, IDC_IBEAM);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = 0;
    wc.lpszClassName = "DBMICROCLS";
    RegisterClassA(&wc);

    g_main = CreateWindowExA(0, "DBMICROCLS", "DBMICRO", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, 0, 0, inst, 0);
    if (!g_main) return 1;

    while (GetMessageA(&msg, 0, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
        check_save_hotkey();
    }

    return 0;
}
