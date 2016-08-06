echo -off
echo Loading UEFI drivers for zoidberg kernel...
load fs0:\EFI\BOOT\SimpleThread.efi


mode 100 31
echo Loading zoidberg kernel...
fs0:\EFI\BOOT\kernel.efi initrd=/boot/EFI/BOOT/initrd.img

