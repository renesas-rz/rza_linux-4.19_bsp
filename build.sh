#!/bin/bash

# Function: usage
function usage {
  echo -ne "\033[1;31m" # RED TEXT
  echo -e "\nWhat do you want to build?"
  echo -ne "\033[00m" # END TEXT COLOR
  echo -e "    ./build.sh config                  : Target Board Selection ($BOARD)"
  echo -e ""
  echo -e "    ./build.sh buildroot               : Builds Root File System (and installs toolchain)"
  echo -e "    ./build.sh u-boot                  : Builds u-boot"
  echo -e "    ./build.sh kernel                  : Builds Linux kernel. Default is to build uImage"
  echo -e "    ./build.sh axfs                    : Builds an AXFS image"
  echo -e ""
  echo -e "    ./build.sh env                     : Set up the Build environment so you can run 'make' directly"
  echo -e ""
  echo -e "    ./build.sh jlink                   : Downlaod a binary image to RAM so you can program it into QSPI"
  echo -e ""
  echo -e "  You may also do things like:"
  echo -e "    ./build.sh kernel menuconfig       : Open the kernel config GUI to enable options/drivers"
  echo -e "    ./build.sh kernel rskrza1_xip_defconfig : Switch to XIP version of the kernel"
  echo -e "    ./build.sh kernel xipImage         : Build the XIP kernel image"
  echo -e "    ./build.sh buildroot menuconfig    : Open the Buildroot config GUI to select additinal apps to build"
  echo -e ""
  echo -e "    Current Target: $BOARD"
  echo -e ""
}

# Function: banner_color
function banner_yellow {
  echo -ne "\033[1;33m" # YELLOW TEXT
  echo "============== $1 =============="
  echo -ne "\033[00m"
}
function banner_red {
  echo -ne "\033[1;31m" # RED TEXT
  echo "============== $1 =============="
  echo -ne "\033[00m"
}
function banner_green {
  echo -ne "\033[1;32m" # GREEN TEXT
  echo "============== $1 =============="
  echo -ne "\033[00m"
}

##### Check for required Host packages #####
MISSING_A_PACKAGE=0
PACAKGE_LIST=(git make gcc g++ python)

for i in 0 1 2 3 4; do
  CHECK=$(which ${PACAKGE_LIST[$i]})
  if [ "$CHECK" == "" ] ; then
    MISSING_A_PACKAGE=1
  fi
done
CHECK=$(dpkg -l 'libncurses5-dev' | grep '^ii')
if [ "$CHECK" == "" ] ; then
  MISSING_A_PACKAGE=1
fi

if [ "$MISSING_A_PACKAGE" != "0" ] ; then

  banner_red "Missing mandatory host packages"
  echo "Please make sure the following packages are installed on your machine."
  echo "    git"
  echo "    make"
  echo "    gcc"
  echo "    g++"
  echo "    libncurses5-dev libncursesw5-dev"
  echo "    python"
  echo ""
  echo "The following command line will ensure all packages are installed."
  echo ""
  echo "   sudo apt-get install git make gcc g++ libncurses5-dev libncursesw5-dev python"
  echo ""
  exit 1
fi

##### Check if toolchain is installed correctly #####
function check_for_toolchain {
  CHECK=$(which ${CROSS_COMPILE}gcc)
  if [ "$CHECK" == "" ] ; then
    # Toolchain was found in path, so maybe it was hard coded in setup_env.sh
    return
  fi
  if [ ! -e $OUTDIR/br_version.txt ] ; then
    banner_red "Toolchain not installed yet."
    echo -e "Buildroot will download and install the toolchain."
    echo -e "Plesae run \"./build.sh buildroot\" first and select the toolchain you would like to use."
    exit 1
  fi
}

# Save current config settings to file
function save_config {
  echo "BOARD=$BOARD" > output/config.txt
  echo "SOC=$SOC" >> output/config.txt
  echo "DLRAM_ADDR=$DLRAM_ADDR" >> output/config.txt
  echo "UBOOT_ADDR=$UBOOT_ADDR" >> output/config.txt
  echo "DTB_ADDR=$DTB_ADDR" >> output/config.txt
  echo "KERNEL_ADDR=$KERNEL_ADDR" >> output/config.txt
  echo "ROOTFS_ADDR=$ROOTFS_ADDR" >> output/config.txt
  echo "QSPI=$QSPI" >> output/config.txt
}

###############################################################################
# script start
###############################################################################

# Save current directory
ROOTDIR=`pwd`

#Defaults (for RZ/A1 RSK)
BOARD=rskrza1
SOC=RZA1H
DLRAM_ADDR=0x08000000
DLRAM_SZ=0x2000000
UBOOT_ADDR=0x18000000
DTB_ADDR=0x180C0000
KERNEL_ADDR=0x18200000
ROOTFS_ADDR=0x18800000
QSPI=DUAL

# Create output build directory
if [ ! -e output ] ; then
  mkdir -p output
fi

# Create config.txt file, or read in current settings
if [ ! -e output/config.txt ] ; then
  save_config
else
  source output/config.txt
fi

# Check command line
if [ "$1" == "" ] ; then
  usage
  exit 0
fi

# Run build environment setup
if [ "$ENV_SET" != "1" ] ; then
  # Because we are using 'source', ROOTDIR can be seen in setup_env.sh
  source ./setup_env.sh
fi

# Find out how many CPU processor cores we have on this machine
# so we can build faster by using multi-threaded builds
NPROC=2
if [ "$(which nproc)" != "" ] ; then  # make sure nproc is installed
  NPROC=$(nproc)
fi
BUILD_THREADS=$(expr $NPROC + $NPROC)

###############################################################################
# config
###############################################################################
if [ "$1" == "config" ] ; then

  # read in our list of boards
  source boards_renesas.txt
  source boards_custom.txt

  # make a list of all boards
  ALL_BOARDS="$RENESAS_BOARDS $CUSTOM_BOARDS"

  # save the current board so can know the user selected a new one
  ORIGINAL_BOARD=$BOARD

  while [ "1" == "1" ]
  do

    CURRENT_DESC="error"

    # What is the current selected board?
    for i in `echo $ALL_BOARDS` ; do
      if [ "$BOARD" == "$i" ] ; then
        BRD_DESC_xxxx=BRD_DESC_${i}
        CURRENT_DESC=$(eval echo \${$BRD_DESC_xxxx})
        break
      fi
    done

    whiptail --title "Build Environment Setup"  --noitem --menu "Make changes the items below as needed.\nYou may use ESC+ESC to cancel." 0 0 0 \
	" Target Board: $BOARD [$CURRENT_DESC]" "" \
	"       Device: $SOC" "" \
	"     RAM addr: $DLRAM_ADDR" "" \
	"  u-boot addr: $UBOOT_ADDR" "" \
	"     DTB addr: $DTB_ADDR" "" \
	"  kernel addr: $KERNEL_ADDR" "" \
	"  rootfs addr: $ROOTFS_ADDR" "" \
	"         QSPI: $QSPI" "" \
	"Save" "" 2> /tmp/answer.txt

    #ans=$(head -c 3 /tmp/answer.txt)
    ans=$(cat /tmp/answer.txt)

    if [ "$ans" == "" ]; then
      break;
    fi

    if [ "$(grep "Target Board" /tmp/answer.txt)" != "" ] ; then

WT_TEXT="whiptail --title \"Build Environment Setup\" --menu \
\"Please select the board you want to build for.\n\"\
\"If you have your own custom board, please manually edit\n\"\
\"file [boards_custom.txt] and then your board will show up\n\"\
\"in this menu.\n\"\
 0 0 40 "

    # add boards to menu
    for i in `echo $ALL_BOARDS` ; do
      BRD_DESC_xxxx=BRD_DESC_${i}
      CURRENT_DESC=$(eval echo \${$BRD_DESC_xxxx})
      WT_TEXT="$WT_TEXT \"$i\" \"$CURRENT_DESC\" "
    done

  eval $WT_TEXT 2> /tmp/answer.txt
  ans=$(cat /tmp/answer.txt)

    # No selection (cancel)
    if [ "$ans" == "" ] ; then
      continue
    fi

    BOARD=${ans}
    #echo "BOARD = $BOARD"

    BRD_SOC_xxxx=BRD_SOC_${ans}
    eval SOC=$(eval echo \${$BRD_SOC_xxxx})
    #echo "SOC = $SOC"

    BRD_DLRAM_xxxx=BRD_DLRAM_${ans}
    eval DLRAM_ADDR=$(eval echo \${$BRD_DLRAM_xxxx})
    #echo "DLRAM_ADDR = $DLRAM_ADDR"

    BRD_DLRAMSZ_xxxx=BRD_DLRAMSZ_${ans}
    eval DLRAM_SZ=$(eval echo \${$BRD_DLRAMSZ_xxxx})
    #echo "DLRAM_SZ = $DLRAM_SZ"

    BRD_UBOOT_xxxx=BRD_UBOOT_${ans}
    eval UBOOT_ADDR=$(eval echo \${$BRD_UBOOT_xxxx})
    #echo "UBOOT_ADDR = $UBOOT_ADDR"

    BRD_DTB_xxxx=BRD_DTB_${ans}
    eval DTB_ADDR=$(eval echo \${$BRD_DTB_xxxx})
    #echo "DTB_ADDR = $DTB_ADDR"

    BRD_KERNEL_xxxx=BRD_KERNEL_${ans}
    eval KERNEL_ADDR=$(eval echo \${$BRD_KERNEL_xxxx})
    #echo "KERNEL_ADDR = $KERNEL_ADDR"

    BRD_ROOTFS_xxxx=BRD_ROOTFS_${ans}
    eval ROOTFS_ADDR=$(eval echo \${$BRD_ROOTFS_xxxx})
    #echo "ROOTFS_ADDR = $ROOTFS_ADDR"

    BRD_QSPI_xxxx=BRD_QSPI_${ans}
    eval QSPI=$(eval echo \${$BRD_QSPI_xxxx})
    #echo "QSPI = $QSPI"

    continue
  fi

  if [ "$(grep 'RAM addr' /tmp/answer.txt)" != "" ] ; then
    whiptail --title "Address selection" --inputbox \
"Enter the address of the RAM that you can download images to\n"\
"using J-Link.\n"\
"This is only needed for the './build.sh jlink' command.\n"\
"If you only have internal RAM (no SDRAM), then you would\n"\
"enter 0x20000000.\n"\
"If you have external SDRAM on CS2, then you would enter 0x08000000.\n"\
"If you have external SDRAM on CS3, then you would enter 0x0C000000.\n"\
 0 0 0x0C000000 \
    2> /tmp/answer.txt
    ans=$(cat /tmp/answer.txt)
    # No selection (cancel)
    if [ "$ans" == "" ] ; then
      continue
    fi
    DLRAM_ADDR=$ans
  fi

  if [ "$(grep 'u-boot addr' /tmp/answer.txt)" != "" ] ; then
    whiptail --title "Address selection" --inputbox "Enter the address of u-boot:" 0 0 0x18000000 \
    2> /tmp/answer.txt
    ans=$(cat /tmp/answer.txt)
    # No selection (cancel)
    if [ "$ans" == "" ] ; then
      continue
    fi
    UBOOT_ADDR=$(cat /tmp/answer.txt)
  fi

  if [ "$(grep 'DTB addr' /tmp/answer.txt)" != "" ] ; then
    whiptail --title "Address selection" --inputbox "Enter the address of the Device Tree Blob:" 0 0 0x180C0000 \
    2> /tmp/answer.txt
    ans=$(cat /tmp/answer.txt)
    # No selection (cancel)
    if [ "$ans" == "" ] ; then
      continue
    fi
    DTB_ADDR=$ans
  fi

  if [ "$(grep 'kernel addr' /tmp/answer.txt)" != "" ] ; then
    whiptail --title "Address selection" --inputbox "Enter the address of the Linux kernel:" 0 0 0x18200000 \
    2> /tmp/answer.txt
    ans=$(cat /tmp/answer.txt)
    # No selection (cancel)
    if [ "$ans" == "" ] ; then
      continue
    fi
    KERNEL_ADDR=$ans
  fi

  if [ "$(grep 'rootfs addr' /tmp/answer.txt)" != "" ] ; then
    whiptail --title "Address selection" --inputbox "Enter the address of the Root File System:" 0 0 0x18800000 \
    2> /tmp/answer.txt
    ans=$(cat /tmp/answer.txt)
    # No selection (cancel)
    if [ "$ans" == "" ] ; then
      continue
    fi
    ROOTFS_ADDR=$ans
  fi

  if [ "$(grep QSPI /tmp/answer.txt)" != "" ] ; then
    whiptail --title "QSPI Selection" --noitem --menu "Is your system single or dual QSPI?" 0 0 40 \
	"SINGLE" "" \
	"DUAL" "" \
	2> /tmp/answer.txt
    ans=$(cat /tmp/answer.txt)
    # No selection (cancel)
    if [ "$ans" == "" ] ; then
      continue
    fi
    QSPI=$ans
  fi

  if [ "$(grep "Save" /tmp/answer.txt)" != "" ] ; then
    save_config

    # If our board selection has changed, then delete the .config files
    # for u-boot and kernel which will force a new defconfig
    if [ "$ORIGINAL_BOARD" != "$BOARD" ] ; then
      if [ -e $OUTDIR/u-boot-2017.05/.config ] ; then
        rm $OUTDIR/u-boot-2017.05/.config
      fi
      if [ -e $OUTDIR/linux-4.19/.config ] ; then
        rm $OUTDIR/linux-4.19/.config
      fi
    fi

    break;
  fi

  done

  exit 0
fi

###############################################################################
# env
###############################################################################
if [ "$1" == "env" ] ; then

  check_for_toolchain

  echo "Copy/paste this line and execute it in your command window."
  echo ""
  echo 'export ROOTDIR=$(pwd) ; source ./setup_env.sh'
  echo ""
  echo "Then, you can execute 'make' directly in u-boot, linux, buildroot, etc..."
  exit 0
fi

###############################################################################
# jlink
###############################################################################
if [ "$1" == "jlink" ] ; then
  echo "-------------------------------------------------------"
  echo "Download binary files to on-board RAM or SPI Flash"
  echo "-------------------------------------------------------"

FILE1=output/u-boot-2017.05/u-boot.bin
if [ "$SOC" != "RZA2M" ] ; then
FILE2=output/linux-4.19/arch/arm/boot/dts/r7s72100-$BOARD.dtb
else
FILE2=output/linux-4.19/arch/arm/boot/dts/r7s9210-$BOARD.dtb
fi
FILE3=output/linux-4.19/arch/arm/boot/uImage
FILE4=output/linux-4.19/arch/arm/boot/xipImage
FILE5=output/buildroot-$BR_VERSION/output/images/rootfs.squashfs
FILE6=output/buildroot-$BR_VERSION/output/images/rootfs.axfs
FILE7=output/buildroot-$BR_VERSION/output/images/cramfs.axfs
FILE8=output/axfs/rootfs.axfs.bin

  if [ "$2" == "" ] ; then
    echo "
usage: ./build.sh jlink {FILE} {ADDRESS} {SPI}
     FILE: The path to the file to download.
  ADDRESS: (Optional) RAM or SPI Flash address. Default is $DLRAM_ADDR (beginning of your RAM)
      SPI: (Optional) 'single' or 'dual' SPI Flash. Default is 'single' SPI flash programming

Examples: (full path)
  ./build.sh jlink output/u-boot-2017.05/u-boot.bin $UBOOT_ADDR

Examples: (shortcut names)
  ./build.sh jlink u-boot              # u-boot
  ./build.sh jlink dtb                 # Device Tree"
    if [ -e $FILE3 ] ; then echo "  ./build.sh jlink uImage              # Linux Kernel"
    fi
    if [ -e $FILE4 ] ; then echo "  ./build.sh jlink xipImage            # Linux XIP Kernel"
    fi
    if [ -e $FILE5 ] ; then echo "  ./build.sh jlink rootfs_squashfs     # Root File System"
    fi
    if [ -e $FILE6 ] || [ -e $FILE7 ] ; then echo "  ./build.sh jlink rootfs_axfs         # Root File System"
    fi

    echo "
Examples: (file number)
  ./build.sh jlink 1
  File Numbers:"
    if [ -e $FILE1 ] ; then  echo "    1 = $FILE1"
    fi
    if [ -e $FILE2 ] ; then  echo "    2 = $FILE2"
    fi
    if [ -e $FILE3 ] ; then  echo "    3 = $FILE3"
    fi
    if [ -e $FILE4 ] ; then  echo "    4 = $FILE4"
    fi
    if [ -e $FILE5 ] ; then  echo "    5 = $FILE5"
    fi
    if [ -e $FILE6 ] ; then  echo "    6 = $FILE6"
    fi
    if [ -e $FILE7 ] ; then  echo "    7 = $FILE7"
    fi
    if [ -e $FILE8 ] ; then  echo "    7 = $FILE8"
    fi

  echo "
Examples: (Download directory to QSPI Flash)
  ./build.sh jlink u-boot $UBOOT_ADDR
  ./build.sh jlink dtb $DTB_ADDR
"
    if [ "$QSPI" == "SINGLE" ] ; then
      echo \
"  ./build.sh jlink uImage $KERNEL_ADDR
  ./build.sh jlink xipImage $KERNEL_ADDR
  ./build.sh jlink rootfs_squashfs $ROOTFS_ADDR
  ./build.sh jlink rootfs_axfs $ROOTFS_ADDR
"
    fi
    echo -ne "\033[1;31m" # RED TEXT
    echo -ne "\nNOTE:"
    echo -ne "\033[00m" # END TEXT COLOR
    echo -e "Your board should be up and running in u-boot first\n     (ie, not Linux) before executing this command."

    exit 0
 fi

  # Shortcuts
  # Change our passed arguments to full paths
  if [ "$2" == "uboot" ] || [ "$2" == "u-boot" ] ; then
    if [ "$3" == "" ] ; then
      set -- $1 $FILE1 $UBOOT_ADDR $4
    else
      set -- $1 $FILE1 $3 $4
    fi
  fi
  if [ "$2" == "dtb" ] ; then
    if [ "$3" == "" ] ; then
      set -- $1 $FILE2 $DTB_ADDR $4
    else
      set -- $1 $FILE2 $3 $4
    fi
  fi
  if [ "$2" == "uImage" ] ; then
    set -- $1 $FILE3 $3 $4
  fi
  if [ "$2" == "xipImage" ] ; then
    set -- $1 $FILE4 $3 $4
  fi
  if [ "$2" == "rootfs_squashfs" ] ; then
    set -- $1 $FILE5 $3 $4
  fi
  if [ "$2" == "rootfs_cramfs" ] ; then
    set -- $1 $FILE7 $3 $4
  fi
  if [ "$2" == "rootfs_axfs" ] ; then
    # Choose Buildroot as default
    if [ -e $FILE6 ] ; then
      set -- $1 $FILE6 $3 $4
    else
      set -- $1 $FILE7 $3 $4
    fi
 fi

  # File Numbers
  if [ "$2" == "1" ] ; then
    if [ "$3" == "" ] ; then
      set -- $1 $FILE1 $UBOOT_ADDR $4
    else
      set -- $1 $FILE1 $3 $4
    fi
  fi
  if [ "$2" == "2" ] ; then
    if [ "$3" == "" ] ; then
      set -- $1 $FILE2 $DTB_ADDR $4
    else
      set -- $1 $FILE2 $3 $4
    fi
  fi
  if [ "$2" == "3" ] ; then
    set -- $1 $FILE3 $3 $4
  fi
  if [ "$2" == "4" ] ; then
    set -- $1 $FILE4 $3 $4
  fi
  if [ "$2" == "5" ] ; then
    set -- $1 $FILE5 $3 $4
  fi
  if [ "$2" == "6" ] ; then
    set -- $1 $FILE6 $3 $4
  fi
  if [ "$2" == "7" ] ; then
    set -- $1 $FILE7 $3 $4
  fi
  if [ "$2" == "8" ] ; then
    set -- $1 $FILE8 $3 $4
  fi

  # File check
  if [ ! -e "$2" ] ; then
    echo "ERROR: File does not exist. $2"
    exit 128
  fi

  filename=$(basename "$2")
  extension="${filename##*.}"
  #filename_only="${filename%.*}"

  # Jlink must have a file extension of .bin for downloading
  if [ "$extension" == "bin" ] ; then
    dlfile=/tmp/$filename
  else
    dlfile=/tmp/$filename.bin
  fi
  cp -v "$2" $dlfile

  ramaddr=$3
  if [ "$ramaddr" == "" ] ; then
    ramaddr=$DLRAM_ADDR
  fi

  # check single or dual
  if [ "$4" != "" ] ; then
    if [ "$4" != "single" ] && [ "$4" != "dual" ] ; then
      echo ""
      banner_red "Argument 4 must be either 'single' or 'dual'"
      echo ""
      exit 128
    fi
  fi

  # Create a jlink script and execute it
  echo "loadbin $dlfile,$ramaddr" > /tmp/jlink_load.txt
  echo "g" >> /tmp/jlink_load.txt
  echo "exit" >> /tmp/jlink_load.txt

  # After version 5.10, a new command line option is needed
  CHECK=`which JLinkExe`
  JLINKPATH=$(readlink -f $CHECK | sed 's:/JLinkExe::')
  if [ -e $JLINKPATH/libjlinkarm.so.5 ] ; then
    JLINKVER=$(ls $JLINKPATH/libjlinkarm.so.5.* | sed "s:$JLINKPATH/libjlinkarm.so.::")
    # Since version numbers are not really 'numbers', we'll use 'sort' to figure
    # out what one is the smallest, and if 5.10 is still the first number in the list,
    # then we know the current version is either the same or later
    ORDER=$(echo -e "5.10\\n$JLINKVER" | sort -V)
    if [ "${ORDER:0:4}" == "5.10" ] ; then
      JTAGCONF='-jtagconf -1,-1'
    fi
  fi
  if [ -e $JLINKPATH/libjlinkarm.so.6 ] ; then
      JTAGCONF='-jtagconf -1,-1'
  fi

  # clear our log
  echo "" > /tmp/jlink.log
  # keep a copy of the output (using 'tee') in a log file to check if the operation
  # was successful or not because jlink does not return an error code
  if [ "$SOC" != "RZA2M" ] ; then
    # == RZ/A1 ==
    if [ "$4" == "dual" ] ; then
      JLinkExe -speed 15000 -if JTAG $JTAGCONF -device R7S721001_DualSPI -CommanderScript /tmp/jlink_load.txt | tee /tmp/jlink.log
    else
      JLinkExe -speed 15000 -if JTAG $JTAGCONF -device R7S721001 -CommanderScript /tmp/jlink_load.txt | tee /tmp/jlink.log
    fi
  else
      # == RZ/A2 ==
      JLinkExe -speed 15000 -if JTAG $JTAGCONF -device R7S921053VCBG -CommanderScript /tmp/jlink_load.txt | tee /tmp/jlink.log
  fi

  # Check to see if download we successful
  CHECK=`grep -m1 FAILED /tmp/jlink.log`
  CHECK2=`grep -m1 " Error:" /tmp/jlink.log`
  CHECK3=`grep -m1 "Cannot connect to target" /tmp/jlink.log`
  if [ "$CHECK" != "" ] ; then
    banner_red "$CHECK"
    exit 128
  fi
  if [ "$CHECK2" != "" ] ; then
    banner_red "$CHECK2"
    exit 128
  fi
  if [ "$CHECK3" != "" ] ; then
    banner_red "$CHECK3"
    exit 128
  fi

  echo "-----------------------------------------------------"
  echo -en "\tFile size was:\n\t"
  du -h $dlfile
  echo "-----------------------------------------------------"

  FILESIZE=$(cat $dlfile | wc -c)

  CHECK=`grep "Comparing flash" /tmp/jlink.log`
  if [ "$CHECK" != "" ] ; then
    exit 128
  fi

  SF_PROBE=""
  if [ "$QSPI" == "DUAL" ] ; then
    SF_PROBE=":1"
  fi

  ############ u-boot Programming ############
  CHECK=$(echo $dlfile | grep u-boot)
  if [ "$CHECK" != "" ] ; then
  echo "Example program operations:

# Rewrite u-boot (512 KB):
=> sf probe 0 ; sf erase 0 80000 ; sf write $ramaddr 0 80000
"
  exit 0
  fi

  ############ DTB Programming ############
  CHECK=$(echo $dlfile | grep dtb)
  if [ "$CHECK" != "" ] ; then
    SPI_ADDR=$(printf "%x\n" $(($DTB_ADDR-$UBOOT_ADDR)))
    echo "Example program operations:

# Program DTB (32 KB)
=> sf probe 0 ; sf erase $SPI_ADDR 40000 ; sf write $ramaddr $SPI_ADDR 8000
"
  exit 0
  fi
  ############ Kernel Programming ############
  CHECK=$(echo $dlfile | grep Image)
  if [ "$CHECK" != "" ] ; then
    # Determine SPI flash offset
    SPI_ADDR=$(printf "%x\n" $(($KERNEL_ADDR-$UBOOT_ADDR)))

    # Calculate how much you need to program (round up to next 1MB)
    SPI_SZ_P=$(printf "%d\n" $(($FILESIZE/0x100000 + 1)))
    SPI_SZ_MB=$(printf "%d\n" $(($SPI_SZ_P)))
    SPI_SZ_P=$(printf "%x\n" $(($SPI_SZ_P*0x100000)))

    echo -e "Example program operations:\n"

    # Make adjustments for Dual SPI flash
    if [ "$QSPI" == "DUAL" ] ; then
      SPI_ADDR=$(printf "%x\n" $((0x$SPI_ADDR/2))) # SPI address is half
      SPI_SZ_E=$(printf "%x\n" $((0x$SPI_SZ_P/2))) # Erase size is half of program size
    else
      SPI_SZ_E=$SPI_SZ_P
    fi

        echo "# Program Kernel (${SPI_SZ_MB}MB, $QSPI SPI flash)
  => sf probe 0$SF_PROBE ; sf erase $SPI_ADDR $SPI_SZ_E ; sf write $ramaddr $SPI_ADDR $SPI_SZ_P"

    exit 0
  fi

  ############ Rootfs Programming ############
  CHECK=$(echo $dlfile | grep rootfs)
  if [ "$CHECK" != "" ] ; then
    # Determine SPI flash offset
    SPI_ADDR=$(printf "%x\n" $(($ROOTFS_ADDR-$UBOOT_ADDR)))

    # Calculate how much you need to program (round up to next 1MB)
    SPI_SZ_P=$(printf "%d\n" $(($FILESIZE/0x100000 + 1)))
    SPI_SZ_MB=$(printf "%d\n" $(($SPI_SZ_P)))
    SPI_SZ_P=$(printf "%x\n" $(($SPI_SZ_P*0x100000)))
    echo -e "Example program operations:\n"

    # Make adjustments for Dual SPI flash
    if [ "$QSPI" == "DUAL" ] ; then
      SPI_ADDR=$(printf "%x\n" $((0x$SPI_ADDR/2))) # SPI address is half
      SPI_SZ_E=$(printf "%x\n" $((0x$SPI_SZ_P/2))) # Erase size is half of program size
    else
      SPI_SZ_E=$SPI_SZ_P
    fi

        echo "# Program Rootfs (${SPI_SZ_MB}MB, $QSPI SPI flash)
  => sf probe 0$SF_PROBE ; sf erase $SPI_ADDR $SPI_SZ_E ; sf write $ramaddr $SPI_ADDR $SPI_SZ_P"

    if [ "$BOARD" == "rskrza1" ] && [ $FILESIZE -ge $((0x2000000)) ] ; then	# >= 32MB?
      echo "Are you sure you have enough SDRAM space? The RSK only has 32MB of SDRAM."
    fi
    echo -e "\n"
    exit 0
  fi

fi

# Create output build directory
if [ ! -e $OUTDIR ] ; then
  mkdir -p $OUTDIR
fi

##### Check if we have all the host tools we need for menuconfig #####
if [ "$2" == "menuconfig" ] ; then
  CHECK=$(which ncurses5-config)
  if [ "$CHECK" == "" ] ; then
    banner_red "ncurses is not installed"
    echo -e "You need the package ncurses installed in order to use menuconfig."
    echo -e "In Ubuntu, you can install it by running:\n\tsudo apt-get install ncurses-dev\n"
    echo -e "Existing build script.\n"
    exit 1
  fi
fi

###############################################################################
# build kernel
###############################################################################
if [ "$1" == "kernel" ] || [ "$1" == "k" ] ; then
  banner_yellow "Building kernel"

  check_for_toolchain

  if [ "$2" == "" ] ; then
    echo " "
    echo "What do you want to build?"
    echo "For example:  (case sensitive)"
    echo " Traditional kernel:  ./build.sh kernel uImage"
    echo "         XIP kernel:  ./build.sh kernel xipImage"
    echo "  Kernel config GUI:  ./build.sh kernel menuconfig"
    exit 0
  fi

  if [ "$2" == "uImage" ] ; then
    CHECK=$(which mkimage)
    if [ "$CHECK" == "" ] ; then
      banner_red "mkimage is not installed"
      echo -e "You need the program mkimage installed in order to build a kernel uImage."
      echo -e "In Ubuntu, you can install it by running:\n\tsudo apt-get install u-boot-tools\n"
      echo -e "Existing build script.\n"
      exit 0
    fi
  fi

  cd $OUTDIR

  # install linux-4.19
  if [ ! -e linux-4.19 ] ;then

    # Download linux-4.19

    # We need git to be installed
    CHECK=`which git`
    if [ "$CHECK" == "" ] ; then
      banner_red "git is not installed"
      echo -e "You need git in order to download the kernel"
      echo -e "In Ubuntu, you can install it by running:\n\tsudo apt-get install git\n"
      echo -e "Existing build script.\n"
      exit 1
    fi


    # Download the current repository
    git clone https://github.com/renesas-rz/rza_linux-4.19.git linux-4.19
  fi

  cd linux-4.19

  # Do the build operation
  IMG_BUILD=0
  XIPCHECK=`grep -s CONFIG_XIP_KERNEL=y .config`

  if [ "$2" == "uImage" ] ;then
    IMG_BUILD=1

    if [ ! -e .config ] || [ "$XIPCHECK" != "" ]; then
      # Need to configure kernel first
      make ${BOARD}_defconfig
    fi

    # To build a uImage, you need to specify LOADADDR.
    if [ "$BOARD" == "rskrza1" ] || [ "$BOARD" == "genmai" ] || [ "$BOARD" == "ylcdrza1h" ] ; then
      MY_LOADADDR='LOADADDR=0x08008000'
    fi
    if [ "$BOARD" == "streamit" ] ; then
      MY_LOADADDR='LOADADDR=0x0C008000'
    fi
    if [ "$3" != "" ] ; then
      MY_LOADADDR=LOADADDR=$3
    fi
    if [ "$MY_LOADADDR" == "" ] ; then
      banner_red "Missing load address (LOADADDR)"
      echo "When building a uImage, you need to specify the load address on the kernel"
      echo "build command line (make LOADADDR=0x55555555 uImage) so u-boot"
      echo "will know where in RAM to decompress the kernel to."
      echo "Please specify that address according to your SDRAM location."
      echo "Examples:"
      echo "   ./build.sh kernel uImage 0x08008000"
      echo "   ./build.sh kernel uImage 0x0C008000"
      exit 128
    fi
  fi
  if [ "$2" == "xipImage" ] ;then
    IMG_BUILD=2
    if [ ! -e .config ] || [ "$XIPCHECK" == "" ]; then
      # Need to configure kernel first
      make ${BOARD}_xip_defconfig
    fi

    # re-configure kernel if we changed target board
    #CHECK=$(grep -i CONFIG_MACH_${BOARD}=y .config )
    #if [ "$CHECK" == "" ] ; then
    #  echo "Reconfiguring for new board..."
    #  make ${BOARD}_xip_defconfig
    #fi

    # LOADADDR is not needed when building a xipImage
    MY_LOADADDR=
  fi

  if [ "$IMG_BUILD" != "0" ] ; then
    # NOTE: Adding "LOCALVERSION=" to the command line will get rid of the
    #       plus sign (+) at the end of the kernel version string. Alternatively,
    #       we could have created a empty ".scmversion" file in the root.
    # NOTE: We have to make the Device Tree Blobs too, so we'll add 'dtbs' to
    #       the command line
    echo -e "make $MY_LOADADDR LOCALVERSION= -j$BUILD_THREADS $2 dtbs\n"
    make $MY_LOADADDR LOCALVERSION= -j$BUILD_THREADS $2 dtbs
    RETURN_CODE=$?

    if [ ! -e vmlinux ] ; then
      # did not build, so exit
      banner_red "Kernel Build failed. Exiting build script."
    else
      banner_green "Kernel Build Successful"
    fi
  else
      # user wants to build something special
      banner_yellow "Custom Build"
      echo -e "make -j$BUILD_THREADS $2 $3 $4\n"
      make -j$BUILD_THREADS $2 $3 $4
      RETURN_CODE=$?
  fi

  exit $RETURN_CODE
fi

###############################################################################
# build u-boot
###############################################################################
if [ "$1" == "u-boot" ] || [ "$1" == "u" ] ; then
  banner_yellow "Building u-boot"

  check_for_toolchain

  cd $OUTDIR

  # install u-boot-2017.05
  if [ ! -e u-boot-2017.05 ] ; then

    # We need git to be installed
    CHECK=`which git`
    if [ "$CHECK" == "" ] ; then
      banner_red "git is not installed"
      echo -e "You need git in order to download the kernel"
      echo -e "In Ubuntu, you can install it by running:\n\tsudo apt-get install git\n"
      echo -e "Existing build script.\n"
      exit 1
    fi

    # Download the latest head of the repository
    git clone https://github.com/renesas-rz/rza_u-boot-2017.05.git u-boot-2017.05
  fi

  cd u-boot-2017.05

  # Patch u-boot
  if [ ! -e include/configs/rskrza1.h ] ;then
    # Combine all the patches, then patch at once
    cat $ROOTDIR/patches-uboot/* > /tmp/uboot_patches.patch
    patch -p1 -i /tmp/uboot_patches.patch

  fi

  # Configure u-boot
  if [ ! -e .config ] ;then
    make ${BOARD}_config
  fi

  # re-configure u-boot if we changed target board
  #CHECK=$(grep CONFIG_SYS_BOARD .config | grep $BOARD)
  #if [ "$CHECK" == "" ] ; then
  #  echo "Reconfiguring for new board..."
  #  make ${BOARD}_config
  #fi

  # Build u-boot
  if [ "$2" == "" ] ;then

    # default build
    make
    RETURN_CODE=$?

    if [ ! -e u-boot.bin ] ; then
      # did not build, so exit
      banner_red "u-boot Build failed. Exiting build script."
    else
      banner_green "u-boot Build Successful"
    fi
  else
      # user wants to build something special
      banner_yellow "Custom Build"
      echo -e "make $2 $3 $4\n"
      make $2 $3 $4
      RETURN_CODE=$?
  fi

  exit $RETURN_CODE

fi

###############################################################################
# build buildroot
###############################################################################
if [ "$1" == "buildroot" ]  || [ "$1" == "b" ] ; then
  banner_yellow "Building buildroot"

  cd $OUTDIR

  if [ ! -e br_version.txt ] ; then
    echo "What version of Buildroot do you want to use?"
    echo "1. buildroot-2017.02 (Long Term Support)"
    echo "2. buildroot-2018.02 (Long Term Support)"
    echo -n "(select number)=> "
    read ANSWER
    if [ "$ANSWER" == "1" ] ; then
      echo "export BR_VERSION=2017.02" > br_version.txt
    elif [ "$ANSWER" == "2" ] ; then
      echo "export BR_VERSION=2018.02" > br_version.txt
    else
      echo "ERROR: \"$ANSWER\" is an invalid selection!"
      exit 128
    fi
    source br_version.txt
  fi

  # Download buildroot-$BR_VERSION.tar.bz2
  if [ ! -e buildroot-$BR_VERSION.tar.bz2 ] ;then
    wget http://buildroot.uclibc.org/downloads/buildroot-$BR_VERSION.tar.bz2
    if [ "$?" != "0" ] ; then
      exit # could not download
    fi
  fi

  # extract buildroot-$BR_VERSION
  if [ ! -e buildroot-$BR_VERSION/README ] ;then
    echo "extracting buildroot..."
    tar -xf buildroot-$BR_VERSION.tar.bz2
    if [ "$?" != "0" ] ; then
      exit # could not extract
    fi
  fi

  if [ ! -e buildroot-$BR_VERSION ] ;then
      # something went wrong
      exit
  fi

  cd buildroot-$BR_VERSION

  # Apply Renesas Buildroot patches that have not been applied yet.
  # Buildroot LTS update patches were made doing this in the Buildroot mainline repoistory:
  #   git diff 2018.02   2018.02.1 > br_2018.02.0_to_2018.02.1.patch
  #   git diff 2018.02.1 2018.02.2 > br_2018.02.1_to_2018.02.2.patch
  #   git diff 2018.02.2 2018.02.3 > br_2018.02.2_to_2018.02.3.patch
  if [ ! -e .applied_renesas_patches ] ; then
    echo "# These patches have already been applied" > .applied_renesas_patches
  fi
  # this ${i##*/} means just the file name, not the full path
  for i in $ROOTDIR/patches-buildroot/buildroot-$BR_VERSION/00*.patch ; do
    grep "${i##*/}" .applied_renesas_patches > /dev/null
    if [ "$?" != "0" ] ; then
      banner_yellow "Applying Buildroot patch ${i##*/}"
      patch -p1 -i $i
      echo "${i##*/}" >> .applied_renesas_patches
      sleep 1
    fi
  done

  if [ ! -e output ] ; then
    mkdir -p output
  fi

  # Copy in our rootfs_overlay directory
  if [ ! -e output/rootfs_overlay ] ; then
    cp -a $ROOTDIR/patches-buildroot/rootfs_overlay output
  fi

 # Patch and Configure Buildroot for RZ/A
  if [ ! -e configs/rza_defconfig ]; then

    # Ask the user if they want to use the glib based Linaro toolchain
    # or build a uclib toolchain from scratch.
    banner_yellow "Toolchain selection"
    #echo -e "\n\n[ Toolchain selection ]"
    echo -e "What toolchain and C Library do you want to use for building applications?"
    echo ""
    echo "By default, we suggest the Linaro pre-built toolchain with hardware float"
    echo "support and glib C Libraries."
    echo ""
    echo "It is also possible to configure Buildroot to download and build from source"
    echo "a uClibc or musl based toolchain. Note that while uClibc or musl produces a smaller binary"
    echo "footprint, some open souce applications are not compatible. (musl is more compatible than uClibC)"
    echo ""
    echo "Finaly, you may also configure Buildroot to use a toolchain that is already"
    echo "install on your machine."
    echo ""
    echo "What would you like to do?"
    echo "  1. Use the default Linaro toolchain (recommended)"
    echo "  2. Install Buildroot and then let me decide in the configuration menu (advanced)"
    echo -n "=> "
    for i in 1 2 3 ; do
      echo -n " Enter your choice (1 or 2): "
      read TC_CHOICE
      if [ "$TC_CHOICE" == "1" ] ; then break; fi
      if [ "$TC_CHOICE" == "2" ] ; then break; fi
      TRY=$i
    done

    if [ "$TRY" == "3" ] ; then
      echo -e "\nI give up! I have no idea what you want to do."
      exit 128
    fi

    # Copy in our default Buildroot config for the RSK
    # NOTE: It was made by running this inside buildroot
    #   make savedefconfig BR2_DEFCONFIG=../../patches-buildroot/rza_defconfig
    # or rather
    #   ./build.sh buildroot savedefconfig BR2_DEFCONFIG=../../patches-buildroot/rza_defconfig
    #          NOTE: 'BR2_PACKAGE_JPEG=y' has to be manually added before
    #                'BR2_PACKAGE_JPEG_TURBO=y' (a bug in savedefconfig I assume)
    #
    cp -a $ROOTDIR/patches-buildroot/buildroot-$BR_VERSION/*_defconfig configs/

    # Just build the minimum file systerm. Users can go back and add more if they want to later.
    make rza_defconfig

    if [ "$TC_CHOICE" == "2" ] ; then

      # User wants to select the toolchain themselves.
      make menuconfig
      RETURN_CODE=$?

      echo ""
      echo "======================================================================="
      echo ""
      echo " If everything is how you like it, you can now build your system by running:"
      echo "     ./build.sh buildroot"
      echo ""
      echo " Or you can add additional SW packages by running:"
      echo "     ./build.sh buildroot menuconfig"
      echo ""

      exit $RETURN_CODE

    fi
  fi

  # Trim buildroot temporary build files since they are not longer needed
  if [ "$2" == "trim" ] ;then

    echo "This will remove a good portion of intermediate build files under"
    echo "under the output/build directory since after they are build, they don't"
    echo "really serve much purpose anymore."
    echo ""
    echo -n "Continue? [y/N] "
    read ANS
    if [ "$ANS" != "y" ] || [ "$ANS" == "Y" ] ; then
      exit 0
    fi

    echo "First, we'll remove all the build files from output/build/host-* because once"
    echo "they are built and copied to output/host, there is not more use for them".
    echo "We only need to kee the .stamp_xxx files to tell Buildroot that they've already"
    echo "been built."
    echo -n "Press return to continue..."
    echo TRIMMING:
    TOTAL=`du -s -h -c $(ls -d output/build/host-*) | grep total`
    for HOST_DIR in $(ls -d output/build/host-*)
    do
      du -s -h $HOST_DIR
      find $HOST_DIR -type f ! -name '.stamp_*' -delete
      find $HOST_DIR -type l -delete
      rm -r -f `find $HOST_DIR -type d -name ".*"`
      find $HOST_DIR -type d -empty -delete
    done
    echo ""
    echo -n $TOTAL
    echo " deleted"

    echo ""
    echo "Next we will look at packages that you have already built and installed in your root"
    echo "file system. After the binaries have been copied to output/target and build libraries"
    echo "have been copied to output/staging, there is no more use for the files under output/build."
    echo ""
    echo "HINT: Just pressing enter defaults to 'y' "
    echo ""
    for BUILD_DIR in $(ls -d output/build/*)
    do
      #echo $BUILD_DIR
      #echo "${BUILD_DIR:13}"
      BUILD_DIR_NAME=${BUILD_DIR:13:18}

      # ignore the host- directories
      if [ "${BUILD_DIR_NAME:0:5}" == "host-" ] ; then
        continue
      fi

      # skip busybox because that is one that can be reconfigured
      # and reinstalled even after initial built
      if [ "${BUILD_DIR_NAME:0:7}" == "busybox" ] ; then
        continue
      fi

      # skip toolchain
      if [ "${BUILD_DIR_NAME:0:9}" == "toolchain" ] ; then
        continue
      fi

      # skip skeleton
      if [ "${BUILD_DIR_NAME:0:8}" == "skeleton" ] ; then
        continue
      fi

      # ignore directories without stamps
      if [ ! -e $BUILD_DIR/.stamp_target_installed ] ; then
        continue
      fi

      echo -n "Clean $BUILD_DIR_NAME ? [ Y/n ]: "
      read ANS
      if [ "$ANS" == "" ] || [ "$ANS" == "y" ] || [ "$ANS" == "Y" ] ; then

        du -s -h $BUILD_DIR
        find $BUILD_DIR -type f ! -name '.stamp_*' -delete
        find $BUILD_DIR -type l -delete
        rm -r -f `find $BUILD_DIR -type d -name ".*"`
        find $BUILD_DIR -type d -empty -delete
      fi
    done

    exit 0
  fi

  # Build Buildroot
  if [ "$2" == "" ] ;then

    # default build
    make
    RETURN_CODE=$?

    if [ ! -e output/images/rootfs.tar ] ; then
      # did not build, so exit
      banner_red "Buildroot Build failed. Exiting build script."
    else
      banner_green "Buildroot Build Successful"
    fi
  else
      # user wants to build something special
      banner_yellow "Custom Build"
      echo -e "make $2 $3 $4 $5\n"
      make $2 $3 $4 $5
      RETURN_CODE=$?
  fi

  exit $RETURN_CODE
fi

###############################################################################
# build axfs
###############################################################################
if [ "$1" == "axfs" ] ; then
  banner_yellow "Building axfs"

  if [ -e output/buildroot-$BR_VERSION/output/images/rootfs.axfs ] && [ "$2" == "" ]; then
    banner_red "AXFS image already built"
    echo "Now, AXFS images are now automatically built by Buildroot".
    echo "So, there is no more need for this command anymore".
    echo "You can find your AXFS image here:"
    echo "  output/buildroot-$BR_VERSION/output/images/rootfs.axfs"
    exit 0
  fi

  # Just use the pre-build versions
  CHECK=$(uname -m)
  if [ "$CHECK" == "x86_64" ] ; then
    # 64-bit OS
    MKFSAXFS=$ROOTDIR/axfs/mkfs.axfs.64
  else
    # 32-bit OS
    MKFSAXFS=$ROOTDIR/axfs/mkfs.axfs.32
  fi

  cd $OUTDIR/axfs

  if [ "$2" != "" ] ; then
    if [ "$3" == "" ] ; then
    echo ""
    banner_red "Missing output file name"
    echo "Usage: ./build.sh axfs {full-path-to-directory} {output-filename}"
    exit 128
   fi
    $MKFSAXFS -s -a $2 $3
    RETURN_CODE=$?

    if [ -e $3 ] ; then
      banner_green "axfs Build Successful"
      echo -n " File: "
      echo `ls $(pwd)/$3`
    fi
    exit $RETURN_CODE
  fi


  # NOTE: If the 's' attribute is set on busybox executable (which it is by default when
  #   Buildroot builds it), and the file owner is not 'root' (which it will not be because
  #   you were not root when you ran Buildroot) you can't boot and will just keep getting
  #   a "Permission denied" message after the file system is mounted"
  chmod a-s $BUILDROOT_DIR/output/target/bin/busybox

  $MKFSAXFS -s -a ../buildroot-$BR_VERSION/output/target rootfs.axfs.bin

  if [ ! -e rootfs.axfs.bin ] ; then
    # did not build, so exit
    banner_red "axfs Build failed. Exiting build script."
    exit 1
  else
    banner_green "axfs Build Successful"
    echo -e "You can find your AXFS image to flash here:"
    echo -e "\t$(pwd)/rootfs.axfs.bin"
  fi

  exit 0
fi

###############################################################################
# update
###############################################################################
if [ "$1" == "update" ] ; then
  banner_yellow "repository update"

  if [ "$2" == "" ] ; then
    echo -e "Update:"
    echo -e "This command will 'git pull' the latest code from the github repositories."
    echo -e "Any changes you have made will be save and re-applied after the updated."
    echo -e "Basically, we will do the following:"
    echo -e "  git stash      # save current changes"
    echo -e "  git pull       # download latest version"
    echo -e "  git stash pop  # re-apply saved changes"
    echo -e ""
    echo -e "  ./build.sh update b   # updates bsp build scripts"
    echo -e "  ./build.sh update u   # updates uboot source"
    echo -e "  ./build.sh update k   # updates kernel source "
    echo -e ""
    exit 0
  fi

  if [ "$2" == "b" ] ; then
    git stash
    git pull
    git stash pop
    exit 0
  fi

  if [ "$2" == "k" ] ; then
    if [ ! -e output/linux-4.19 ] ; then
      cd output
      git clone https://github.com/renesas-rz/rza_linux-4.19.git linux-4.19
    else
      cd output/linux-4.19
      git stash
      git checkout master
      git pull
      git stash pop
    fi
    exit 0
  fi

  if [ "$2" == "u" ] ; then
    if [ ! -e output/u-boot-2017.05 ] ; then
      cd output
      git clone https://github.com/renesas-rz/rza_u-boot-2017.05.git u-boot-2017.05
    else
      cd output/u-boot-2017.05
      git stash
      git checkout master
      git pull
      git stash pop
    fi
    exit 0
  fi
fi

