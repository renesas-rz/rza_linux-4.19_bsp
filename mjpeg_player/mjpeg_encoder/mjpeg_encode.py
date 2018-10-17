import os
import sys
from glob import iglob
import shutil
import struct
import subprocess
import time

def get_frame_rate(filename):
    if not os.path.exists(filename):
        sys.stderr.write("ERROR: filename %r was not found!" % (filename,))
        return -1         
    out = subprocess.check_output(["ffprobe",filename,"-v","0","-select_streams","v","-print_format","flat","-show_entries","stream=r_frame_rate"])
    print(out)
    rate = out.split(b'=')[1].strip()[1:-1].split(b'/')
    if len(rate)==1:
        return float(rate[0])
    if len(rate)==2:
        return float(rate[0])/float(rate[1])
    return -1


#parser = argparse.ArgumentParser()
#parser = argparse.ArgumentParser(description='Process some integers.')
#parser.add_argument('--foo', default=42, help='foo help')
#args = parser.parse_args()

# defaults
#bitrate="5000k"
bitrate="200k"
width="640"
height="480"
skip=0
max_length=-1
input_file=""
output_file=""
keep_jpegs = 0

missing_args = 0

# Parse arguments
total_args = len(sys.argv)
if total_args == 1:
        missing_args = 1

skip_next = 0
for i in range(1,total_args):
    #print(i)
    #print(sys.argv[i])
    if skip_next:
        skip_next = 0
        continue
    if sys.argv[i] == "-s":
        skip = int(sys.argv[i + 1])
        skip_next = 1
        continue

    if sys.argv[i] == "-l":
        max_length = int(sys.argv[i + 1])
        skip_next = 1
        continue

    if sys.argv[i] == "-b":
        bitrate = sys.argv[i + 1]
        skip_next = 1
        continue

    if sys.argv[i] == "-k":
        keep_jpegs = 1
        continue

    # All the is the input
    if (i + 4) != total_args:
        print("ERROR: Missing arguments")
        print(i + 4)
        print(total_args)
        missing_args = 1
        break

    # The rest are all manditory
    input_file = sys.argv[i]
    output_file = sys.argv[i + 1]
    width = sys.argv[i + 2]
    height = sys.argv[i + 3]
    break

if missing_args:
        print("  usage: mjpeg_encode.py [-s x] [-l x] [input] [output] [width] [height]\n")
        print("  -s X: Skip X number of jpeg images at the beginning\n")
        print("  -l X: Encode only maximum X number of jpeg images\n")
        print("  -b X: bitrate image quality of JPEG file(1k-20000k). Default is 200k\n")
        print("  -k  : Keep intermediate JPEG files (don't delete them when done)\n")
        sys.exit(-1)

# Create a temp directory to hold our temporary JPEG images
if os.name == 'nt': # Windows
    print("Running in Windows\n")
    tempdir="_tmp"
else:   # other (linux)
    print("running in Linux\n")
    tempdir="/tmp/mjpeg_encode"

if os.path.isdir(tempdir):
    print("Removing previous files...\n")
    shutil.rmtree(tempdir)
    time.sleep(1)
os.mkdir(tempdir)

# print args
print("bitrate = " + str(bitrate) )
print("width = " + str(width) )
print("height = " + str(height) )
print("skip = " + str(skip) )
print("max_length = " + str (max_length) )
print("input = " + input_file )
print("output = " + output_file )

# Create string for scaling ("scale=640:480")
scale = "scale=" + width + ":" + height

if os.path.isfile(input_file):
        #ffmpeg_result = subprocess.call(["ffmpeg", "-i", input_file, "-vf", "scale=320:240,drawtext=fontsize=20:x=10:y=10:fontfile=\"C:\\\Windows\\\Fonts\\\consola.ttf\":text=%{pts}:box=1", "-b:v", bitrate, "_tmp\%08d.jpg"])
        #ffmpeg_result = subprocess.call(["ffmpeg", "-i", input_file, "-vf", "crop=960:720:160:0" , scale, "-b:v", bitrate, "_tmp\%08d.jpg"])
        ffmpeg_result = subprocess.call(["ffmpeg", "-i", input_file, "-vf", scale, "-b:v", bitrate, tempdir + "/%08d.jpg"])
        if ffmpeg_result:
                print("encoding failed\n")
                sys.exit(-1)
        framerate = get_frame_rate(input_file)
        #print(framerate)
        if framerate < 0.5 or framerate > 90.0:
                print("Could not retrieve framerate from stream\n")
                sys.exit(-1)
else:
        print("Input file not found\n")
        sys.exit(-1)

if os.path.isfile(output_file):
        os.remove(output_file)
outfile = open(output_file, 'wb')
outfile.write(struct.pack('1f', framerate))

count = 0
for fname in iglob(os.path.join('./_tmp', '*.jpg')):
        if skip:
            skip -= 1
            continue
	jpeg = open(fname, 'rb')
	outfile.write(struct.pack('1I', os.stat(fname).st_size))
	outfile.write(jpeg.read())
	jpeg.close()
	count += 1
        if max_length != -1:
            if count > max_length:
                    break
outfile.write(struct.pack('1I', 0xFFFFFFFF))
outfile.close()
if keep_jpegs != 1:
    shutil.rmtree(tempdir)
