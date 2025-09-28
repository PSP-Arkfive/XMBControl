#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <pspsdk.h>
#include <pspkernel.h>

#include <ark.h>
#include <cfwmacros.h>

#include "list.h"
#include "settings.h"
#include "plugins.h"
#include "main.h"
#include "utils.h"

List plugins;
List iplugins;

extern ARKConfig ark_config;

static char* sample_plugin_path = "ULUS01234, ms0:/SEPLUGINS/example.prx";

int cur_place = 0;

char* plugins_paths[] = {
    "ms0:/SEPLUGINS/",
    "ef0:/SEPLUGINS/",
    ark_config.arkpath
};

static void list_cleaner(void* item){
    Plugin* plugin = (Plugin*)item;
    sce_paf_private_free(plugin->path);
    if (plugin->name) sce_paf_private_free(plugin->name);
    if (plugin->surname) sce_paf_private_free(plugin->surname);
    sce_paf_private_free(plugin);
}

static void processCustomLine(char* line){
    Plugin* plugin = (Plugin*)sce_paf_private_malloc(sizeof(Plugin));
    memset(plugin, 0, sizeof(Plugin));
    plugin->path = line;
    plugin->place = cur_place;
    add_list(&plugins, plugin);
}

static int processPlugin(char* runlevel, char* path, char* enabled){
    int n = plugins.count;
    char* name = sce_paf_private_malloc(20);
    sprintf(name, "plugin_%d", n);

    char* surname = sce_paf_private_malloc(20);
    sprintf(surname, "plugins%d", n);

    int path_len = strlen(runlevel) + strlen(path) + 10;
    char* full_path = (char*)sce_paf_private_malloc(path_len);
    sprintf(full_path, "%s, %s", runlevel, path);

    Plugin* plugin = (Plugin*)sce_paf_private_malloc(sizeof(Plugin));
    plugin->name = name;
    plugin->surname = surname;
    plugin->path = full_path;
    plugin->place = cur_place;
    plugin->active = isRunlevelEnabled(enabled);

    add_list(&plugins, plugin);

    return 1;
}

static int processInstallablePlugin(char* plugin_name, int place){
    int n = iplugins.count;
    char* name = sce_paf_private_malloc(20);
    sprintf(name, "iplugin_%d", n);

    char* surname = sce_paf_private_malloc(20);
    sprintf(surname, "iplugins%d", n);

    char* path = (char*)sce_paf_private_malloc(sce_paf_private_strlen(plugin_name)+1);
    sce_paf_private_strcpy(path, plugin_name);

    Plugin* plugin = (Plugin*)sce_paf_private_malloc(sizeof(Plugin));
    plugin->name = name;
    plugin->surname = surname;
    plugin->path = path;
    plugin->place = place;
    plugin->active = 0;

    add_list(&iplugins, plugin);

    return 1;
}

void loadPlugins(){
    clear_list(&plugins, &list_cleaner);

    char path[ARK_PATH_SIZE];
    strcpy(path, ark_config.arkpath);
    strcat(path, "PLUGINS.TXT");
    

    if (strncasecmp(path+5, "seplugins", 9) != 0){
        cur_place = PLACE_ARK_PATH;
        ProcessConfigFile(path, &processPlugin, &processCustomLine);
    }

    cur_place = PLACE_MS0;
    ProcessConfigFile("ms0:/SEPLUGINS/PLUGINS.TXT", &processPlugin, &processCustomLine);
    
    cur_place = PLACE_EF0;
    ProcessConfigFile("ef0:/SEPLUGINS/PLUGINS.TXT", &processPlugin, &processCustomLine);

    if (plugins.count == 0){
        // Add example plugin
        Plugin* plugin = (Plugin*)sce_paf_private_malloc(sizeof(Plugin));
        plugin->name = (char*)sce_paf_private_malloc(20);
        plugin->surname = (char*)sce_paf_private_malloc(20);
        plugin->path = (char*)sce_paf_private_malloc(strlen(sample_plugin_path)+1);
        plugin->active = 1;
        plugin->place = 0;
        strcpy(plugin->name, "plugin_0");
        strcpy(plugin->surname, "plugins0");
        strcpy(plugin->path, sample_plugin_path);
        add_list(&plugins, plugin);
    }
}

void savePlugins(){

    if (plugins.count == 0) return;

    if (plugins.count == 1){
        Plugin* plugin = (Plugin*)(plugins.table[0]);
        if (strcmp(plugin->path, sample_plugin_path) == 0){
            return;
        }
    }

    char path[ARK_PATH_SIZE];
    strcpy(path, ark_config.arkpath);
    strcat(path, "PLUGINS.TXT");

    int fd[] = {
        sceIoOpen("ms0:/SEPLUGINS/PLUGINS.TXT", PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777),
        sceIoOpen("ef0:/SEPLUGINS/PLUGINS.TXT", PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777),
        sceIoOpen(path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777)
    };

    for (int i=0; i<plugins.count; i++){
        Plugin* plugin = (Plugin*)(plugins.table[i]);
        if (plugin->active == PLUGIN_REMOVED) continue;
        sceIoWrite(fd[plugin->place], plugin->path, strlen(plugin->path));
        if (plugin->name != NULL){
            char* sep = ", ";
            char* enabled = (plugin->active)? "on" : "off";
            sceIoWrite(fd[plugin->place], sep, strlen(sep));
            sceIoWrite(fd[plugin->place], enabled, strlen(enabled));
        }
        sceIoWrite(fd[plugin->place], "\n", 1);
    }

    sceIoClose(fd[0]);
    sceIoClose(fd[1]);
    sceIoClose(fd[2]);
}

int isPluginInstalled(char* plugin_path, char* plugin_name){
    char file1[128]; char file2[128];
    for (int i=0; i<plugins.count; i++){
        Plugin* plugin = (Plugin*)plugins.table[i];
        if (
                strcasecmp(getPluginName(plugin_name, file1), getPluginName(plugin->path, file2)) == 0 && // same name
                strcasecmp(plugin_path, plugins_paths[plugin->place]) == 0) // same path
        {
            return 1;
        }
    }
    return 0;
}

static void findInstallablePluginsSubfolder(int place, char* subfolder){
    char fullpath[128];
    sce_paf_private_strcpy(fullpath, plugins_paths[place]);
    strcat(fullpath, subfolder);

    SceUID dir = sceIoDopen(fullpath);
    SceIoDirent dit;
    memset(&dit, 0, sizeof(SceIoDirent));

    while ((sceIoDread(dir, &dit)) > 0){
        if (dit.d_name[0] == '.') continue;
        char* ext = sce_paf_private_strrchr(dit.d_name, '.');
        if (ext && strcasecmp(ext, ".prx") == 0 && !isPluginInstalled(plugins_paths[place], dit.d_name)){
            char plugin_name[64];
            sce_paf_private_sprintf(plugin_name, "%s/%s", subfolder, dit.d_name);
            processInstallablePlugin(plugin_name, place);
        }
    }

    sceIoDclose(dir);
}

void findInstallablePlugins(){
    clear_list(&iplugins, &list_cleaner);
    for (int i=0; i<NELEMS(plugins_paths)-1; i++){
        SceUID dir = sceIoDopen(plugins_paths[i]);
        SceIoDirent dit;
        memset(&dit, 0, sizeof(SceIoDirent));

        while ((sceIoDread(dir, &dit)) > 0){
            if (dit.d_name[0] == '.') continue;
            if (isFolder(&dit)){
                findInstallablePluginsSubfolder(i, dit.d_name);
                continue;
            }
            char* ext = sce_paf_private_strrchr(dit.d_name, '.');
            if (ext && strcasecmp(ext, ".prx") == 0 && !isPluginInstalled(plugins_paths[i], dit.d_name)){
                processInstallablePlugin(dit.d_name, i);
            }
        }

        sceIoDclose(dir);
    }
}

void installPlugin(Plugin* plugin){

    static char* runlevels[] = {
        "none",
        "always",
        "game",
        "vsh",
        "pops",
        "psp",
        "homebrew",
    };

    char txtpath[128];
    sce_paf_private_strcpy(txtpath, plugins_paths[plugin->place]);
    strcat(txtpath, PLUGINS_FILE);

    SceUID fd = sceIoOpen(txtpath, PSP_O_WRONLY|PSP_O_CREAT|PSP_O_APPEND, 0777);
    sceIoWrite(fd, "\n", 1);
    sceIoWrite(fd, runlevels[plugin->active], strlen(runlevels[plugin->active]));
    sceIoWrite(fd, ", ", 2);
    sceIoWrite(fd, plugins_paths[plugin->place], strlen(plugins_paths[plugin->place]));
    sceIoWrite(fd, plugin->path, strlen(plugin->path));
    sceIoWrite(fd, ", on\n", 5);
    sceIoClose(fd);

}

char* getPluginName(char* plugin_path, char* file){

    char *p = sce_paf_private_strrchr(plugin_path, '/');
    if (!p)
        p = sce_paf_private_strchr(plugin_path, ',');
    if (p)
    {
        p = strtrim(p+1);
        
    }
    else {
        p = strtrim(plugin_path);
    }
    
    int len = strlen(p);
    char *p2 = sce_paf_private_strchr(p + 1, '.');
    
    if(p2) len = (int)(p2 - p);

    sce_paf_private_strncpy(file, p, len);
    file[len] = '\0';

    return file;
}
