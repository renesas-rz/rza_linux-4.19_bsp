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
Some setup is needed in the ceu_omni.c file for your board.

You must hard code a RAM location for the CEU to capture images to.
This is done at the top of the file:
	static unsigned int cap_buf_addr = 0x60900000;	/* 9MB offset in internal RAM */
	static unsigned int cap_buf_size = (1*1024*1024); 	/* Capture buffer size */

You cannot simply 'malloc' this RAM in the application because you need to use a
physical RAM address (not virutal address) and the memory must be continuous (not paged)

Note when external SDRAM is used, it is easy to hard code the RAM loacion to internal RAM
because internal RAM is not used by the kernel.


----------------------------------------
Stream it Board
----------------------------------------
When running this code on a Stream it board, it will automatically detect that it is a stream it board
and adjust the settings for you. However, this assumes you have made the appropriate changes as
for a V2.3 board or later outlined here:
	https://elinux.org/RZ-A/Boards/Stream-it


