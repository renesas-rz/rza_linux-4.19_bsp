@echo off


rem Command line options for python script
rem -----------------------------------------
rem usage: mjpeg_encode.py [-s x] [-l x] [input] [output] [width] [height]
rem        -s X: Skip X number of jpeg images at the beginning
rem        -l X: Encode only maximum X number of jpeg images 
rem        -b X: bitrate image quality of JPEG file(1k-20000k). Default is 200k 
rem        -k  : Keep intermediate JPEG files (don't delete them when done) 

rem C:\Python27\python mjpeg_encode.py -b 1000k COSTA-RICA-720p.mp4 COSTA-RICA-800x480.mjpeg 800 480

rem C:\Python27\python mjpeg_encode.py -l 300 -k -b 1000k Zootopia_Trailer_720p.mp4 Zootopia_Trailer_480x272.mjpeg 480 272


pause
