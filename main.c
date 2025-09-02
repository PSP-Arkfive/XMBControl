/*
    6.39 TN-A, XmbControl
    Copyright (C) 2011, Total_Noob
    Copyright (C) 2011, Frostegater
    Copyright (C) 2011, codestation

    6.60 ARK, Acid_Snake & krazynez

    main.c: XmbControl main code
    
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stddef.h>
#include <string.h>
#include <pspsdk.h>
#include <pspkernel.h>
#include <psputility_sysparam.h>

#include <ark.h>
#include <cfwmacros.h>
#include <kubridge.h>
#include <systemctrl.h>
#include <systemctrl_se.h>
#include <vshctrl.h>

#include "main.h"
#include "utils.h"
#include "list.h"
#include "settings.h"
#include "plugins.h"

PSP_MODULE_INFO("XmbControl", 0x0007, 1, 5);

int psp_model;
ARKConfig ark_config;
CFWConfig config;
SEConfig se_config;

extern void initXmbPatch();

int module_start(SceSize args, void *argp)
{        
    psp_model = kuKernelGetModel();

    sctrlSEGetConfig(&se_config);

    sctrlArkGetConfig(&ark_config);

    initXmbPatch();

    return 0;
}
