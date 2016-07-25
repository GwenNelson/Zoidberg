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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

struct _ew_progress_int
{
	EW_PROGRESS info;
	WINDOW *outer, *bar;
};

static void set_outer_bar_rect(WINDOW *progress, RECT *outer_rect)
{
	outer_rect->x = 0;
	outer_rect->y = 0;
	outer_rect->w = progress->loc.w;
	outer_rect->h = progress->loc.h;
}

static void set_inner_bar_rect(WINDOW *progress, RECT *inner_rect, int v, int dir)
{
	switch(dir)
	{
		case EW_PROGRESS_LEFTRIGHT:
			inner_rect->x = 0;
			inner_rect->y = 0;
			inner_rect->w = (progress->loc.w * v) / 100;
			inner_rect->h = progress->loc.h;
			break;
		case EW_PROGRESS_RIGHTLEFT:
			inner_rect->x = (progress->loc.w * (100 - v)) / 100;
			inner_rect->y = 0;
			inner_rect->w = progress->loc.w - inner_rect->x;
			inner_rect->h = progress->loc.h;
			break;
		case EW_PROGRESS_TOPBOTTOM:
			inner_rect->x = 0;
			inner_rect->y = 0;
			inner_rect->w = progress->loc.w;
			inner_rect->h = (progress->loc.h * v) / 100;
			break;
		case EW_PROGRESS_BOTTOMTOP:
			inner_rect->x = 0;
			inner_rect->y = (progress->loc.h * (100 - v)) / 100;
			inner_rect->w = progress->loc.w;
			inner_rect->h = progress->loc.h - inner_rect->y;
			break;
	}
}

EFI_STATUS ew_create_progress(WINDOW **w, RECT *loc, WINDOW *parent, EW_PROGRESS *info)
{
	if((w == NULL) || (loc == NULL) || (parent == NULL) || (info == NULL))
		return EFI_INVALID_PARAMETER;

	struct _ew_progress_int *desc = (struct _ew_progress_int *)malloc(sizeof(struct _ew_progress_int));
	if(desc == NULL)
		return EFI_OUT_OF_RESOURCES;
	memset(desc, 0, sizeof(struct _ew_progress_int));
	memcpy(desc, info, sizeof(EW_PROGRESS));

	EFI_STATUS s = ew_create_window(w, loc, parent, ew_paint_null, desc, sizeof(struct _ew_progress_int));
	if(EFI_ERROR(s))
	{
		fprintf(stderr, "efiwindow: ew_create_progress: ew_create_window() failed: %i\n", s);
		return s;
	}

	/* Create the bars */
	RECT outer_rect, inner_rect;
	set_outer_bar_rect(*w, &outer_rect);
	set_inner_bar_rect(*w, &inner_rect, info->progress, info->direction);
	EW_RECT outer_info = { info->backcolor, info->linecolor, info->linewidth };
	EW_RECT inner_info = { info->barcolor, info->linecolor, info->linewidth };

	s = ew_create_rect(&desc->outer, &outer_rect, *w, &outer_info);
	if(EFI_ERROR(s))
	{
		fprintf(stderr, "efiwindow: ew_create_progress: ew_create_rect(outer) failed: %i\n", s);
		return s;
	}
	s = ew_create_rect(&desc->bar, &inner_rect, *w, &inner_info);
	if(EFI_ERROR(s))
	{
		fprintf(stderr, "efiwindow: ew_create_progress: ew_create_rect(inner) failed: %i\n", s);
		return s;
	}

	ew_show(desc->outer);
	ew_show(desc->bar);

	return EFI_SUCCESS;
}

EFI_STATUS ew_set_progress_info(WINDOW *w, EW_PROGRESS *desc)
{
	if((w == NULL) || (desc == NULL))
		return EFI_INVALID_PARAMETER;

	memcpy(w->paint_data, desc, sizeof(EW_PROGRESS));

	struct _ew_progress_int *pi = (struct _ew_progress_int *)w->paint_data;
	RECT bar;
	set_inner_bar_rect(w, &bar, desc->progress, desc->direction);
	ew_resize_window(pi->bar, &bar);

	return EFI_SUCCESS;
}

EFI_STATUS ew_get_progress_info(WINDOW *w, EW_PROGRESS *desc)
{
	if((w == NULL) || (desc == NULL))
		return EFI_INVALID_PARAMETER;

	memcpy(desc, w->paint_data, sizeof(EW_PROGRESS));

	return EFI_SUCCESS;
}
