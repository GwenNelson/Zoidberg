/* Copyright (C) 2014 by John Cronin
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <efiwindow.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_LIBPNG
#include <png.h>
static EFI_STATUS png_init(WINDOW **w, RECT *loc, WINDOW *parent, FILE *fp, EW_BITMAP *info);
static EFI_STATUS png_paint(WINDOW *w, RECT *update);
#endif

#define make_bgra_pixel(r, g, b, a) \
	((b) | ((g) << 8) | ((r) << 16) | ((a) << 24))


EFI_STATUS ew_create_bitmap(WINDOW **w, RECT *loc, WINDOW *parent, EW_BITMAP *info)
{
	if((w == NULL) || (loc == NULL) || (info == NULL) || (parent == NULL))
	{
		fprintf(stderr, "efiwindow: ew_create_bitmap: invalid param: w: %#016llx, loc: %#016llx, parent: %#016llx, fname: %#016llx\n",
			w, loc, parent, info->fname);
		return EFI_INVALID_PARAMETER;
	}

	FILE *fp = fopen(info->fname, "r");
	if(fp == NULL)
		return EFI_NOT_FOUND;

	switch(info->bitmap_type)
	{
	case EW_BITMAP_TYPE_GUESS:
		{
			/* Try and guess the bitmap type */

#ifdef HAVE_LIBPNG
			uint8_t png_header[8];
			size_t png_fread_ret = fread(png_header, 1, 8, fp);
			fseek(fp, 0, SEEK_SET);
			if((png_fread_ret == 8) && (!png_sig_cmp(png_header, 0, 8)))
			{
				/* Its a png file */
				return png_init(w, loc, parent, fp, info);
			}
#endif
			
			/* Unknown file type */
			fprintf(stderr, "efiwindow: ew_create_bitmap: unknown file type for %s\n", info->fname);
			return EFI_INVALID_PARAMETER;
		}

#ifdef HAVE_LIBPNG
	case EW_BITMAP_TYPE_PNG:
		{
			/* Still confirm its a png */
			uint8_t png_header[8];
			size_t png_fread_ret = fread(png_header, 1, 8, fp);
			fseek(fp, 0, SEEK_SET);
			if((png_fread_ret == 8) && (!png_sig_cmp(png_header, 0, 8)))
				return png_init(w, loc, parent, fp, info);

			fprintf(stderr, "efiwindow: ew_create_bitmap: %s is not a png file\n", info->fname);
			return EFI_INVALID_PARAMETER;
		}
#endif

	default:
		fprintf(stderr, "efiwindow: ew_create_bitmap: unknown bitmap_type: %i\n", info->bitmap_type);
		return EFI_INVALID_PARAMETER;
	}
}

#ifdef HAVE_LIBPNG

struct png_data
{
	EW_BITMAP bmp;
	png_uint_32 width;
	png_uint_32 height;
	int bdepth;
	int color_type;
	int interlace_method, comp_method, filt_method;
	int pixel_size;
	png_bytep *row_pointers;
};

EFI_STATUS png_init(WINDOW **w, RECT *loc, WINDOW *parent, FILE *fp, EW_BITMAP *info)
{
	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
		NULL, NULL, NULL);
	if(png_ptr == NULL)
	{
		fprintf(stderr, "efiwindow: png_init: png_create_read_struct failed\n");
		return EFI_ABORTED;
	}

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if(info_ptr == NULL)
	{
		fprintf(stderr, "efiwindow: png_init: png_create_info_struct failed\n");
		return EFI_ABORTED;
	}

	png_init_io(png_ptr, fp);
	png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_PACKING | PNG_TRANSFORM_BGR, NULL);

	struct png_data *pd = (struct png_data *)malloc(sizeof(struct png_data));
	memset(pd, 0, sizeof(struct png_data));

	png_get_IHDR(png_ptr, info_ptr, &pd->width, &pd->height, &pd->bdepth, &pd->color_type, &pd->interlace_method,
		&pd->comp_method, &pd->filt_method);

	pd->row_pointers = png_get_rows(png_ptr, info_ptr);

	switch(pd->color_type)
	{
		case PNG_COLOR_TYPE_RGB:
			pd->pixel_size = (3 * pd->bdepth) / 8;
			break;
		case PNG_COLOR_TYPE_RGBA:
			pd->pixel_size = (4 * pd->bdepth) / 8;
			break;
		default:
			fprintf(stderr, "efiwindow: png_init: unsupported color type: %i\n", pd->color_type);
			return EFI_ABORTED;
	}

	memcpy(&pd->bmp, info, sizeof(EW_BITMAP));
	
	return ew_create_window(w, loc, parent, png_paint, pd, sizeof(struct png_data));
}

EFI_STATUS png_paint(WINDOW *w, RECT *update)
{
	int i, j;
	struct png_data *pd = (struct png_data *)w->paint_data;

	for(j = update->y; j < (update->y + update->h); j++)
	{
		for(i = update->x; i < (update->x + update->w); i++)
		{
			int src_x, src_y;

			/* Determine the source coordinates */
			switch(pd->bmp.stretch)
			{
				case EW_BITMAP_STRETCH_FILL:
					src_x = (pd->width * i) / w->loc.w;
					src_y = (pd->height * j) / w->loc.h;
					break;

				case EW_BITMAP_STRETCH_CENTER:
					src_x = i - (w->loc.w - pd->width) / 2;
					src_y = j - (w->loc.h - pd->height) / 2;
					break;

				case EW_BITMAP_STRETCH_TILE:
					src_x = i % pd->width;
					src_y = j % pd->height;
					break;

				case EW_BITMAP_STRETCH_NONE:
				default:
					src_x = i;
					src_y = j;
					break;
			};

			uint32_t *dest_pixel = (uint32_t *)&((uint8_t *)w->buf)[(i + j * w->loc.w) * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL)];
			EW_COLOR blank = 0x0;
			uint32_t color;
			
			if((src_x >= 0) && (src_x < (int)pd->width) && (src_y >= 0) && (src_y < (int)pd->height))
			{
				uint8_t *src_pixel = &((uint8_t *)pd->row_pointers[src_y])[src_x * pd->pixel_size];
				color = make_bgra_pixel(src_pixel[2], src_pixel[1], src_pixel[0],
					(pd->color_type == PNG_COLOR_TYPE_RGBA) ? src_pixel[3] : 0xff);
			}
			else
				color = blank;

			*dest_pixel = color;
		}
	}

	return EFI_SUCCESS;
}
#endif