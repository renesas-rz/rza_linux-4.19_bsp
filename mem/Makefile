# Pass "make S=1" on the make comamnd line to build a statically linked application
ifeq ($(S),1)
	STATIC = -static
else
	STATIC =
endif

#######################################################################
# Application Specific setup
#
# (Change as needed)
#######################################################################
app_name	:= mem
LIBS		:=
CFLAGS		:= -O2
SRC_FILES	:= mem.c


#######################################################################
# Toolchain setup
#
# (Should not require chanages)
#######################################################################

# You want to build apps with the same toolchain and libraries
# that you built your root file system with. We'll assume you
# are using Buildroot.
# Therefore, make sure BUILDROOT_DIR is defined before calling this
# Makefile.
# 'BUILDROOT_DIR' should be imported after running setup_env.sh

# Define the base path of the toolchain and put it in the system path
ifneq ("$(wildcard $(BUILDROOT_DIR)/output/host/bin)","")
  # Buildroot after 2018, output/host/bin contains host apps for building
  TC_PATH := $(BUILDROOT_DIR)/output/host
else
  # Buildroot before 2018, output/host/usr/bin contains host apps for building
  TC_PATH := $(BUILDROOT_DIR)/output/host/usr
endif
export PATH:=$(TC_PATH)/bin:$(PATH)

# Find out what the prefix of the toolchain that is used for Buildroot
GCCEXE := $(notdir $(wildcard $(TC_PATH)/bin/*abi*-gcc))
CROSS_COMPILE := $(subst gcc,,$(GCCEXE))

# Find the location of the sysroot where all the libraries and header files are
SYSROOT := $(shell find $(TC_PATH) -name sysroot)

# Check that our build environtment is set up correctly
ifeq ("$(BUILDROOT_DIR)","")
BR_CHECK := $(error "The environment variable BUILDROOT_DIR has not been set.")
endif
ifeq ("$(SYSROOT)","")
SR_CHECK := $(error "The SYSROOT in the toolchain can not be detected")
endif
ifeq ("$(CROSS_COMPILE)","")
CC_CHECK := $(error "The CROSS_COMPILE prefix for the toolchain can not be detected")
endif
$(BR_CHECK)
$(SR_CHECK)
$(CC_CHECK)


# Manual setup if set_env.sh is not used
#TC_PATH=/home/renesas/toolchain/buildroot-2014.05/output/host/usr
#CROSS_COMPILE=arm-buildroot-linux-uclibcgnueabi-
ARCH:=arm

all: default

#default: br_check sr_check cc_check
default:
	$(CROSS_COMPILE)gcc $(STATIC) $(CFLAGS) --sysroot=$(SYSROOT) -o $(app_name) $(SRC_FILES) $(LIBS)

install:
	@# Copy to Buildroot overlay directory (only for Renesas BSP)
	cp $(app_name) $(BUILDROOT_DIR)/output/rootfs_overlay/root/bin
	@echo ''
	@echo 'Please run Buildroot again to repackage the rootfs image.'
	@echo 'For example:'
	@echo '  $$ cd $$BUILDROOT_DIR'
	@echo '  $$ make'

clean:
	@rm -f *.o
	@rm -f *.a
	@rm -f $(app_name)

