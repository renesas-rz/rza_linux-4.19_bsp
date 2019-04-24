#!/bin/bash
# Will be run automatically by build.sh
# Please edit the settings below if you don't want to use the defaults

# When calling this script, an enviroment variable 'ROOTDIR' must be set
# to contain a full path.
if [ "$ROOTDIR" == "" ]; then
  echo -e '
ERROR: You must set ROOTDIR before calling this file.
   If you want to use this file without build.sh, then
   you could pass it on the same line like:

      $ export ROOTDIR=$(pwd) ; source ./setup_env.sh

'
  exit
fi

# Settings
export OUTDIR=${ROOTDIR}/output

# As of GCC 4.9, you can now get a colorized output
export GCC_COLORS='error=01;31:warning=01;35:note=01;36:caret=01;32:locus=01:quote=01'

# Buildroot directory
if [ -e $OUTDIR/br_version.txt ] ; then
  source $OUTDIR/br_version.txt
fi
export BUILDROOT_DIR=$OUTDIR/buildroot-$BR_VERSION


# Define the base path of the toolchain
if [ -e $BUILDROOT_DIR/output/host/bin ] ; then
  # Buildroot versions after 2018, output/host/bin contains host apps for building
  export TC_PATH=$BUILDROOT_DIR/output/host
else
  # Buildroot versions before 2018, output/host/usr/bin contains host apps for building
  export TC_PATH=$BUILDROOT_DIR/output/host/usr
fi
export TOOLCHAIN_DIR=$TC_PATH

# Set toolchain prefix and add to path
cd $TC_PATH/bin
export CROSS_COMPILE=`ls *abi*-gcc | sed 's/gcc//'`
export PATH=`pwd`:$PATH
cd -
export ARCH=arm

# Find the location of the sysroot where all the libraries and header files are
export SYSROOT_DIR=`find $TC_PATH -name sysroot`
if [ "$SYSROOT_DIR" == "" ] ; then
  echo "==========================================="
  echo "  ERROR: Cannot find sysroot"
  echo "         Toolchain path = $TC_PATH"
  echo "==========================================="
fi


# -------------------------------------------------
# Change prompt to inform the BSP env has been set
# -------------------------------------------------
# Uncomment the prompt you want

# Change prompt to (rza_bsp)
#PS1="(rza_bsp)$ "

# Change prompt to (rza_bsp) with RED text
#PS1="\[\e[1;31m\](rza_bsp)$\[\e[00m\] "

# Change prompt to (rza_bsp) with RED text
# with current directory printed out on the line above
#PS1="dir: \w\n\[\e[1;31m\](rza_bsp)$\[\e[00m\] "

# Change prompt to (rza_bsp) with RED text
# with current directory printed out on the line above in ORANGE text
PS1="\[\e[33m\]dir: \w\n\[\e[1;31m\](rza_bsp)$\[\e[00m\] "


echo "Build Environment set"
export ENV_SET=1


