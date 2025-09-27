#include <stdio.h>
#include <string.h>
#include <pspkernel.h>
#include <psputility.h>
#include <pspdisplay.h>
#include <psprtc.h>

#include <ark.h>
#include <cfwmacros.h>
#include <vshctrl.h>
#include <kubridge.h>
#include <systemctrl.h>

#include "main.h"
#include "utils.h"

extern int psp_model;
extern ARKConfig ark_config;

u16 backup1 = 0;
u32 backup2 = 0;
u32 patch_addr = 0;

SceCtrlData ctrl_pad;
u32 cur_buttons = 0xFFFFFFFF;
u32 button_on  = 0;
int stop_flag = 0;
int menu_mode  = 0;
int vshmenu_registered = 0;

wchar_t full_version[256];

int EatKey(SceCtrlData *pad_data, int count)
{
    u32 buttons;
    int i;

    // copy true value
    memcpy(&ctrl_pad, pad_data, sizeof(SceCtrlData));

    // buttons check
    buttons     = ctrl_pad.Buttons;
    button_on   = ~cur_buttons & buttons;
    cur_buttons = buttons;

    // mask buttons for LOCK VSH control
    for(i=0;i < count;i++) {
        pad_data[i].Buttons &= ~(
                PSP_CTRL_SELECT|PSP_CTRL_START|
                PSP_CTRL_UP|PSP_CTRL_RIGHT|PSP_CTRL_DOWN|PSP_CTRL_LEFT|
                PSP_CTRL_LTRIGGER|PSP_CTRL_RTRIGGER|
                PSP_CTRL_TRIANGLE|PSP_CTRL_CIRCLE|PSP_CTRL_CROSS|PSP_CTRL_SQUARE|
                PSP_CTRL_HOME|PSP_CTRL_NOTE);

    }

    return 0;
}

int scePafAddClockPatched(ScePspDateTime* time, wchar_t* str, int max_len, wchar_t* format) {
    if (vshmenu_registered){
        return sce_paf_private_wcscpy(str, full_version);
    }
    else {
        int (*orig)(ScePspDateTime*, wchar_t*, int, wchar_t*) = (void*)backup2;
        return orig(time, str, max_len, format);
    }
}

void patchVshClock(u32 addr){

    u32 fw = sceKernelDevkitVersion();
    u32 major = fw>>24;
    u32 minor = (fw>>16)&0xF;
    u32 micro = (fw>>8)&0xF;
    
    char* console_type = "";

    if (IS_VITA((&ark_config))){
        console_type = "PS Vita";
    }
    else {
        switch (sctrlHENIsToolKit()){
            default:
            case 0:
                switch (psp_model) {
                    default:
                    case 0: console_type = "PSP Phat"; break;
                    case 1: console_type = "PSP Lite"; break;
                    case 2: case 3: case 6:
                    case 8: console_type = "PSP Bright"; break;
                    case 4: console_type = "PSP Go"; break;
                    case 10: console_type = "PSP Street"; break;
                    
                }
                break;
            case 1: console_type = "PSP TestingTool"; break;
            case 2: console_type = "PSP DevelopmentTool"; break;
        }
    }

    char tmp[128];
    sce_paf_private_sprintf(tmp, "\n\n\n"
        "CFW: ARK %d.%d.%d r%d\n"
        "Built: %s %s\n"
        "Console: %s FW%d%d%d",
        ARK_MAJOR_VERSION, ARK_MINOR_VERSION,
        ARK_MICRO_VERSION, ARK_REVISION,
        __DATE__, __TIME__,
        console_type, major, minor, micro
    );
    utf8_to_unicode(full_version, tmp);

    backup1 = _lh(addr - 0x48);
    backup2 = U_EXTRACT_CALL(addr + 4);
    patch_addr = addr;

    _sh(0, addr - 0x48);
    MAKE_CALL(addr + 4, (u32)&scePafAddClockPatched);

    sceKernelDcacheWritebackAll();
    kuKernelIcacheInvalidateAll();

}

int menu_ctrl(u32 button_on)
{
    if( (button_on & PSP_CTRL_SELECT) || (button_on & PSP_CTRL_HOME) ) {
        return 1;
    }

    return 0; // continue
}

static void button_func(void)
{
    // menu controll
    switch(menu_mode) {
        case 0:    
            if( (cur_buttons & ALL_CTRL) == 0) {
                menu_mode = 1;
            }
            break;
        case 1:
            if(menu_ctrl(button_on))
				menu_mode = 2;
            break;
		case 2:
			if ((cur_buttons & ALL_CTRL) == 0)
				stop_flag = 1;
			break;
    }
}

int TSRThread(SceSize args, void *argp)
{

    sceKernelChangeThreadPriority(0, 8);
    vctrlVSHRegisterVshMenu(EatKey);

    vshmenu_registered = 1;
    while (!stop_flag) {
        if( sceDisplayWaitVblankStart() < 0)
            break; // end of VSH ?

        button_func();
    }
    vshmenu_registered = 0;

	vctrlVSHExitVSHMenu(NULL, NULL, 0);
    return sceKernelExitDeleteThread(0);
}

int xmbctrlEnterVshMenuMode(){

    memset(&ctrl_pad, 0, sizeof(SceCtrlData));
    cur_buttons = 0xFFFFFFFF;
    button_on  = 0;
    stop_flag = 0;
    menu_mode  = 0;
    vshmenu_registered = 0;

    SceUID thread_id = sceKernelCreateThread("VshMenu_Thread", (void*)KERNELIFY(TSRThread), 16 , 0x1000 , 0 , 0);
    return sceKernelStartThread(thread_id, 0, 0);
}
