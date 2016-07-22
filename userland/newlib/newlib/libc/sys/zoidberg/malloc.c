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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <efi.h>
#include <efilib.h>
#include "k_heap.h"

void *malloc(size_t size)
{
	return k_heap_malloc(size);
}

void *calloc(size_t nmemb, size_t size)
{
	uint64_t s = (uint64_t)nmemb * (uint64_t)size;
	return k_heap_malloc(s);
}

void *realloc(void *ptr, size_t size)
{
	if(ptr == NULL)
		return malloc(size);
	if(size == 0)
	{
		free(ptr);
		return NULL;
	}

	void *buf;
	buf = malloc(size);
	if(buf==NULL) return NULL;

	memcpy(buf,ptr,size);
	free(ptr);
	return buf;
}

void free(void *ptr)
{
	return k_heap_free(ptr);
}
