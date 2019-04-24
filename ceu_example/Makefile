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
# NOTE: You can also pass in things like CFLAGS on the command line.
# For example "CFLAGS=-DDEBUG  make"
#######################################################################
app_name	:= ceu_omni
LIBS		:=
CFLAGS		+= -O2
SRC_FILES	:= ceu_omni.c ceu.c ov7670.c ov7740.c

# For adding in external libraries, you have 2 choices:
#  1. let them link in a run-time .so file
#  2. pull in  all the code at build (no .so file needed at run-time)
#
# For 1:
# Link against libjpeg shared library 

# For adding in external libraries, you have 2 choices:
#  1. let them link in a run-time .so file
#  2. pull in  all the code at build (no .so file needed at run-time)
#
# For 1:
# Link against libjpeg shared library 
CFLAGS+=-ljpeg

# For 2:
# Add libjpeg statically by specifying the lib directly at the end of the command line (after the .c files)
#LIBS=$(SYSROOT_DIR)/usr/lib/libjpeg.a

#for debugging only
#CFLAGS+=-g


# Check that our build environtment is set up correctly
ifeq ("$(BUILDROOT_DIR)","")
BR_CHECK := $(error "The environment variable BUILDROOT_DIR has not been set.")
endif
ifeq ("$(SYSROOT_DIR)","")
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
	$(CROSS_COMPILE)gcc $(STATIC) $(CFLAGS) --sysroot=$(SYSROOT_DIR) -o $(app_name) $(SRC_FILES) $(LIBS)

# Strip off debug symbols if not debugging (makes file size smaller)
strip:
	$(CROSS_COMPILE)strip --strip-debug $(app_name)

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

