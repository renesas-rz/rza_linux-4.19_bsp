Hello Registers Application Example


This example shows you can build your own applications that can access
peripheral registers directly without creating a 'kernel driver'

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

	$ cd hello_registers
	$ make
	$ make install
	$ cd ..
	$ ./build.sh buildroot

