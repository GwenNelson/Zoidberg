#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/EfiSysCall.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/DevicePath.h>
#include <Library/PcdLib.h>
#include <Library/UefiLib.h>
#include <Protocol/LoadedImage.h>
#include <../MdePkg/Include/Library/DevicePathLib.h>
#include <Protocol/EfiShell.h>
#include <Library/BaseLib.h>
#include <Pi/PiFirmwareFile.h>
#include <Library/DxeServicesLib.h>
#include <IndustryStandard/Bmp.h>
#include <Protocol/GraphicsOutput.h>


#include "kmsg.h"
#include "k_thread.h"
#include "zoidberg_version.h"
#include "efiwindow/efiwindow.h"
#include "efiwindow/ewbitmap.h"
#include "libvterm/vterm.h"

EFI_SYSTEM_TABLE *ST;
EFI_BOOT_SERVICES *BS;
EFI_RUNTIME_SERVICES *RT;
extern EFI_HANDLE gImageHandle;


EFI_STATUS OpenShellProtocol( EFI_SHELL_PROTOCOL            **gEfiShellProtocol )
{
    EFI_STATUS                      Status;
    Status = gBS->OpenProtocol(
            gImageHandle,
            &gEfiShellProtocolGuid,
            (VOID **)gEfiShellProtocol,
            gImageHandle,
            NULL,
            EFI_OPEN_PROTOCOL_GET_PROTOCOL
            );
    if (EFI_ERROR(Status)) {
    //
    // Search for the shell protocol
    //
        Status = gBS->LocateProtocol(
                &gEfiShellProtocolGuid,
                NULL,
                (VOID **)gEfiShellProtocol
                );
        if (EFI_ERROR(Status)) {
            gEfiShellProtocol = NULL;
        }
  }
  return Status;
}

// TODO - move this to another file
void uefi_run(void* _t) {
     struct task_def_t *t = (struct task_def_t*)_t;
     char* _filename = (char*)t->arg;
     EFI_STATUS rstat = 0;
     EFI_SHELL_PROTOCOL            *shell;
     EFI_DEVICE_PATH_PROTOCOL *path;
     EFI_STATUS s = OpenShellProtocol(&shell);
     EFI_HANDLE child_h;
     path = shell->GetDevicePathFromFilePath(_filename);


     s = BS->LoadImage(0,gImageHandle,path,NULL,NULL,&child_h);
     install_syscall_protocol(child_h,ST,t->task_id);
     s = BS->StartImage(child_h,NULL,NULL);
     BS->UnloadImage(child_h);
}

void idle_task(void* _t) {
     klog("IDLE",1,"Kernel idle task started");
     for(;;) {
         BS->Stall(100000);
     }
}

EFI_STATUS
ConvertBmpToGopBlt (
  IN     VOID      *BmpImage,
  IN     UINTN     BmpImageSize,
  IN OUT VOID      **GopBlt,
  IN OUT UINTN     *GopBltSize,
     OUT UINTN     *PixelHeight,
     OUT UINTN     *PixelWidth
  )
{
  UINT8                         *Image;
  UINT8                         *ImageHeader;
  BMP_IMAGE_HEADER              *BmpHeader;
  BMP_COLOR_MAP                 *BmpColorMap;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *BltBuffer;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Blt;
  UINT64                        BltBufferSize;
  UINTN                         Index;
  UINTN                         Height;
  UINTN                         Width;
  UINTN                         ImageIndex;
  UINT32                        DataSizePerLine;
  BOOLEAN                       IsAllocated;
  UINT32                        ColorMapNum;

  if (sizeof (BMP_IMAGE_HEADER) > BmpImageSize) {
    return EFI_INVALID_PARAMETER;
  }

  BmpHeader = (BMP_IMAGE_HEADER *) BmpImage;

  if (BmpHeader->CharB != 'B' || BmpHeader->CharM != 'M') {
    return EFI_UNSUPPORTED;
  }

  //
  // Doesn't support compress.
  //
  if (BmpHeader->CompressionType != 0) {
    return EFI_UNSUPPORTED;
  }

  //
  // Only support BITMAPINFOHEADER format.
  // BITMAPFILEHEADER + BITMAPINFOHEADER = BMP_IMAGE_HEADER
  //
  if (BmpHeader->HeaderSize != sizeof (BMP_IMAGE_HEADER) - OFFSET_OF(BMP_IMAGE_HEADER, HeaderSize)) {
    return EFI_UNSUPPORTED;
  }

  //
  // The data size in each line must be 4 byte alignment.
  //
  DataSizePerLine = ((BmpHeader->PixelWidth * BmpHeader->BitPerPixel + 31) >> 3) & (~0x3);
  BltBufferSize = MultU64x32 (DataSizePerLine, BmpHeader->PixelHeight);
  if (BltBufferSize > (UINT32) ~0) {
    return EFI_INVALID_PARAMETER;
  }

  if ((BmpHeader->Size != BmpImageSize) || 
      (BmpHeader->Size < BmpHeader->ImageOffset) ||
      (BmpHeader->Size - BmpHeader->ImageOffset !=  BmpHeader->PixelHeight * DataSizePerLine)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Calculate Color Map offset in the image.
  //
  Image       = BmpImage;
  BmpColorMap = (BMP_COLOR_MAP *) (Image + sizeof (BMP_IMAGE_HEADER));
  if (BmpHeader->ImageOffset < sizeof (BMP_IMAGE_HEADER)) {
    return EFI_INVALID_PARAMETER;
  }

  if (BmpHeader->ImageOffset > sizeof (BMP_IMAGE_HEADER)) {
    switch (BmpHeader->BitPerPixel) {
      case 1:
        ColorMapNum = 2;
        break;
      case 4:
        ColorMapNum = 16;
        break;
      case 8:
        ColorMapNum = 256;
        break;
      default:
        ColorMapNum = 0;
        break;
      }
    //
    // BMP file may has padding data between the bmp header section and the bmp data section.
    //
    if (BmpHeader->ImageOffset - sizeof (BMP_IMAGE_HEADER) < sizeof (BMP_COLOR_MAP) * ColorMapNum) {
      return EFI_INVALID_PARAMETER;
    }
  }

  //
  // Calculate graphics image data address in the image
  //
  Image         = ((UINT8 *) BmpImage) + BmpHeader->ImageOffset;
  ImageHeader   = Image;

  //
  // Calculate the BltBuffer needed size.
  //
  BltBufferSize = MultU64x32 ((UINT64) BmpHeader->PixelWidth, BmpHeader->PixelHeight);
  //
  // Ensure the BltBufferSize * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL) doesn't overflow
  //
  if (BltBufferSize > DivU64x32 ((UINTN) ~0, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL))) {
    return EFI_UNSUPPORTED;
  }
  BltBufferSize = MultU64x32 (BltBufferSize, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));

  IsAllocated   = FALSE;
  if (*GopBlt == NULL) {
    //
    // GopBlt is not allocated by caller.
    //
    *GopBltSize = (UINTN) BltBufferSize;
    *GopBlt     = AllocatePool (*GopBltSize);
    IsAllocated = TRUE;
    if (*GopBlt == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  } else {
    //
    // GopBlt has been allocated by caller.
    //
    if (*GopBltSize < (UINTN) BltBufferSize) {
      *GopBltSize = (UINTN) BltBufferSize;
      return EFI_BUFFER_TOO_SMALL;
    }
  }

  *PixelWidth   = BmpHeader->PixelWidth;
  *PixelHeight  = BmpHeader->PixelHeight;

  //
  // Convert image from BMP to Blt buffer format
  //
  BltBuffer = *GopBlt;
  for (Height = 0; Height < BmpHeader->PixelHeight; Height++) {
    Blt = &BltBuffer[(BmpHeader->PixelHeight - Height - 1) * BmpHeader->PixelWidth];
    for (Width = 0; Width < BmpHeader->PixelWidth; Width++, Image++, Blt++) {
      switch (BmpHeader->BitPerPixel) {
      case 1:
        //
        // Convert 1-bit (2 colors) BMP to 24-bit color
        //
        for (Index = 0; Index < 8 && Width < BmpHeader->PixelWidth; Index++) {
          Blt->Red    = BmpColorMap[((*Image) >> (7 - Index)) & 0x1].Red;
          Blt->Green  = BmpColorMap[((*Image) >> (7 - Index)) & 0x1].Green;
          Blt->Blue   = BmpColorMap[((*Image) >> (7 - Index)) & 0x1].Blue;
          Blt++;
          Width++;
        }

        Blt--;
        Width--;
        break;

      case 4:
        //
        // Convert 4-bit (16 colors) BMP Palette to 24-bit color
        //
        Index       = (*Image) >> 4;
        Blt->Red    = BmpColorMap[Index].Red;
        Blt->Green  = BmpColorMap[Index].Green;
        Blt->Blue   = BmpColorMap[Index].Blue;
        if (Width < (BmpHeader->PixelWidth - 1)) {
          Blt++;
          Width++;
          Index       = (*Image) & 0x0f;
          Blt->Red    = BmpColorMap[Index].Red;
          Blt->Green  = BmpColorMap[Index].Green;
          Blt->Blue   = BmpColorMap[Index].Blue;
        }
        break;

      case 8:
        //
        // Convert 8-bit (256 colors) BMP Palette to 24-bit color
        //
        Blt->Red    = BmpColorMap[*Image].Red;
        Blt->Green  = BmpColorMap[*Image].Green;
        Blt->Blue   = BmpColorMap[*Image].Blue;
        break;

      case 24:
        //
        // It is 24-bit BMP.
        //
        Blt->Blue   = *Image++;
        Blt->Green  = *Image++;
        Blt->Red    = *Image;
        break;

      default:
        //
        // Other bit format BMP is not supported.
        //
        if (IsAllocated) {
          FreePool (*GopBlt);
          *GopBlt = NULL;
        }
        return EFI_UNSUPPORTED;
      };

    }

    ImageIndex = (UINTN) (Image - ImageHeader);
    if ((ImageIndex % 4) != 0) {
      //
      // Bmp Image starts each row on a 32-bit boundary!
      //
      Image = Image + (4 - (ImageIndex % 4));
    }
  }

  return EFI_SUCCESS;
}



#include "zoidberg_logo.h"
void draw_logo() {
     EFI_GRAPHICS_OUTPUT_PROTOCOL *GraphicsOutput;
     UINTN handleCount;
     EFI_HANDLE *handleBuffer;
     BS->LocateHandleBuffer(
                    ByProtocol,
                    &gEfiGraphicsOutputProtocolGuid,
                    NULL,
                    &handleCount,
                    &handleBuffer);
     EFI_STATUS s = BS->HandleProtocol (handleBuffer[0], &gEfiGraphicsOutputProtocolGuid, (VOID **) &GraphicsOutput);
     if(EFI_ERROR(s)) {
        klog("VIDEO",0,"No graphics output support: %d",s); 
        return;
     }


	UINTN BltSize = Logo_bmp_size * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL);
	void* Blt = malloc(BltSize);
	UINTN width;
	UINTN height;
	s = ConvertBmpToGopBlt((UINT8*)Logo_bmp,(UINTN)Logo_bmp_size,(void**)&Blt,&BltSize,&height,&width);
	if(EFI_ERROR(s)) {
	   klog("VIDEO",0,"Could not convert logo.bmp: %d",s);
           return;
	}

	s = GraphicsOutput->Blt (
                 GraphicsOutput,
                 Blt,
                 EfiBltBufferToVideo,
                 0,
                 0,
                 (UINTN) 10,
                 (UINTN) 10,
                 width,
                 height,
                 width * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
                 );
	if(EFI_ERROR(s)) {
	   klog("VIDEO",0,"Could not render logo: %d", s);
	}
}

void init_video() {
    EFI_STATUS s = ew_init(gImageHandle);
    if(EFI_ERROR(s)) {
       klog("VIDEO",0,"Could not setup efiwindow library!");
       return;
    } else {
       klog("VIDEO",1,"Started efiwindow");
    }
    
    FONT *font;
    s = ew_load_psf_font(&font, "fs0:\\EFI\\boot\\unifont.psf");
      
    if(EFI_ERROR(s)) {
       klog("VIDEO",0,"Could not load console font");
       return;
    } else {
       klog("VIDEO",1,"Loaded console font");
    }

    s = ew_set_mode(800,600,32);
    if(EFI_ERROR(s)) {
       klog("VIDEO",0,"Could not set graphics mode 800x600x32");
       return;
    } else {
       klog("VIDEO",1,"Set graphics mode 800x600x32");
    }
}

static VTerm *console_term=NULL; 
static VTermScreen* vscreen=NULL;
int term_damage(VTermRect rect, __unused void* user)
{
    /* TODO: Reimplement in a less slow-ass way */
    VTermScreenCell cell;
    VTermPos pos;
    uint8_t fg, bg, color;
    int row, col;
    for (row = rect.start_row; row < rect.end_row; row++)
    for (col = rect.start_col; col < rect.end_col; col++)
    {
        pos.col = col;
        pos.row = row;
        vterm_screen_get_cell(vscreen, pos, &cell);
//        fg = rgb2vga(cell.fg.red, cell.fg.green, cell.fg.blue);
//        bg = rgb2vga(cell.bg.red, cell.bg.green, cell.bg.blue);

        if (cell.attrs.reverse)
            color = bg | (fg << 4);
        else
            color = fg | (bg << 4);
//        display_put(col, row, cell.chars[0], color);
    }
    return 1;
}

static VTermScreenCallbacks vtsc =
{
    .damage = &term_damage,
    .moverect = NULL,
    .movecursor = NULL,
    .settermprop = NULL,
    .bell = NULL,
    .resize = NULL,
};

int main(int argc, char** argv) {

    ST = gST;
    BS = ST->BootServices;
    RT = ST->RuntimeServices;
    

    init_static_kmsg();

    int i;

    char* initrd_path = NULL;

    if(argc>1) {
       for(i=1; i<argc; i++) {
           if(strncmp(argv[i],"initrd=",7)==0) {
              initrd_path = argv[i]+7;
           }
       }
    }

    init_video();
    ST->ConOut->ClearScreen(ST->ConOut);

    char* build_no = ZOIDBERG_BUILD;
    char* version  = ZOIDBERG_VERSION;
    ST->ConOut->SetAttribute(ST->ConOut,EFI_TEXT_ATTR(EFI_WHITE,EFI_BACKGROUND_BLACK));
    kprintf("\n");
    kprintf("\t\tZoidberg kernel, Copyright 2016 Gareth Nelson\n");
    kprintf("\t\tKernel version: %s, build number: %s\n", version, build_no);
    kprintf("\t\tKernel entry point located at %#11x\n\n\n\n", (UINT64)main);
 

    draw_logo();
 
    init_dynamic_kmsg();
 

    if(initrd_path==NULL) {
       klog("INITRD",0,"No initrd= option specified, will default to initrd.img");
    } else {
       klog("INITRD",1,"Mounting initrd image from %s",initrd_path);
    }

    klog("UEFI",1,"Disabling watchdog");
    BS->SetWatchdogTimer(0, 0, 0, NULL);

    klog("TASKING",1,"Starting multitasking");
    scheduler_start();

    klog("TASKING",1,"Spawning kernel idle task");
    init_kernel_task(&idle_task,NULL);
    BS->Stall(1000);

    console_term = vterm_new(80,25);    

    if(console_term==NULL) {
       klog("TERM",0,"Failed to create libvterm terminal");
    } else {
       klog("TERM",1,"Created libvterm terminal");
       vscreen = vterm_obtain_screen(console_term);
       vterm_screen_set_callbacks(vscreen, &vtsc, NULL);
       vterm_screen_enable_altscreen(vscreen, 1);
       vterm_screen_reset(vscreen, 1);
    }

    klog("INIT",1,"Starting PID 1 /sbin/init");
 
    req_task(&uefi_run,(void*)L"initrd:\\sbin\\init");
    while(1) {
       init_tasks();
    }
    return EFI_SUCCESS;
}
