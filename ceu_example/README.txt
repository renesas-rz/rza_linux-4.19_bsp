This code demonstrates how to use the CEU to capture images from the camera.

This code works with Omnivision OV7670 or OV7740 cameras.

----------------------------------------
Building
----------------------------------------

In order to save image as JPEG files, libjpeg is required to build against.
Make sure you have BR2_PACKAGE_LIBJPEG or BR2_PACKAGE_JPEG_TURBO selected in buildroot menuconfig.
Location:
  -> Target packages
    -> Libraries
      -> Graphics
       -> jpeg variant
               ( ) jpeg
               (X) jpeg-turbo


First set the BSP build environment

	$ ./build.sh env
	$ export ROOTDIR=$(pwd) ; source ./setup_env.sh

Then you can build the applicaiton:

	$ make

By doing a 'make install' will copy to your buildroot overlay directory. But, make sure
to rebuild buildroot to add it to your image to program into the board. For example:
	$ make install
	$ cd ..
	$ ./build.sh buildroot


----------------------------------------
Pin setup
----------------------------------------
This code assumes you have taken care of the CEU pin setup in u-boot or the kernel.


----------------------------------------
CEU Capture Buffer Address
----------------------------------------
You must hard code a RAM location for the CEU to capture images to.
You can see the values of "cap_buf_addr" depend on what board you are using.
You cannot simply 'malloc' this RAM in the application because you need to use a
physical RAM address (not virtual address) and the memory must be continuous (not paged).

When external SDRAM is used ('run xsa_boot', or 'run xha_boot)', it is easy to hard code
the capture RAM loacion to internal RAM because internal RAM is not used by the kernel.


----------------------------------------
Supported Boards
----------------------------------------
This code has been written to work on the follow boards:
 * Renesas RZ/A1H RSK
 * Renesas RZ/A1LU Stream it
 * Renesas RZ/A2M EVB

Stream it:
 * When using the stream it board, plese make sure you haver the appropriate changes for
   a V2.3 board or later outlined here:	https://elinux.org/RZ-A/Boards/Stream-it
 * u-boot will configure the CEU pins for you automatically when "#define SDRAM_SIZE_MB 16"
   is specified in the include/configs/streamit.h file.


RZ/A2M EVB:
 * When using the RZ/A2M EVB, please set switch SW6-4 = OFF on teh sub-board. Note that
   Ethernet #1 (eth0) cannot be used. However, Ethernet #2 (eth1) will work.
 * A Device Tree for this board has been supplied that will configure the CEU pins.
   To compile it, simple do:
     (rza_bsp)$ make
     (rza_bsp)$ make program

RZ/A1H RSK:
 * Note that the LCD that comes with the kit cannot be used at the same time as
   the camera because of pin muxing. However, the LVDS connector can be used.


