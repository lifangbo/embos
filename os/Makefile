PREFIX  = /usr/local
ARCH    = h8300-elf
BINDIR  = $(PREFIX)/bin
ADDNAME = $(ARCH)-

AR      = $(BINDIR)/$(ADDNAME)ar
AS      = $(BINDIR)/$(ADDNAME)as
CC      = $(BINDIR)/$(ADDNAME)gcc
LD      = $(BINDIR)/$(ADDNAME)ld
NM      = $(BINDIR)/$(ADDNAME)nm
OBJCOPY = $(BINDIR)/$(ADDNAME)objcopy
OBJDUMP = $(BINDIR)/$(ADDNAME)objdump
RANLIB  = $(BINDIR)/$(ADDNAME)ranlib
STRIP   = $(BINDIR)/$(ADDNAME)strip

TARGET = bin/kernel

#kernelビルドオプション
CFLAGS = -Wall -mh -nostdinc -nostdlib -fno-builtin
#CFLAGS += -mint32
CFLAGS += -I.
#CFLAGS += -g
CFLAGS += -Os


#OSソースマクロ
CFLAGS += -DKERNEL
CFLAGS += -DTSK_LIBRARY
CFLAGS += -DLOG
#CFLAGS += -DDEBUG
CFLAGS += -DKERNEL_MSG

#OSリンクオプション
LFLAGS = -static -T kernel/ld.scr


#include directive
#下位のmkファイルへのパラメータ渡し
ASM_SOURCES :=
C_SOURCES :=

ARCH_CPU_DIR := arch/cpu/
include $(ARCH_CPU_DIR)build.mk

TARGET_DRIVER_DIR := target/driver/
include $(TARGET_DRIVER_DIR)build.mk

KERNEL_DIR := kernel/
include $(KERNEL_DIR)build.mk

KERNEL_SVC_DIR := kernel_svc/
include $(KERNEL_SVC_DIR)build.mk

NET_DIR := net/
include $(NET_DIR)build.mk

CLIB_DIR := c_lib/
include $(CLIB_DIR)build.mk

TSKLIB_DIR := tsk_lib/
include $(TSKLIB_DIR)build.mk


ASM_OBJ := $(ASM_SOURCES:.s=.o)
C_OBJ := $(C_SOURCES:.c=.o)
OBJS += $(ASM_OBJ) $(C_OBJ)
OBJS := $(addprefix objs/,$(OBJS))


all : $(TARGET)

$(TARGET) :	$(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(CFLAGS) $(LFLAGS)


# startup.oとmain.oコンポーネントを先に生成
# -先に生成しないと，シンボルがずれ，エントリーアドレスから起動できなくなる
objs/%.o : $(ARCH_CPU_DIR)%.s
	$(CC) -c $(CFLAGS) $< -o $@

objs/%.o : $(ARCH_CPU_DIR)%.c
	$(CC) -c $(CFLAGS) $< -o $@

objs/%.o : $(TARGET_DRIVER_DIR)%.c
	$(CC) -c $(CFLAGS) $< -o $@

objs/%.o : $(ARCH_GCC_DIR)%.c
	$(CC) -c $(CFLAGS) $< -o $@

objs/%.o : $(KERNEL_DIR)%.c
	$(CC) -c $(CFLAGS) $< -o $@

objs/%.o : $(KERNEL_SVC_DIR)%.c
	$(CC) -c $(CFLAGS) $< -o $@

objs/%.o : $(NET_DIR)%.c
	$(CC) -c $(CFLAGS) $< -o $@

objs/%.o : $(CLIB_DIR)%.c
	$(CC) -c $(CFLAGS) $< -o $@

objs/%.o : $(TSKLIB_DIR)%.c
	$(CC) -c $(CFLAGS) $< -o $@



clean :
	rm -f $(OBJS) $(TARGET)
	rm -f *~ $(ARCH_CPU_DIR)*.*~ $(TARGET_DRIVER_DIR)*.*~ $(ARCH_GCC_DIR)*.*~ $(KERNEL_DIR)*.*~ \
        $(KERNEL_SVC_DIR)*.*~ $(NET_DIR)*.*~ $(CLIB_DIR)*.*~ $(TSKLIB_DIR)*.*~
