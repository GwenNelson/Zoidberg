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
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

struct console_data
{
	FONT *f;
	int c_w, c_h;
	int cur_x, cur_y;
	int ncols, nrows;
	EW_COLOR fc, bc;
	char *text_buf;
	WINDOW *w;
	int dirty_x, dirty_y;
};

EFI_STATUS textbox_paint(WINDOW *w, RECT *update);

static void resize_tb(struct console_data *cd, RECT *loc, int cols, int rows)
{
	cd->nrows = rows;
	cd->ncols = cols;
	cd->c_w = loc->w / cd->ncols;
	cd->c_h = loc->h / cd->nrows;
	if(cd->cur_x >= cols)
		cd->cur_x = cols - 1;
	if(cd->cur_y >= rows)
		cd->cur_y = rows - 1;

	cd->text_buf = (char *)realloc(cd->text_buf, rows * cols * sizeof(char));
}

EFI_STATUS ew_create_textbox(WINDOW **w, RECT *loc, WINDOW *parent, FONT *f, int cols, int rows, EW_COLOR forecolor, EW_COLOR backcolor)
{
	if((w == NULL) || (loc == NULL) || (parent == NULL) || (f == NULL))
		return EFI_INVALID_PARAMETER;

	EFI_STATUS s;

	struct console_data *cd = (struct console_data *)malloc(sizeof(struct console_data));
	if(cd == NULL)
		return EFI_OUT_OF_RESOURCES;
	memset(cd, 0, sizeof(struct console_data));

	cd->text_buf = (char *)malloc(cols * rows * sizeof(char));
	assert(cd->text_buf);
	memset(cd->text_buf, ' ', cols * rows * sizeof(char));

	resize_tb(cd, loc, cols, rows);

	cd->fc = forecolor;
	cd->bc = backcolor;
	cd->f = f;

	fprintf(stderr, "creating textbox window\n");

	s = ew_create_window(&cd->w, loc, EW_DESKTOP, textbox_paint, cd, sizeof(struct console_data));
	if(EFI_ERROR(s))
	{
		fprintf(stderr, "efiwindow: ew_create_textbox: ew_create_window() failed: %i\n", s);
		return -1;
	}
	fprintf(stderr, "textbox window created (%#016llx), parent = %#016llx\n", cd->w, cd->w->parent);
	*w = cd->w;

	return EFI_SUCCESS;
}

EFI_STATUS textbox_paint(WINDOW *w, RECT *update)
{
	struct console_data *cd = (struct console_data *)w->paint_data;
	/* Determine the characters to paint */
	int first_col = update->x / cd->c_w;
	int first_row = update->y / cd->c_h;
	int last_col = (update->x + update->w) / cd->c_w;
	if((update->x + update->w) % cd->c_w == 0)
		last_col--;
	int last_row = (update->y + update->h) / cd->c_h;
	if((update->y + update->h) % cd->c_h == 0)
		last_row--;
	if(first_col >= cd->ncols)
		first_col = cd->ncols - 1;
	if(last_col >= cd->ncols)
		last_col = cd->ncols - 1;
	if(first_row >= cd->nrows)
		first_row = cd->nrows - 1;
	if(last_row >= cd->nrows)
		last_row = cd->nrows - 1;

	/* Paint them */
	int i, j;
	for(j = first_row; j <= last_row; j++)
	{
		for(i = first_col; i <= last_col; i++)
		{
			cd->f->draw_glyph(cd->f, w, (CHAR16)(cd->text_buf[i + j * cd->ncols]), cd->c_w * i, cd->c_h * j,
				cd->c_w, cd->c_h, cd->fc, cd->bc);
		}
	}

	return EFI_SUCCESS;
}

static void invalidate_loc(int x, int y, struct console_data *cd, RECT *fwrite_invalidate)
{
	int p_x = x * cd->c_w;
	int p_y = y * cd->c_h;
	
	if(fwrite_invalidate->x == -1)
		fwrite_invalidate->x = p_x;
	if(fwrite_invalidate->y == -1)
		fwrite_invalidate->y = p_y;

	if(p_x < fwrite_invalidate->x)
		fwrite_invalidate->x = p_x;
	if(p_y < fwrite_invalidate->y)
		fwrite_invalidate->y = p_y;

	if((p_x + cd->c_w) > (fwrite_invalidate->x + fwrite_invalidate->w))
		fwrite_invalidate->w = p_x + cd->c_w - fwrite_invalidate->x;
	if((p_y + cd->c_h) > (fwrite_invalidate->y + fwrite_invalidate->h))
		fwrite_invalidate->h = p_y + cd->c_h - fwrite_invalidate->y;
}

static void scr_up(struct console_data *cd, RECT *fwrite_invalidate)
{
	int j;

	for(j = 0; j < cd->nrows; j++)
	{
		if(j == (cd->nrows - 1))
			memset(&cd->text_buf[j * cd->ncols], ' ', cd->ncols);
		else
			memcpy(&cd->text_buf[j * cd->ncols], &cd->text_buf[(j + 1) * cd->ncols], cd->ncols);
	}

	fwrite_invalidate->x = 0;
	fwrite_invalidate->y = 0;
	fwrite_invalidate->w = cd->ncols * cd->c_w;
	fwrite_invalidate->h = cd->nrows * cd->c_h;
}

static void newline(struct console_data *cd, RECT *fwrite_invalidate)
{
	cd->cur_x = 0;
	cd->cur_y++;
	if(cd->cur_y == cd->nrows)
	{
		scr_up(cd, fwrite_invalidate);
		cd->cur_y = cd->nrows - 1;
	}
}

size_t ew_textbox_fwrite(const void *ptr, size_t size, size_t nmemb, void *data)
{
	int bytes = size * nmemb;
	int i = 0;

	/* fprintf(stderr, "ew_textbox_fwrite: ptr: %#016llx, size: %i, nmemb: %i, data: %#016llx\n",
		ptr, size, nmemb, data); */
	
	struct console_data *cd = (struct console_data *)data;

	RECT fwrite_invalidate;
	fwrite_invalidate.x = -1;
	fwrite_invalidate.y = -1;
	fwrite_invalidate.w = 0;
	fwrite_invalidate.h = 0;

	for(i = 0; i < bytes; i++)
	{
		char c = (char)*(uint8_t *)((uintptr_t)ptr + i);
		if(isprint(c))
		{
			cd->text_buf[cd->cur_x + cd->cur_y * cd->ncols] = c;
			invalidate_loc(cd->cur_x, cd->cur_y, cd, &fwrite_invalidate);

			cd->cur_x++;

			/* Update the dirty rectangle if necessary */
			if(cd->cur_x > cd->dirty_x)
				cd->dirty_x = cd->cur_x;

			if(cd->cur_x == cd->ncols)
				newline(cd, &fwrite_invalidate);
		}
		else if(c == '\n')
			newline(cd, &fwrite_invalidate);

		/* Update dirty rectangle y */
		if(cd->cur_y > cd->dirty_y)
			cd->dirty_y = cd->cur_y;
	}

	if((fwrite_invalidate.x != -1) && (fwrite_invalidate.y != -1))
		ew_invalidate_rect(cd->w, &fwrite_invalidate);
	
	return nmemb;
}

EFI_STATUS ew_set_textbox_text(WINDOW *w, const char *buf, size_t nchars)
{
	if((w == NULL) || (buf == NULL))
		return EFI_INVALID_PARAMETER;

	/* Clear the dirty rectangle */
	struct console_data *cd = (struct console_data *)w->paint_data;
	size_t dirty_length = cd->dirty_y * cd->ncols + cd->dirty_x;
	memset(cd->text_buf, ' ', dirty_length);
	RECT dirty_rect = { 0, 0, cd->dirty_x * cd->c_w, cd->dirty_y * cd->c_h };
	ew_invalidate_rect(w, &dirty_rect);

	/* Write out the text */
	if(nchars == EW_TEXTBOX_WHOLE_STRING)
		nchars = strlen(buf);
	ew_textbox_fwrite(buf, 1, nchars, cd);

	return EFI_SUCCESS;
}

EFI_STATUS ew_get_textbox_text(WINDOW *w, char *buf, size_t nchars)
{
	if((w == NULL) || (buf == NULL))
		return EFI_INVALID_PARAMETER;
	if(nchars == EW_TEXTBOX_WHOLE_STRING)
		return EFI_INVALID_PARAMETER;

	struct console_data *cd = (struct console_data *)w->paint_data;
	size_t buf_len = strnlen(cd->text_buf, cd->ncols * cd->nrows);
	size_t to_copy = nchars;
	if(buf_len < to_copy)
		to_copy = buf_len;

	memcpy(buf, cd->text_buf, to_copy);

	/* Null terminate if there is room */
	if(to_copy < nchars)
		((char *)buf)[to_copy] = 0;

	return EFI_SUCCESS;
}

EFI_STATUS ew_get_textbox_length(WINDOW *w, size_t *nchars)
{
	if((w == NULL) || (nchars == NULL))
		return EFI_INVALID_PARAMETER;

	struct console_data *cd = (struct console_data *)w->paint_data;
	*nchars = strnlen(cd->text_buf, cd->ncols * cd->nrows);

	return EFI_SUCCESS;
}
