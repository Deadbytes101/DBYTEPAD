; DBYTEPAD-1K
;
; Clean-room tiny Win32 edit surface.
; Goal: beat TinyRetroPad on measured bytes without copying its source.
;
; Build from an x86 Native Tools Command Prompt:
;   tiny\build.bat
;
; This source deliberately avoids windows.inc, CRT, resources, RichEdit, menus,
; status bars, dialogs, config, and the DByte bridge.

.386
.model flat, stdcall
option casemap:none

EXTERN _imp__CreateWindowExA@48 :PTR
EXTERN _imp__DispatchMessageA@4 :PTR
EXTERN _imp__ExitProcess@4     :PTR
EXTERN _imp__GetMessageA@16    :PTR
EXTERN _imp__GetModuleHandleA@4:PTR
EXTERN _imp__IsWindow@4        :PTR
EXTERN _imp__TranslateMessage@4:PTR

.DATA
editClass db 'EDIT',0
titleText db 'DB1K',0
msg dd 7 dup(0)

.CODE
start:
    xor ebx, ebx

    push ebx
    call dword ptr [_imp__GetModuleHandleA@4]

    ; Top-level EDIT control. USER32 supplies the editing surface.
    ; If the user closes it, the default window proc destroys the window.
    ; The loop below detects that with IsWindow and exits the process.
    push ebx
    push eax
    push ebx
    push ebx
    push 480
    push 640
    push 80000000h
    push 80000000h
    push 10FF10C4h
    push offset titleText
    push offset editClass
    push ebx
    call dword ptr [_imp__CreateWindowExA@48]
    mov esi, eax

message_loop:
    push ebx
    push ebx
    push ebx
    push offset msg
    call dword ptr [_imp__GetMessageA@16]
    test eax, eax
    jz exit_process

    push offset msg
    call dword ptr [_imp__TranslateMessage@4]

    push offset msg
    call dword ptr [_imp__DispatchMessageA@4]

    push esi
    call dword ptr [_imp__IsWindow@4]
    test eax, eax
    jnz message_loop

exit_process:
    push ebx
    call dword ptr [_imp__ExitProcess@4]

end start
