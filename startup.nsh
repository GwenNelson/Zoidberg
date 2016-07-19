echo -off
echo Loading UEFI drivers for zoidberg kernel...
load fs0:\EFI\BOOT\SimpleThread.efi

echo Loading zoidberg kernel...
fs0:\EFI\BOOT\kernel.efi

