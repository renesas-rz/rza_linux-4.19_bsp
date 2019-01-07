ifeq ("$(BUILDROOT_DIR)","")
	BR_CHECK:=echo !!ERROR!! BUILDROOT_DIR not set
endif

# Define the base path of the toolchain
TC_PATH:=$(BUILDROOT_DIR)/output/host/
export PATH:=$(TC_PATH)/bin:$(PATH)

# Find out what the prefix of the toolchain that is used for Buildroot
GCCEXE:=$(notdir $(wildcard $(TC_PATH)/bin/*abi*-gcc))
CROSS_COMPILE:=$(subst gcc,,$(GCCEXE))

DIR		:= ${CURDIR}

DTS_FILE	:= r7s9210-rza2mevb-ceu

KERNEL_DIR	:= $(OUTDIR)/linux-4.19
DTC		:= $(KERNEL_DIR)/scripts/dtc/dtc
DTC_INCLUDE	:= $(KERNEL_DIR)/scripts/dtc/include-prefixes
GCC_INCLUDE	:= -I$(KERNEL_DIR)/arch/arm/boot/dts -I$(KERNEL_DIR)/include -I$(DTC_INCLUDE)

# Command line arguments taken from 'cmd_dtc' inside of linux-x.xx/scritps/Makefile.lib
DTC_FLAGS	:= -Wno-unit_address_vs_reg -Wno-unit_address_format -Wno-avoid_unnecessary_addr_size \
		   -Wno-alias_paths -Wno-graph_child_address -Wno-graph_port -Wno-unique_unit_address \
		   -Wno-pci_device_reg

all:
	@$(BR_CHECK)
	@# First use compiler to process pre-compiler macros (like #include)
	@$(CROSS_COMPILE)gcc -E -nostdinc $(GCC_INCLUDE) \
		-undef -D__DTS__ -x assembler-with-cpp -o $(DTS_FILE).dtb.dts.tmp $(DTS_FILE).dts
	@# Now use dtc inside kernel directory to create the the dtb
	@$(DTC) -O dtb -o $(DTS_FILE).dtb -b 0 \
		-i $(DTC_INCLUDE) $(DTC_FLAGS) \
		$(DTS_FILE).dtb.dts.tmp
	@# Finally remove our tmp file
	@rm $(DTS_FILE).dtb.dts.tmp

program:
	@$(BR_CHECK)
	@# Create a jlink script and execute it
	@cp -v $(DIR)/$(DTS_FILE).dtb /tmp/$(DTS_FILE).dtb.bin
	@echo "loadbin /tmp/$(DTS_FILE).dtb.bin,0x200C0000" > /tmp/jlink_load.txt
	@echo "g" >> /tmp/jlink_load.txt
	@echo "exit" >> /tmp/jlink_load.txt
	@JLinkExe -speed 15000 -if JTAG -jtagconf -1,-1 -device R7S921053VCBG -CommanderScript /tmp/jlink_load.txt

clean:
	@rm $(DTS_FILE).dtb*
