
// you need the Oopo ps3libraries to work with freetype

#include <ft2build.h>
#include <freetype/freetype.h> 
#include <freetype/ftglyph.h>
#include "ttf_render.h"

/******************************************************************************************************************************************************/
/* TTF functions to load and convert fonts                                                                                                             */
/******************************************************************************************************************************************************/

static int ttf_inited = 0;

static FT_Library freetype;
static FT_Face face;

int TTFLoadFont(char * path, void * from_memory, int size_from_memory)
{
   
    if(!ttf_inited)
        FT_Init_FreeType(&freetype);
    ttf_inited = 1;

    if(path) {
        if(FT_New_Face(freetype, path, 0, &face)) return -1;
    } else {
        if(FT_New_Memory_Face(freetype, from_memory, size_from_memory, 0, &face)) return -1;
        }

    return 0;
}

/* release all */

void TTFUnloadFont()
{
   if(!ttf_inited) return;
   FT_Done_FreeType(freetype);
   ttf_inited = 0;
}

/* function to render the character

chr : character from 0 to 255

bitmap: u8 bitmap passed to render the character character (max 256 x 256 x 1 (8 bits Alpha))

*w : w is the bitmap width as input and the width of the character (used to increase X) as output
*h : h is the bitmap height as input and the height of the character (used to Y correction combined with y_correction) as output

y_correction : the Y correction to display the character correctly in the screen

*/

void TTF_to_Bitmap(u8 chr, u8 * bitmap, short *w, short *h, short *y_correction)
{
    FT_Set_Pixel_Sizes(face, (*w), (*h));
    
    FT_GlyphSlot slot = face->glyph;

    memset(bitmap, 0, (*w) * (*h));

    if(FT_Load_Char(face, (char) chr, FT_LOAD_RENDER )) {(*w) = 0; return;}

    int n, m, ww;

    *y_correction = (*h) - 1 - slot->bitmap_top;
    
    ww = 0;

    for(n = 0; n < slot->bitmap.rows; n++) {
        for (m = 0; m < slot->bitmap.width; m++) {

            if(m >= (*w) || n >= (*h)) continue;
            
            bitmap[m] = (u8) slot->bitmap.buffer[ww + m];
        }
    
    bitmap += *w;

    ww += slot->bitmap.width;
    }

    *w = ((slot->advance.x + 31) >> 6) + ((slot->bitmap_left < 0) ? -slot->bitmap_left : 0);
    *h = slot->bitmap.rows;
}

int Render_String_UTF8(u16 * bitmap, int w, int h, u8 *string, int sw, int sh)
{
    int posx = 0;
    int n, m, ww, ww2;
    u8 color;
    u32 ttf_char;

    FT_Set_Pixel_Sizes(face, sw, sh);
    FT_GlyphSlot slot = face->glyph;

    memset(bitmap, 0, w * h * 2);

    while(*string) {

        if(*string == 32 || *string == 9) {posx += sw>>1; string++; continue;}

        if(*string & 128) {
            m = 1;

            if((*string & 0xf8)==0xf0) { // 4 bytes
                ttf_char = (u32) (*(string++) & 3);
                m = 3;
            } else if((*string & 0xE0)==0xE0) { // 3 bytes
                ttf_char = (u32) (*(string++) & 0xf);
                m = 2;
            } else if((*string & 0xE0)==0xC0) { // 2 bytes
                ttf_char = (u32) (*(string++) & 0x1f);
                m = 1;
            } else {string++;continue;} // error!

             for(n = 0; n < m; n++) {
                if(!*string) break; // error!
                    if((*string & 0xc0) != 0x80) break; // error!
                    ttf_char = (ttf_char <<6) |((u32) (*(string++) & 63));
             }
           
            if((n != m) && !*string) break;
        
        } else ttf_char = (u32) *(string++);

        if(ttf_char == 13 || ttf_char == 10) ttf_char='/';

        if(FT_Load_Char(face, ttf_char, FT_LOAD_RENDER )==0) {
            ww = ww2 = 0;

            int y_correction = sh - 1 - slot->bitmap_top;
            if(y_correction < 0) y_correction = 0;
            ww2 = y_correction * w;

            for(n = 0; n < slot->bitmap.rows; n++) {
                if(n + y_correction >= h) break;
                for (m = 0; m < slot->bitmap.width; m++) {

                    if(m + posx >= w) continue;
                    
                    color = (u8) slot->bitmap.buffer[ww + m];
                    
                    if(color) bitmap[posx + m + ww2] = (color<<8) | 0xfff;
                }
            
            ww2 += w;

            ww += slot->bitmap.width;
            }
            
        }

        posx+= slot->bitmap.width;
    }
    return posx;
}
