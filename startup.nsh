echo -off
echo Loading UEFI drivers for zoidberg kernel...
load fs0:\EFI\BOOT\SimpleThread.efi

echo Mounting zoidberg initrd...
map initrd fs1:

mode 100 31
echo Loading zoidberg kernel...
fs0:\EFI\BOOT\kernel.efi initrd=fs0:\EFI\BOOT\initrd.img

