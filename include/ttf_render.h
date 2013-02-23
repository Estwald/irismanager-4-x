#ifndef TTF_RENDER_H
#define TTF_RENDER_H

#include <tiny3d.h>
#include <libfont.h>

int TTFLoadFont(char * path, void * from_memory, int size_from_memory);
void TTFUnloadFont();

void TTF_to_Bitmap(u8 chr, u8 * bitmap, short *w, short *h, short *y_correction);

int Render_String_UTF8(u16 * bitmap, int w, int h, u8 *string, int sw, int sh);

#endif
