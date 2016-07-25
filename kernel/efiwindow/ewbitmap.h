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

#ifndef BITMAP_H
#define BITMAP_H

#define EW_BITMAP_TYPE_GUESS		0

#ifdef HAVE_LIBPNG
#define EW_BITMAP_TYPE_PNG			1
#endif

#define EW_BITMAP_STRETCH_NONE		0
#define EW_BITMAP_STRETCH_TILE		1
#define EW_BITMAP_STRETCH_FILL		2
#define EW_BITMAP_STRETCH_CENTER	3

typedef struct _ew_bitmap
{
	char *fname;
	int bitmap_type;
	int stretch;
} EW_BITMAP;

EFI_STATUS ew_create_bitmap(WINDOW **w, RECT *loc, WINDOW *parent, EW_BITMAP *info); 

#endif