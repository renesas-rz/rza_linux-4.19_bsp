ifeq ("$(CROSS_COMPILE)","")
	CC_CHECK:=echo !!ERROR!! CROSS_COMPILE not set
else
endif

ifeq ("$(BUILDROOT_DIR)","")
	BR_CHECK:=echo !!ERROR!! BUILDROOT_DIR not set
else
endif

ifeq ($(S),1)
	STATIC = -static
else
        STATIC =
endif


#######################################################################
# Toolchain setup
#######################################################################

# You want to build apps with the same toolchain and libraries
# that you built your root file system with. We'll assume you
# are using Buildroot.
# Therefore, make sure BUILDROOT is defined before calling this
# Makefile.

# 'BUILDROOT_DIR' should be imported after running setup_env.sh

# Define the base path of the toolchain
TC_PATH:=$(BUILDROOT_DIR)/output/host/usr
export PATH:=$(TC_PATH)/bin:$(PATH)

# Find out what the prefix of the toolchain that is used for Buildroot
GCCEXE:=$(notdir $(wildcard $(TC_PATH)/bin/*abi*-gcc))
CROSS_COMPILE:=$(subst gcc,,$(GCCEXE))

# Find the location of the sysroot where all the libraries and header files are
SYSROOT:=$(shell find $(TC_PATH) -name sysroot)

# Manual setup if set_env.sh is not used
#TC_PATH=/home/renesas/toolchain/buildroot-2014.05/output/host/usr
#CROSS_COMPILE=arm-buildroot-linux-uclibcgnueabi-
ARCH:=arm

app := balls

# D/AVE 2D Driver files
SRCS += $(wildcard driver/code/*.c)
SRCS += $(wildcard driver/linux/*.c)
SRCS += $(wildcard driver/linux/memmgr/*.c)
SRCS += $(wildcard driver/linux/driver_l1/*.c)
OBJS = $(SRCS:.c=.o)


# Location of header files for D/AVE library
INC = -Idriver/inc -Idriver/linux -Idriver/linux/driver_l1 -Idriver/linux/memmgr -Idriver/linux/kernel
CFLAGS:=-O2 --sysroot=$(SYSROOT_DIR) $(INC)

.PHONY: all
all: dave2d.so balls

%.o: %.c
	@echo CC $<
	@$(CROSS_COMPILE)gcc $(CFLAGS) -c -fPIC -Wint-conversion -o $@ $<

# D/AVE User Driver Library
dave2d.so: $(OBJS)
	@echo LD $@
	@$(CROSS_COMPILE)gcc $(CFLAGS) -fPIC -Wint-conversion -shared -o $@ $^
	@echo AR dave2d.a
	@$(CROSS_COMPILE)ar rcs dave2d.a $(OBJS)

# Convert .jpg to .c array
example/bg_800x480.c: example/bg_800x480.jpg
	@echo "Converting bg_800x480.jpg to bg_800x480.c"
	@cd example ; ./jpg_to_c.sh

# Example Application
balls: example/balls.c example/bg_800x480.c
	@echo CC example/$@.c
	@$(CROSS_COMPILE)gcc $(CFLAGS) -Wall example/$@.c -o $@ dave2d.a -lm


# Strip off debug symbols if not debugging (makes file size smaller)
strip:
	#$(CROSS_COMPILE)strip --strip-all --strip-debug $(app)
	$(CROSS_COMPILE)strip -v --strip-all $(app)

install:
	@# Copy to Buildroot overlay directory (only for Renesas BSP)
	cp $(app) $(BUILDROOT_DIR)/output/rootfs_overlay/root/bin
	@echo ''
	@echo 'Please run Buildroot again to repackage the rootfs image.'
	@echo 'For example:'
	@echo '  $$ cd $$BUILDROOT_DIR'
	@echo '  $$ make'

.PHONY : clean
clean:
	@# Clean library files
	@rm -f $(shell find . -name '*.o')
	@rm -f $(shell find . -name '*.a')
	@rm -f $(shell find . -name '*.so')

	@# Clean apps
	@rm -f $(app)
	@rm -f example/bg_800x480.c

