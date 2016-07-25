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

#include "efiwindow.h"
#include <stdio.h>
#include <stdlib.h>
#include "windowlist.h"
#include <assert.h>
#include <string.h>
#include <sys/param.h>

EFI_GRAPHICS_OUTPUT_PROTOCOL *GOP;
extern EFI_GUID GraphicsOutputProtocol;

extern EFI_SYSTEM_TABLE *ST;
extern EFI_BOOT_SERVICES *BS;
extern EFI_RUNTIME_SERVICES *RT;
extern EFI_HANDLE gImageHandle;

WINDOW *EW_DESKTOP = NULL;

static int ew_can_blit = 0;


EFI_STATUS ew_init(EFI_HANDLE ImageHandle)
{
	/* Ensure the EFI_SYSTEM_TABLE *ST and EFI_BOOT_SERVICES *BS pointers are set */
	if(ST == NULL)
	{
		fprintf(stderr, "efiwindow: ew_init(): error: please set EFI_SYSTEM_TABLE *ST prior to calling efilibc_init()\n");
		return EFI_NOT_READY;
	}
	if(BS == NULL)
	{
		fprintf(stderr, "efiwindow: ew_init(): error: please set EFI_BOOT_SERVICES *BS prior to calling efilibc_init()\n");
		return EFI_NOT_READY;
	}

	/* Check arguments */
	if(ImageHandle == NULL)
	{
		fprintf(stderr, "efiwindow: ew_init(): error: ImageHandle cannot be NULL\n");
		return EFI_INVALID_PARAMETER;
	}

	/* Get the GOP device from the console out device */
	EFI_STATUS s = BS->OpenProtocol(ST->ConsoleOutHandle, &GraphicsOutputProtocol, (void **)&GOP, ImageHandle, NULL,
		EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
	if(EFI_ERROR(s))
	{
		fprintf(stderr, "efiwindow: ew_init(): error: Could not get Graphics Output Protocol: %i\n", s);
		return s;
	}

	return EFI_SUCCESS;
}

EFI_STATUS ew_create_window(WINDOW **w, RECT *loc, WINDOW *parent, ew_paint_func paint_func, void *data, size_t data_size)
{
	fprintf(stderr, "ew_create_window(%#016llx, %#016llx, %#016llx, %#016llx, %#016llx)\n",
		w, loc, parent, paint_func, data);
	if((EW_DESKTOP == NULL) && (parent != NULL))
	{
		fprintf(stderr, "efiwindow: ew_create_window(): please call ew_set_mode() first\n");
		return EFI_NOT_READY;
	}
	if((EW_DESKTOP != NULL) && (parent == NULL))
	{
		fprintf(stderr, "efiwindow: ew_create_window(): 'parent' argument cannot be NULL\n");
		return EFI_INVALID_PARAMETER;
	}

	WINDOW *ret = (WINDOW *)malloc(sizeof(WINDOW));
	assert(ret);
	memset(ret, 0, sizeof(WINDOW));

	ret->show = 0;
	ret->paint_data = data;
	ret->data_size = data_size;

	if(paint_func == NULL)
		ret->paint = ew_paint_color;
	else
		ret->paint = paint_func;

	fprintf(stderr, "ew_create_window: parent: %#016llx\n", parent);
	if(parent != NULL)
		ew_add_list(parent, ret);

	*w = ret;

	ew_resize_window(ret, loc);

	return EFI_SUCCESS;
}

EFI_STATUS ew_resize_window(WINDOW *w, RECT *loc)
{
	if((w == NULL) || (loc == NULL))
	{
		fprintf(stderr, "ew_resize_window: invalid arguments\n");
		return EFI_INVALID_PARAMETER;
	}

	w->loc.x = loc->x;
	w->loc.y = loc->y;
	w->loc.h = loc->h;
	w->loc.w = loc->w;

	size_t buf_size = w->loc.w * w->loc.h * EW_BB_PIXEL_SIZE;
	fprintf(stderr, "ew_resize_window: w: %#016llx, loc: (%i,%i,%i,%i), buf_size: %i\n", w, w->loc.x, w->loc.y, w->loc.w, w->loc.h, buf_size);

	w->buf = realloc(w->buf, buf_size);
	assert(w->buf || (buf_size == 0));

	if(w->resize)
		w->resize(w);

	if(w->show)
	{
		RECT r;
		r.x = 0;
		r.y = 0;
		r.w = loc->w;
		r.h = loc->h;
		ew_invalidate_rect(w, &r);
	}

	return EFI_SUCCESS;
}

EFI_STATUS ew_show(WINDOW *w)
{
	RECT r;
	r.x = 0;
	r.y = 0;
	r.w = w->loc.w;
	r.h = w->loc.h;
	w->show = 1;
	ew_invalidate_rect(w, &r);
	return EFI_SUCCESS;
}

EFI_STATUS ew_hide(WINDOW *w)
{
	RECT r;
	r.x = w->loc.x;
	r.y = w->loc.y;
	r.w = w->loc.w;
	r.h = w->loc.h;
	w->show = 0;
	ew_invalidate_rect(w->parent, &r);
	return EFI_SUCCESS;
}

struct update_bb_data
{
	RECT absolute_rect;
	void *bb;
	int bbw, bbh;
};

/* Get the intersection of two rectangles */
static void ew_intersect_rect(RECT *a, RECT *b, RECT *out)
{
	int a_right = a->x + a->w;
	int b_right = b->x + b->w;
	int a_bottom = a->y + a->h;
	int b_bottom = b->y + b->h;

	out->x = -1;
	out->y = -1;
	out->w = -1;
	out->h = -1;

	int out_right = -1;
	int out_bottom = -1;

	if((((a->x < b_right) && (a_right > b->x)) || ((b->x < a_right) && (b_right > a->x))))
	{
		out->x = MAX(a->x, b->x);
		out_right = MIN(a_right, b_right);
	}

	if((((a->y < b_bottom) && (a_bottom > b->y)) || ((b->y < a_bottom) && (b_bottom > a->y))))
	{
		out->y = MAX(a->y, b->y);
		out_bottom = MIN(a_bottom, b_bottom);
	}

	if((out_right != -1) && (out->x != -1))
		out->w = out_right - out->x;
	else
		out->x = out->y = out->h = out->w = -1;

	if((out_bottom != -1) && (out->y != -1))
		out->h = out_bottom - out->y;
	else
		out->x = out->y = out->h = out->w = -1;
}

/* Convert a window-relative rectangle to a screen-relative one */
static void ew_get_absolute_rect(WINDOW *w, RECT *r, RECT *out)
{
	out->w = r->w;
	out->h = r->h;
	out->x = r->x;
	out->y = r->y;

	//fprintf(stderr, "ew_get_absolute_rect: w: %#016llx, w->parent: %#016llx, EW_DESKTOP: %#016llx\n", w, w->parent, EW_DESKTOP);

	WINDOW *cur_window = w;
	while(cur_window != EW_DESKTOP)
	{
		//fprintf(stderr, "ew_get_absolute_rect: cur_window: %#016llx, cur_window->parent: %#016llx\n", cur_window, cur_window->parent);
		out->x += cur_window->loc.x;
		out->y += cur_window->loc.y;
		cur_window = cur_window->parent;
	}
}

/* Determine if the window is to be shown (only if window and all parents have show set to 1) */
static int ew_get_absolute_show(WINDOW *w)
{
	WINDOW *cur_window = w;
	while(cur_window != EW_DESKTOP)
	{
		if(cur_window->show == 0)
			return 0;
		cur_window = cur_window->parent;
	}

	return cur_window->show;
}

static void ew_update_bb(WINDOW *w, void *data)
{
	struct update_bb_data *ubbd = (struct update_bb_data *)data;
	RECT w_abs_rect;
	RECT w_rect;
	w_rect.x = 0;
	w_rect.y = 0;
	w_rect.w = w->loc.w;
	w_rect.h = w->loc.h;
	ew_get_absolute_rect(w, &w_rect, &w_abs_rect);
	RECT intersect_rect;
	ew_intersect_rect(&ubbd->absolute_rect, &w_abs_rect, &intersect_rect);

	int i, j;

	for(j = intersect_rect.y; j < (intersect_rect.y + intersect_rect.h); j++)
	{
		for(i = intersect_rect.x; i < (intersect_rect.x + intersect_rect.w); i++)
		{
			int w_x = i - w_abs_rect.x;
			int w_y = j - w_abs_rect.y;

			uint32_t *src_color = (uint32_t *)&((uint8_t *)w->buf)[(w_x + w_y * w->loc.w) * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL)];
			uint32_t *dest_color = (uint32_t *)&((uint8_t *)ubbd->bb)[(i + j * ubbd->bbw) * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL)];

			*dest_color = ew_blend(*src_color, *dest_color);
		}
	}
}

EFI_STATUS ew_invalidate_rect(WINDOW *w, RECT *r)
{
	if(w == NULL)
		return EFI_INVALID_PARAMETER;

	//fprintf(stderr, "ew_invalidate_rect: w: %#016llx\n", w);

	/* If r is NULL, invalidate the whole window */
	RECT full_window;
	if(r == NULL)
	{
		full_window.x = 0; full_window.y = 0; full_window.w = w->loc.w; full_window.h = w->loc.h;
		r = &full_window;
	}

	/* Determine the absolute coordinates of the particular rectangle */
	RECT abs_rect;
	ew_get_absolute_rect(w, r, &abs_rect);

	/* Paint the window */
	void *buf;
	int bbw, bbh;
	ew_get_backbuffer(&buf);
	ew_get_backbuffer_size(&bbw, &bbh);

	if(w->paint != NULL)
		w->paint(w, r);

	/* Don't write to backbuffer if we're invisible */
	if(ew_get_absolute_show(w) == 0)
		return EFI_SUCCESS;

	/* Update the backbuffer */
	struct update_bb_data ubbd;
	ubbd.absolute_rect = abs_rect;
	ubbd.bb = buf;
	ubbd.bbw = bbw;
	ubbd.bbh = bbh;

	/* First clear the backbuffer */
	int i, j;
	for(j = abs_rect.y; j < (abs_rect.y + abs_rect.h); j++)
	{
		for(i = abs_rect.x; i < (abs_rect.x + abs_rect.w); i++)
		{
			uint32_t *bb_pixel = (uint32_t *)&((uint8_t *)buf)[(i + j * bbw) * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL)];
			*bb_pixel = 0;
		}
	}

	ew_leftmost_traversal(EW_DESKTOP, ew_update_bb, &ubbd);

	if(ew_can_blit)
	{
		/* blit to the display */
		return GOP->Blt(GOP, (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)buf, EfiBltBufferToVideo, abs_rect.x, abs_rect.y, 
			abs_rect.x, abs_rect.y, r->w, r->h, bbw * 4);
	}
	else
	{
		/* use our own blit method */
		return ew_blit(buf, abs_rect.x, abs_rect.y, abs_rect.x, abs_rect.y, r->w, r->h, bbw * 4);
	}
}

EFI_STATUS ew_set_can_blit(int can_blit)
{
	if((can_blit != 0) && (can_blit != 1))
		return EFI_INVALID_PARAMETER;

	ew_can_blit = can_blit;
	return EFI_SUCCESS;
}

EFI_STATUS ew_paint_color(WINDOW *w, RECT *update)
{
	UINT32 pixel = (UINT32)(uintptr_t)w->paint_data;
	UINT32* bbuf = (UINT32 *)w->buf;
	
	int i, j;
	for(j = update->y; j < (update->y + update->h); j++)
	{
		for(i = update->x; i < (update->x + update->w); i++)
			bbuf[j * w->loc.w + i] = pixel;
	}

	return EFI_SUCCESS;
}

EFI_STATUS ew_paint_null(WINDOW *w, RECT *update)
{
	(void)w; (void)update;

	fprintf(stderr, "ew_paint_null\n");

	return EFI_SUCCESS;
}

EFI_STATUS ew_get_data(WINDOW *w, void *buf, size_t *bufsize)
{
	if(w == NULL)
		return EFI_INVALID_PARAMETER;
	if(buf == NULL)
	{
		*bufsize = w->data_size;
		return EFI_BUFFER_TOO_SMALL;
	}
	memcpy(buf, w->paint_data, *bufsize);
	if(*bufsize < w->data_size)
	{
		*bufsize = w->data_size;
		return EFI_BUFFER_TOO_SMALL;
	}
	return EFI_SUCCESS;
}
