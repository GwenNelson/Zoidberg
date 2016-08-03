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

extern EFI_BOOT_SERVICES *BS;

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


EFI_GRAPHICS_OUTPUT_PROTOCOL *GraphicsOutput;
#include "zoidberg_logo.h"
void draw_logo() {




	UINTN BltSize = Logo_bmp_size * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL);
	void* Blt = malloc(BltSize);
	UINTN width;
	UINTN height;
	EFI_STATUS s = ConvertBmpToGopBlt((UINT8*)Logo_bmp,(UINTN)Logo_bmp_size,(void**)&Blt,&BltSize,&height,&width);
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

void init_video(char* vgamode) {
     if(vgamode==NULL) {
        vgamode = "1024x768x32"; // a sane default for most platforms
     }

     UINTN desired_width  = atoi(strtok(vgamode,"x"));
     UINTN desired_height = atoi(strtok(NULL,"x"));
     UINTN desired_bpp    = atoi(strtok(NULL,"x"));

     klog("VIDEO",1,"Will attempt to set display mode of %dx%dx%d",desired_width,desired_height,desired_bpp);

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
     } else {
        klog("VIDEO",1,"Located %d GOP handles", handleCount);
     }
     int m=0;
     BS->HandleProtocol (handleBuffer[0], &gEfiGraphicsOutputProtocolGuid, (VOID **) &GraphicsOutput);
     EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *mi;
     UINTN isize;
     UINTN bpp;
     UINTN width;
     UINTN height;
     int matching_mode = -1;
     for(m=0; m < GraphicsOutput->Mode->MaxMode; m++) {
         GraphicsOutput->QueryMode(GraphicsOutput, m, &isize, &mi);
         bpp = 0;
         width  = mi->HorizontalResolution;
         height = mi->VerticalResolution;
         switch(mi->PixelFormat) {
            case PixelRedGreenBlueReserved8BitPerColor:
              bpp = 32;
            break;
            case PixelBlueGreenRedReserved8BitPerColor:
              bpp = 32;
            break;
            case PixelBitMask:
              bpp = 0; // this will fail to match any sane mode, and weird framebuffers are a total pain, so forget about using bitmasks
            break;
         }
         if((width==desired_width) && (height==desired_height) && (bpp==desired_bpp)) {
            matching_mode = m;
            break;
         }
     }
     if(matching_mode==-1) {
        klog("VIDEO",0,"Failed to set desired mode!");
     } else {
        GraphicsOutput->SetMode(GraphicsOutput,matching_mode);
     }
}
