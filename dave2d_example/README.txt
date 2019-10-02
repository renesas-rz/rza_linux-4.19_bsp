=======================================================
D/AVE 2D sample software and documentation
=======================================================

This software only applies to the RZ/A2M.

The software and documentation in these directories were taken from the
following release package from TES.
	Date: 2018-07-25
	SVN revision: 1685
Not all files from the orgiginal TES package are being included here.


Please refer to DISCLAIMER.txt about usage in a product.


Linux Kernel Requirements:
 1. Make sure to set CONFIG_DAVE2D=y in your kernel config.

	$ ./build.sh kernel menuconfig

	(in menuconfig)
	Device Drivers -> Graphics support -> [*] TES D/AVE 2D Rendering Engine

 2. Make sure to enable D/AVE in the Device Tree.

    rza_linux-4.19_bsp/output/linux-4.19/arch/arm/boot/dts/r7s9210-rza2mevb.dts

    Change '#if 0' to '#if 1'
	/* ========== D/AVE 2D Example ========== */
	/* Requires CONFIG_DAVE2D=y */
	# if 1
	&drw {


Build instructions:
 (start in rza_linux-4.19_bsp )
 $ ./build.sh env
 $ export ROOTDIR=$(pwd) ; source ./setup_env.sh
 $ cd dave2d_example
 $ make
 $ make install
 $ cd ..
 $ ./build.sh buildroot


Run instructions:
 * Simply run the program 'balls' wihout any arugments. The program exit after
   10 seconds.

