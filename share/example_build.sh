#!/bin/bash
get_char()
{
  SAVEDSTTY=`stty -g`
  stty -echo
  stty raw
  dd if=/dev/tty bs=1 count=1 2> /dev/null
  stty -raw
  stty echo
  stty $SAVEDSTTY
}

if [ -f "$0" ]; then
echo -e "$0"
echo 'Please press any key to continue...'
fi

echo FULL_CURRENT_PATH = "$FULL_CURRENT_PATH"
echo CURRENT_DIRECTORY = "$CURRENT_DIRECTORY"
echo FILE_NAME = "$FILE_NAME"
echo NAME_PART = "$NAME_PART"
echo EXT_PART = "$EXT_PART"
echo CURRENT_LINESTR = "$CURRENT_LINESTR"
echo CURRENT_SELSTR = "$CURRENT_SELSTR"
echo NUM_SELSTR = "$NUM_SELSTR"
gcc --version
g++ "$*" -g -o "/tmp/$NAME_PART.out"
if [ "$?" == "0" ]; then
chmod +x "/tmp/$NAME_PART.out"
"/tmp/$NAME_PART.out"
fi
get_char
# remove execute file
rm -f "/tmp/$NAME_PART.out" 2>/dev/null
# remove temp file
rm -f "$*" 2>/dev/null
