TARGET = xmbctrl
OBJS = main.o \
	src/xmbpatch.o \
	src/list.o \
	src/config.o \
	src/settings.o \
	src/plugins.o \
	src/utils.o \

IMPORTS = imports.o

INCDIR = include external/include
CFLAGS = -std=c99 -Os -G0 -Wall -fno-pic

PSP_FW_VERSION = 660

OBJS += $(IMPORTS)
all: $(TARGET).prx
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

BUILD_PRX = 1
PRX_EXPORTS = exports.exp

USE_KERNEL_LIBS = 0
USE_KERNEL_LIBC = 0

LIBDIR = external/libs
LDFLAGS =  -nostartfiles
LIBS = -lpspsystemctrl_user -lpspkubridge -lpspvshctrl -lpspreg -lpspsysc_user

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
