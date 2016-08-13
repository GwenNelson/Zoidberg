echo -off
cls
mode 100 31
fs0:
cd fs0:\EFI\BOOT

echo Loading UEFI drivers for zoidberg kernel...

echo Loading zoidberg kernel...
kernel.efi initrd=initrd.img

