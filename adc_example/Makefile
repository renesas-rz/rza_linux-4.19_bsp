#BUILDROOT_DIR:=/home/renesas/archive/toolchain/buildroot-2014.05

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

all: default

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


#######################################################################
# Application Specific setup
#######################################################################
app:=adc_example


CFLAGS:=-O2

# For adding in external libraries, you have 2 choices:
#  1. let them link in a run-time .so file
#  2. pull in  all the code at build (no .so file needed at run-time)
#
# For 1:
# Link against libjpeg shared library 
#CFLAGS+=-ljpeg

# For 2:
# Add libjpeg statically by specifying the lib directly at the end of the command line (after the .c files)
#LIBS=$(SYSROOT)/usr/lib/libjpeg.a

default:
	@$(BR_CHECK)
	$(CROSS_COMPILE)gcc $(STATIC) $(CFLAGS) --sysroot=$(SYSROOT) -o $(app) $(app).c $(LIBS)

# Strip off debug symbols if not debugging (makes file size smaller)
strip:
	$(CROSS_COMPILE)strip --strip-debug $(app)

install:
	@# Copy to Buildroot overlay directory (only for Renesas BSP)
	cp $(app) $(BUILDROOT_DIR)/output/rootfs_overlay/root/bin
	@echo ''
	@echo 'Please run Buildroot again to repackage the rootfs image.'
	@echo 'For example:'
	@echo '  $$ cd $$BUILDROOT_DIR'
	@echo '  $$ make'

clean:
	rm $(app)

