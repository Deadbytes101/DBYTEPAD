; DBYTEPAD-1K
;
; Clean-room tiny Win32 edit surface.
; Goal: keep this file simple enough to inspect while the build system fights bytes.
;
; Build from an x86 Native Tools Command Prompt:
;   tiny\build.bat
;
; This source deliberately avoids windows.inc, CRT, resources, RichEdit, menus,
; status bars, dialogs, config, and the DByte bridge.

.386
.model flat, stdcall
option casemap:none

extrn CreateWindowExA:proc
extrn DispatchMessageA:proc
extrn ExitProcess:proc
extrn GetMessageA:proc
extrn GetModuleHandleA:proc
extrn TranslateMessage:proc

.data
editClass db 'EDIT',0
titleText db 'DBYTEPAD-1K',0
msg dd 7 dup(0)

.code
start:
    push 0
    call GetModuleHandleA

    ; Create a top-level EDIT control. USER32 supplies the edit behavior.
    ; Style bits:
    ;   WS_VISIBLE          10000000h
    ;   WS_OVERLAPPEDWINDOW 00CF0000h
    ;   WS_VSCROLL          00200000h
    ;   WS_HSCROLL          00100000h
    ;   ES_MULTILINE        00000004h
    ;   ES_AUTOVSCROLL      00000040h
    ;   ES_AUTOHSCROLL      00000080h
    ;   ES_WANTRETURN       00001000h
    push 0
    push eax
    push 0
    push 0
    push 480
    push 640
    push 80000000h
    push 80000000h
    push 10FF10C4h
    push offset titleText
    push offset editClass
    push 0
    call CreateWindowExA

message_loop:
    push 0
    push 0
    push 0
    push offset msg
    call GetMessageA
    test eax, eax
    jz exit_process

    push offset msg
    call TranslateMessage

    push offset msg
    call DispatchMessageA
    jmp message_loop

exit_process:
    push 0
    call ExitProcess

end start
