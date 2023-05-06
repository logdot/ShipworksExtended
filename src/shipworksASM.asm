.data
; External functions
New             QWORD 140334094h
CreateLibraryEx QWORD 14027f200h
MakeGlobalEx    QWORD 140194de0h
CopyStringEx    QWORD 1400056e0h
DefinePartEx    QWORD 14021b190h

mdlEngine BYTE "MDL_ENGINE_10", 00

.code

public DefinePart
DefinePart PROC
    sub rsp, 72 ; Need to assure that the stack is 32 bit's aligned otherwise a movaps crashes the game.
                ; Additionally need enough stack space for DefinePartEx to encroach on
    mov r9d, 1
    mov r8, [r8]
    mov rcx, [rcx]
    call DefinePartEx
    mov dword ptr [rax+7a8h], 3E7h
    add rsp, 72
    ret
DefinePart ENDP 

; __fast_call
; void CreateLibrary(Library library, char* libraryName)
; RDX = libraryName
; RCX = library
public CreateLibrary
CreateLibrary PROC
    push rbx
    mov rbx, [rcx]
    test rbx, rbx
    jmp noDeconstruct
    jz noDeconstruct
    mov rax, [rbx]
    ;; This calls the function at 14025e320, which reads the vftable at rcx
    ;; (which normally is just the vftable of the object that called it) and
    ;; calls the function at 14025E330, which I'm not sure what it does. When
    ;; using the modloader rcx is a garbage value, causing it to try calling
    ;; into random memory. For now I'm just skipping calling this function, as
    ;; it doesn't seem to affect the game at all.
    call qword ptr [rax+48h]
noDeconstruct:
    ;; Save our parameters
    push rdx
    push rcx
    mov rcx, 128h
    call New
    test rax, rax
    jz newFail
    mov rcx, rax
    sub rsp, 40
    call CreateLibraryEx
    add rsp, 40
    jmp newSuccess
newFail:
    xor rbx, rbx
    mov rax, rbx
newSuccess:
    mov rdx, rax
    pop rbx ; Load our first parameter into rbx
    lea rcx, [rbx-8]
    sub rsp, 40
    call MakeGlobalEx
    add rsp, 40
    mov rcx, [rbx]
    add rcx, 0c8h
    mov r8d, 13h
    pop rdx
    sub rsp, 40
    call CopyStringEx
    add rsp, 40
    pop rbx
    ret
CreateLibrary ENDP
END