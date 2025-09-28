#include <stdio.h>
#include <string.h>
#include <pspkernel.h>
#include <psputility.h>
#include <pspdisplay.h>
#include <psprtc.h>
#include <psppower.h>

#include <ark.h>
#include <cfwmacros.h>
#include <vshctrl.h>
#include <kubridge.h>
#include <systemctrl.h>

#include "main.h"
#include "utils.h"

extern int psp_model;
extern ARKConfig ark_config;

struct {
    SceCtrlData ctrl_pad;
    u32 cur_buttons;
    u32 button_on;
    int stop_flag;
    int menu_mode;
    int is_registered;
    int cur_idx;
    int show_info;
} vshmenu;

char* menu_opts[] = {
    "Information",
    "Soft Reset",
    "Hard Reset",
    "Suspend",
    "Shutdown",
    "Exit",
};

enum {
    SHOW_INFORMATION,
    SOFT_RESET,
    HARD_RESET,
    SUSPEND_DEVICE,
    SHUTDOWN_DEVICE,
    EXIT_MENU
};

char info_string[128];

int (*scePafAddClockOrig)(ScePspDateTime*, wchar_t*, int, wchar_t*) = NULL;

int EatKey(SceCtrlData *pad_data, int count)
{
    u32 buttons;

    // copy true value
    memcpy(&vshmenu.ctrl_pad, pad_data, sizeof(SceCtrlData));

    // buttons check
    buttons             = vshmenu.ctrl_pad.Buttons;
    vshmenu.button_on   = ~vshmenu.cur_buttons & buttons;
    vshmenu.cur_buttons = buttons;

    // mask buttons for LOCK VSH control
    for (int i=0; i<count; i++) {
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
    if (vshmenu.is_registered){
        if (vshmenu.menu_mode == 2){
            return utf8_to_unicode(str, "Bye!");
        }
        else if (vshmenu.show_info){
            return utf8_to_unicode(str, info_string);
        }
        else {
            char* tmp = sce_paf_private_malloc(512);
            sce_paf_private_sprintf(tmp, "\n\n\n\n\n\n\n %s%s\n %s%s\n %s%s\n %s%s\n %s%s\n %s%s\n",
                (vshmenu.cur_idx==0)? STAR:" ", menu_opts[0],
                (vshmenu.cur_idx==1)? STAR:" ", menu_opts[1],
                (vshmenu.cur_idx==2)? STAR:" ", menu_opts[2],
                (vshmenu.cur_idx==3)? STAR:" ", menu_opts[3],
                (vshmenu.cur_idx==4)? STAR:" ", menu_opts[4],
                (vshmenu.cur_idx==5)? STAR:" ", menu_opts[5]

            );
            int res = utf8_to_unicode(str, tmp);
            sce_paf_private_free(tmp);
            return res;
        }
    }
    else {
        return scePafAddClockOrig(time, str, max_len, format);
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

    sce_paf_private_sprintf(info_string, "\n\n\n"
        "CFW: ARK %d.%d.%d r%d\n"
        "Built: %s %s\n"
        "Console: %s FW%d%d%d",
        ARK_MAJOR_VERSION, ARK_MINOR_VERSION,
        ARK_MICRO_VERSION, ARK_REVISION,
        __DATE__, __TIME__,
        console_type, major, minor, micro
    );

    scePafAddClockOrig = (void*)U_EXTRACT_CALL(addr + 4);

    _sh((u16)-4, addr - 0x48);
    MAKE_CALL(addr + 4, (u32)&scePafAddClockPatched);

    sceKernelDcacheWritebackAll();
    kuKernelIcacheInvalidateAll();

}

int menu_ctrl(u32 button_on)
{
    if ((button_on & PSP_CTRL_SELECT) || (button_on & PSP_CTRL_HOME)) {
        return 1;
    }
    if (!vshmenu.show_info){
        if (button_on & PSP_CTRL_DOWN) {
            if (vshmenu.cur_idx < NELEMS(menu_opts)-1){
                vshmenu.cur_idx++;
            }
        }
        else if (button_on & PSP_CTRL_UP) {
            if (vshmenu.cur_idx > 0){
                vshmenu.cur_idx--;
            }
        }
        else if ((button_on & PSP_CTRL_CROSS) || (button_on & PSP_CTRL_CIRCLE)) {
            switch (vshmenu.cur_idx){
                case SHOW_INFORMATION:   vshmenu.show_info = 1;    break;
                case SOFT_RESET:         sctrlKernelExitVSH(NULL); return 1;
                case HARD_RESET:         scePowerRequestSuspend(); return 1;
                case SUSPEND_DEVICE:     scePowerRequestSuspend(); return 1;
                case SHUTDOWN_DEVICE:    scePowerRequestStandby(); return 1;
                case EXIT_MENU:                                    return 1;
            }
        }
    }
    else if ((button_on & PSP_CTRL_CROSS) || (button_on & PSP_CTRL_CIRCLE)) {
        vshmenu.show_info = 0;
    }

    return 0; // continue
}

static void button_func(void)
{
    // menu controll
    switch (vshmenu.menu_mode) {
        case 0:    
            if ((vshmenu.cur_buttons & ALL_CTRL) == 0) {
                vshmenu.menu_mode = 1;
            }
            break;
        case 1:
            if (menu_ctrl(vshmenu.button_on))
				vshmenu.menu_mode = 2;
            break;
		case 2:
			if ((vshmenu.cur_buttons & ALL_CTRL) == 0)
				vshmenu.stop_flag = 1;
			break;
    }
}

int TSRThread(SceSize args, void *argp)
{

    sceKernelChangeThreadPriority(0, 8);
    vctrlVSHRegisterVshMenu(EatKey);

    vshmenu.is_registered = 1;
    while (!vshmenu.stop_flag) {
        if( sceDisplayWaitVblankStart() < 0)
            break; // end of VSH ?

        button_func();
    }

    vshmenu.menu_mode = 2;
    sceKernelDelayThread(1000000); // let "Bye" message be seen for a second
    vshmenu.is_registered = 0;

	vctrlVSHExitVSHMenu(NULL, NULL, 0);
    return sceKernelExitDeleteThread(0);
}

int xmbctrlEnterVshMenuMode(){

    memset(&vshmenu, 0, sizeof(vshmenu));
    vshmenu.cur_buttons = 0xFFFFFFFF;

    SceUID thread_id = sceKernelCreateThread("VshMenu_Thread", (void*)KERNELIFY(TSRThread), 16 , 0x1000 , 0 , 0);
    return sceKernelStartThread(thread_id, 0, 0);
}
