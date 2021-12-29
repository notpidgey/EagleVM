#include "EagleVMStub.h"
#include <cstdio>

/*
.text:0000000140011860 sub_140011860   proc near               ; CODE XREF: sub_140011262↑j
.text:0000000140011860                                         ; DATA XREF: .pdata:000000014001E818↓o
.text:0000000140011860
.text:0000000140011860 var_10C         = dword ptr -10Ch
.text:0000000140011860 var_EC          = dword ptr -0ECh
.text:0000000140011860 arg_0           = dword ptr  10h
.text:0000000140011860 arg_8           = qword ptr  18h
.text:0000000140011860
.text:0000000140011860                 mov     [rsp-8+arg_8], rdx
.text:0000000140011865                 mov     [rsp-8+arg_0], ecx
.text:0000000140011869                 push    rbp
.text:000000014001186A                 push    rdi
.text:000000014001186B                 sub     rsp, 128h
.text:0000000140011872                 lea     rbp, [rsp+20h]
.text:0000000140011877                 lea     rcx, unk_140022003
.text:000000014001187E                 call    sub_140011361
.text:0000000140011883                 call    cs:?fnEagleVMBegin@@YAXXZ ; fnEagleVMBegin(void)
.text:0000000140011889                 mov     [rbp+110h+var_10C], 5
.text:0000000140011890                 mov     eax, [rbp+110h+var_10C]
.text:0000000140011893                 inc     eax
.text:0000000140011895                 mov     [rbp+110h+var_10C], eax
.text:0000000140011898                 mov     eax, [rbp+110h+var_10C]
.text:000000014001189B                 dec     eax
.text:000000014001189D                 mov     [rbp+110h+var_10C], eax
.text:00000001400118A0                 mov     [rbp+110h+var_EC], 0
.text:00000001400118A7                 mov     eax, [rbp+110h+var_10C]
.text:00000001400118AA                 mov     ecx, [rbp+110h+var_EC]
.text:00000001400118AD                 add     ecx, eax
.text:00000001400118AF                 mov     eax, ecx
.text:00000001400118B1                 mov     [rbp+110h+var_EC], eax
.text:00000001400118B4                 mov     r8d, [rbp+110h+var_EC]
.text:00000001400118B8                 mov     edx, [rbp+110h+var_10C]
.text:00000001400118BB                 lea     rcx, aTestII    ; "test %i %i"
.text:00000001400118C2                 call    sub_140011190
.text:00000001400118C7                 call    cs:?fnEagleVMEnd@@YAXXZ ; fnEagleVMEnd(void)
.text:00000001400118CD                 xor     eax, eax
.text:00000001400118CF                 lea     rsp, [rbp+108h]
.text:00000001400118D6                 pop     rdi
.text:00000001400118D7                 pop     rbp
.text:00000001400118D8                 retn
.text:00000001400118D8 sub_140011860   endp
*/

int main(int argc, char* argv[])
{
    fnEagleVMBegin();

    int i = 5;
    i++;
    i--;

    int x = 0;
    x += i;

    printf("test %i %i", i, x);

    fnEagleVMEnd();
    return 0;
}
