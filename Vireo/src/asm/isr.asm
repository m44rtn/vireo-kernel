bits 32

global isr0
global isr1
global isr3
global isr4
global isr7
global isr8
global isr9
global isr11
global isr13
global isr14
global isr20
global isr21
global isr30
global isr47
global isr2e
global isr2F

extern isr0c
extern isr1c
extern isr3c
extern isr4c
extern isr7c
extern isr8c
extern isr11c
extern isr12c
extern isr13c
extern isr14c
extern isr20c
extern isr21c
extern isr30c
extern isr47c
extern main
extern sleep
extern print
extern counter


isr0:
pushad
cld
call isr0c
push 300
call sleep
call main
iretd

isr1:
pushad
cld
call isr1c
popad
iretd

isr3:
pushad

push eax

cld
call isr3c 
popad
iretd

isr4:
popad
;pushad
cld
call isr4c
;popad
iretd

isr7:
pushad
cld
call isr7c
popad
iretd

isr8:
pushad
cld
call isr8c
popad
iretd

isr9:
hlt

isr11:
pushad
cld 
call isr11c
popad
iretd

isr12:
;hlt
popad
cld
call isr12c
iret

isr13:
;pushad

;mov eax, [esp + 36] ;32 ip
;mov ecx, [esp + 40] ;cs
;mov ebx, [esp + 52] ;esp/ss
;mov edx, [esp + 56] ;ss/esp

pop eax
pop ecx
;pop ebx ;eflags
;pop ebx
;pop edx

;push ebx
;push edx

push ecx
push eax

;push ebx
;push edx

;push ecx
;push eax

;cld
call isr13c
popad
iretd

isr14:
pushad
cld
call isr13c
popad
iretd
hlt


extern Task_Save_State

isr20:
;   PIT
pushad
call Task_Save_State

call isr20c
popad
iretd

isr21:
pushad
cld
call isr21c
popad
iretd

isr30:
pushad
cld
call isr30c
popad
iretd

isr47:
pushad
cld
call isr47c
popad
iretd
