cmake_minimum_required(VERSION 3.28)

project(xmbcontrol VERSION 1.0 LANGUAGES C ASM)

set(DEBUG "" CACHE STRING "Debug level (1,2,3)")
message(STATUS "DEBUG LEVEL = ${DEBUG}")

include(FetchContent)

FetchContent_Declare(
   ark-dev-sdk
   GIT_REPOSITORY https://github.com/PSP-Arkfive/ark-dev-sdk.git
   GIT_TAG main
   EXCLUDE_FROM_ALL
)

FetchContent_MakeAvailable(ark-dev-sdk)

set(ARKSDK ${ark-dev-sdk_SOURCE_DIR})

add_prx_module(xmbcontrol exports.exp)

target_sources(xmbcontrol PRIVATE 
   main.c
   stub.S
   src/xmbpatch.c
   src/list.c
   src/settings.c
   src/config.c
   src/plugins.c
   src/battery.c
   src/vshmenu.c
   src/utils.c
)

remove_definitions("-D_PSP_FW_VERSION=600")
add_definitions("-D_PSP_FW_VERSION=660")

target_compile_options(xmbcontrol PRIVATE -std=c99 -O2 -Os -G0 -Wall -fshort-wchar -fno-pic -mno-check-zero-division)
target_include_directories(xmbcontrol PRIVATE include ${ARKSDK}/include)
target_link_directories(xmbcontrol PRIVATE ${ARKSDK}/libs)
target_link_libraries(xmbcontrol PRIVATE
   -nostartfiles
   pspsystemctrl_user
   pspkubridge
   pspvshctrl
   pspreg
   pspsysc_user
   psppower
   pspdebug
   pspdisplay
   pspge
   pspctrl
   pspnet
   pspnet_apctl
)

if(DEBUG)
   target_compile_definitions(xmbcontrol PRIVATE -DDEBUG=${DEBUG})
endif()
