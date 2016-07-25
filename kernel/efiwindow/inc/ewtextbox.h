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

#ifndef EWTEXTBOX_H
#define EWTEXTBOX_H

#define EW_TEXTBOX_WHOLE_STRING		((size_t)-1)

EFI_STATUS ew_create_textbox(WINDOW **w, RECT *loc, WINDOW *parent, FONT *f, int cols, int rows, EW_COLOR forecolor, EW_COLOR backcolor);
EFI_STATUS ew_get_textbox_text(WINDOW *w, char *buf, size_t nchars);
EFI_STATUS ew_get_textbox_length(WINDOW *w, size_t *nchars);
EFI_STATUS ew_set_textbox_text(WINDOW *w, const char *buf, size_t nchars);
size_t ew_textbox_fwrite(const void *ptr, size_t size, size_t nmemb, void *data);

#endif
