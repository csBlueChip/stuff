#!/bin/bash

OF=orig-full.keymap
OC=orig-compose.keymap
HC=hex-compose.keymap

echo "# Dump current maps:"
echo "    ${OF}" ; [[ -f ${OF} ]] || dumpkeys     >"${OF}" || exit 1
echo "    ${OC}" ; [[ -f ${OC} ]] || dumpkeys -d  >"${OC}" || exit 2

# So. It turns out the kernel limits the number of compose keys to 256 [qv. MAX_DIACR]
# ...and `loadkeys` blocks any compose which resolves to a NUL ...So ^@ FTW

echo "# Generate hex compose map"
echo "    ${HC}"
for i in `seq 1 255` ; do
	printf -v x "%02x" $i
	h=${x:0:1}
	l=${x:1:1}
#	printf "compose '%s' '%s' to 0x%s\n" ${h^^} ${l^^} ${x}  # uppercase
	printf "compose '%s' '%s' to 0x%s\n" ${h,,} ${l,,} ${x}  # lowecase
done  >${HC}

echo "* Compose Key:"
echo "    Try ^. or ask sudo dpkg-reconfigure keyboard-configuration"
echo "* Enable:"
echo "    kbd_mode -a"
echo "    loadkeys $(realpath ${HC})"
echo "* Test:"
echo "     showkey -a"
echo "* Restore:"
echo "    kbd_mode -u"
echo "    loadkeys $(realpath ${OC})"
