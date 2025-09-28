#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <pspsdk.h>
#include <pspkernel.h>
#include <psputility_sysparam.h>

#include <ark.h>
#include <cfwmacros.h>
#include <kubridge.h>
#include <vshctrl.h>
#include <systemctrl.h>
#include <systemctrl_se.h>

#include "main.h"
#include "utils.h"
#include "list.h"
#include "settings.h"
#include "plugins.h"

PSP_MODULE_INFO("XmbControl", 0x0007, 1, 5);

extern int psp_model;
extern ARKConfig ark_config;
extern SEConfig se_config;
extern STMOD_HANDLER previous;

extern void findAllTranslatableStrings();
extern int OnModuleStart(SceModule *mod);

int module_start(SceSize args, void *argp)
{

    psp_model = kuKernelGetModel();

    sctrlSEGetConfig(&se_config);

    sctrlArkGetConfig(&ark_config);

    findAllTranslatableStrings();
    
    previous = sctrlHENSetStartModuleHandler(OnModuleStart);

    return 0;
}
