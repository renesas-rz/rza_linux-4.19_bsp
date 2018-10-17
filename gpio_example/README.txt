gpio_example.c

This is an example of access GPIO from a user application using the
kernel's GPIO interface.

This example runs on the RZ/A1H RSK board.

Buttons SW1, SW2 and SW3 are used as GPIO input, and LCD0 is used
as GPIO output.

The ADC connected to "RV1" on the RSK board is also displayed.

Step 1. Set up your build environment

 * Start in the base directory of the BSP (where build.sh is located).

 * Enter the command below to set the build environment.

	$ export ROOTDIR=$(pwd) ; source ./setup_env.sh

  NOTE: You can also type "./build.sh env" which will print out that
  command that you can copy/paste into the terminal.

Step 2. Build your application and add it to your file system.

 * The first step defined and environment variable BUILDROOT_DIR
   that the Makefile in this directory can use to then figure out
   where the toolchain is and where the sysroot (shared libraries)
   are.
   Enter the following commands:

	$ cd gpio_example
	$ make
	$ make install
	$ cd ..
	$ ./build.sh buildroot

