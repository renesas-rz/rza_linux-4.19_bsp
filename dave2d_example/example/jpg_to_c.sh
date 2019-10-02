#!/bin/bash

IN_FILE=bg_800x480.jpg
OUT_FILE=bg_800x480.c
TMP_FILE=/tmp/tmp.c

# declare the array
echo -en "\n\nconst unsigned char bg_jpeg[] = { \n " > $TMP_FILE

# convert the binary to ASCII hex
hexdump -v -e '16/1 "0x%02X, ""\n"" "' $IN_FILE >> $TMP_FILE

# get rid of the extra that hexdump adds to fill out the line
sed -i 's/ 0x  ,//g' $TMP_FILE

# Get rid of that last comma at the end
cat $TMP_FILE | head -c -3 > $OUT_FILE
rm $TMP_FILE

# close the array
echo " };" >> $OUT_FILE

