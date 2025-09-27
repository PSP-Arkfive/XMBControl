#include <stdio.h>
#include <string.h>
#include <pspkernel.h>
#include <psputility.h>
#include <pspdisplay.h>
#include <psprtc.h>

#include <cfwmacros.h>
#include <vshctrl.h>
#include <kubridge.h>
#include <systemctrl_se.h>

#include "main.h"

u16 backup1 = 0;
u32 backup2 = 0;
u32 patch_addr = 0;

int vshmenu_registered = 0;

int scePafAddClockPatched(ScePspDateTime* time, wchar_t* str, int max_len, wchar_t* format) {
    if (vshmenu_registered)
        return sce_paf_private_wcscpy(str, L"ARK-5");
    else {
        int (*orig)(ScePspDateTime*, wchar_t*, int, wchar_t*) = (void*)backup2;
        return orig(time, str, max_len, format);
    }
}

void patchVshClock(u32 addr){

    backup1 = _lh(addr - 0x48);
    backup2 = U_EXTRACT_CALL(addr + 4);
    patch_addr = addr;

    _sh(0, addr - 0x48);
    MAKE_CALL(addr + 4, (u32)&scePafAddClockPatched);

    sceKernelDcacheWritebackAll();
    kuKernelIcacheInvalidateAll();

}

int xmbctrlEnterVshMenuMode(){
    vshmenu_registered = 1;
    return 0;
}

int xmbctrlExitVshMenuMode(){
    vshmenu_registered = 0;
    return 0;
}
