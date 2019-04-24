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


app=can_test

all: default

#CFLAGS="-I$(SYSROOT_DIR)/usr/include/libnl3/"
ARCH=arm

default: 
	@$(CROSS_COMPILE)gcc $(CFLAGS) --sysroot=$(SYSROOT_DIR) -o $(app) $(app).c

clean:
	@rm $(app)

