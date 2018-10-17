This application use the JCU in order to decode either a single JPEG image, or multiple JPEG images directly to the LCD screen.


--------------------------------------------------
MJPEG File
--------------------------------------------------
The format for the mjpeg file is non-standard. The reason is that there is no standard MJPEG file format. Traditionally, many MJPEG files that are used for things such as security camera are simple a JPEG file with multile complete JPEG files one after the other in a file.
However, there is no frame rate.

Additionally, in a JPEG file header, there is no 'file size' attribute, so you have to keep reading the file until you get to the end of it.
That is not very efficient.

The file format for this demo is as follows:

4 bytes: frames per second
4 bytes: file size of next JPEG image
x bytes: JPEG image
4 bytes: file size of next JPEG image
x bytes: JPEG image
4 bytes: file size of next JPEG image
x bytes: JPEG image
etc...

--------------------------------------------------
Installing FFMPEG on your Host PC
--------------------------------------------------

First you must acquire FFMPEG in order to covert a .mp4 file into individual JPEG files.

For Windows, you can download from here. This will simply be a .zip file (no installer needed). Please place files "ffmpeg.exe" and "ffprobe.exe" in the same directory that you run the python script from.

    http://ffmpeg.zeranoe.com/builds/

For Linux:
    $ sudo apt-get install ffmpeg


--------------------------------------------------
Creating the Renesas mjpeg file
--------------------------------------------------
When creating a motion JPEG file, please use the python script provided.

Command line options for python script
  usage: mjpeg_encode.py [-s x] [-l x] [input] [output] [width] [height]
       -s X: Skip X number of jpeg images at the beginning
       -l X: Encode only maximum X number of jpeg images 
       -b X: bitrate image quality of JPEG file(1k-20000k). Default is 200k 
       -k  : Keep intermediate JPEG files (don't delete them when done) 

An example would be:
  C:\Python27\python mjpeg_encode.py -l 300 -k -b 1000k my_movie.mp4 my_movie_480x272.mjpeg 480 272

  
NOTE: The RZ/A user application will require a buffer to read each JPEG image into in order to pass that image to the HW JPEG engine. when the python script runs, it will keep track of what the larges JPEG image was that it encoded and will print that information out at the end of the script. This will then let you know what is the minimum buffer size you need to reserve for JPEG decoding.

--------------------------------------------------
Reviewing the mjpeg file
--------------------------------------------------
Even though the .mjpeg file created by the python script contains extra data that does not follow and JPEG standard, we have found that "SMPlayer" for windows seems to be able to decode and play the .mjpeg file. Since it doesn't know the framerate, it uses the default rate of 25 fps.


--------------------------------------------------
Customizing RZ/A Linux Application 
--------------------------------------------------
When the application runs, it will open the mjpeg file passed on the command line and feed the JPEG images one at at time to the HW JPEG engine in the RZ/A at the frame rate specified in the custom .mjpeg file.
The application will detect the size of the currently attached LCD by querying /dev/fb0. If the resolution of the MJPEG video you plan showing is less than the current screen, then, you can use the x,y offset parameters in order to specify where on the screen you would like the video to be played.

This application assumes that external SDRAM is being used and that the internal RAM is only being used for the LCD. Basically this means that you booted the system using "run xsa_boot" or "run s_boot".

Since the HW JPEG engine requires the complete JPEG image to be in RAM and passed to the HW JPEG, the application will assume that the internal RAM can be used. The address for this area is assume to be 2MB after the start of the allocated LCD frame buffer. Also, the applicaiton assumes that any given compressed JPEG image that it reads from the .mjpeg file will be less then 128KB.
Also note that the application will double buffer the JPEG images being passed to the HE JPEG engine, so a total RAM buffer of 2x the max image size needs to be allocated (2x128KB = 512KB).

You may adjust these settings by editing these lines in the application source code. No modifications are needed to the kernel code itself.

#define	JPEG_WB_PHYS	lcd_fb_phys + 0x200000	// Physical RAM address to hold JPEG file to decompress
#define JPEG_IMG_SIZE	(128*1024)		// The max size of a .jpg file to decode
#define JPEG_WB_SIZE	(JPEG_IMG_SIZE*2)	// Size of total work buffer area (double buffered)


--------------------------------------------------
Booting the RSK board with 'run xsa_boot'
--------------------------------------------------
Some boards (such as the STream it) by default use external SDRAM when running and XIP Linux kernel. In that case, you can simply boot the board using "run xsa_boot".
However, some boards default to only using internal RAM for the XIP Linux system such as the RZ/A1H RSK board.
Please use the following steps to change the configuration of the RSK board to use external SDRAM instead.
$ ./build.sh kernel rskrza1_xip_defconfig
$ ./build.sh kernel menuconfig
   Now change the value of the very first menuc item from 0x20000000 to 0x08000000,
   then exit and save.
$  ./build.sh kernel xipImage
$  ./build.sh kernel dtbs

Then reprogram the new xipImage into your board.

Now you will boot using "run xsa_boot" and your system will have 32MB of system RAM. The LCD frame buffer will still reside in internal RAM.


--------------------------------------------------
Building the RZ/A Linux Application 
--------------------------------------------------
Front the BSP directory, enter the following commands
	$ ./build.sh env
	# export ROOTDIR=$(pwd) ; source ./setup_env.sh
	$ cd hw_jpeg
	$ make
	$ make install
	$ cd ..
	$ ./build.sh buildroot

Now reprogram your root file system.
The application "mjpeg_player" will exist in /root/bin/mjpeg_player.


--------------------------------------------------
Running the RZ/A Linux Application 
--------------------------------------------------
Since the application should be installed to /root/bin/, you can just enter mjpeg_player on the command line to execute the program. with no arguments, it should output the usage.
The .mjpeg file can be located anywhere (QSPI Flash, USB stick, SD card, etc..)
You could also copy mjpeg_player to a USB Flash stick and just run it from there.


--------------------------------------------------
Future work
--------------------------------------------------
Some ideas on improving this demonstration
 * Instead of putting the size of the next JPEG image as a 4 byte address in the stream, place this information in the form of text in the COM ("comment") marker in the JPEG header. For example: FF FE 08 '2' '3' '4' '1' 00.
 See https://en.wikipedia.org/wiki/JPEG#Syntax_and_structure
 Then you just need to read in a fixed amount of data in order to find out what the rest
 of the file size it. The benefit is that then the .mjpeg files will not have those extra 4 bytes in between images.
