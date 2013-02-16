#include "gfx.h"

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <math.h>
#include "utils.h"


struct {
    float x, y, dx, dy, r, rs;

} m_twat[32];

void DrawAdjustBackground(u32 rgba)
{
   
    tiny3d_SetPolygon(TINY3D_LINE_LOOP);

    tiny3d_VertexPos(0  , 0  , 65535);
    tiny3d_VertexColor(rgba);

    tiny3d_VertexPos(847, 0  , 65535);

    tiny3d_VertexPos(847, 511, 65535);

    tiny3d_VertexPos(0  , 511, 65535);
    tiny3d_End();

    tiny3d_SetPolygon(TINY3D_LINES);

    tiny3d_VertexPos(0        , 0        , 65535);
    tiny3d_VertexColor(rgba);

    tiny3d_VertexPos(120      , 120      , 65535);

    tiny3d_VertexPos(847      , 0        , 65535);
    tiny3d_VertexPos(847 - 120, 120      , 65535);

    tiny3d_VertexPos(847, 511, 65535);
    tiny3d_VertexPos(847 - 120, 511 - 120, 65535);

    tiny3d_VertexPos(0  , 511, 65535);
    tiny3d_VertexPos(120      , 511 - 120, 65535);
    tiny3d_End();


}

void DrawBox(float x, float y, float z, float w, float h, u32 rgba)
{
    tiny3d_SetPolygon(TINY3D_QUADS);
       
    tiny3d_VertexPos(x    , y    , z);
    tiny3d_VertexColor(rgba);

    tiny3d_VertexPos(x + w, y    , z);

    tiny3d_VertexPos(x + w, y + h, z);

    tiny3d_VertexPos(x    , y + h, z);

    tiny3d_End();
}

void DrawTextBox(float x, float y, float z, float w, float h, u32 rgba)
{
    tiny3d_SetPolygon(TINY3D_QUADS);
    
   
    tiny3d_VertexPos(x    , y    , z);
    tiny3d_VertexColor(rgba);
    tiny3d_VertexTexture(0.0f , 0.0f);

    tiny3d_VertexPos(x + w, y    , z);
    tiny3d_VertexTexture(0.99f, 0.0f);

    tiny3d_VertexPos(x + w, y + h, z);
    tiny3d_VertexTexture(0.99f, 0.99f);

    tiny3d_VertexPos(x    , y + h, z);
    tiny3d_VertexTexture(0.0f , 0.99f);

    tiny3d_End();
}

void DrawLineBox(float x, float y, float z, float w, float h, u32 rgba)
{
    tiny3d_SetPolygon(TINY3D_LINE_LOOP);
       
    tiny3d_VertexPos(x    , y    , z);
    tiny3d_VertexColor(rgba);

    tiny3d_VertexPos(x + w, y    , z);

    tiny3d_VertexPos(x + w, y + h, z);

    tiny3d_VertexPos(x    , y + h, z);

    tiny3d_End();
}


float DrawButton1(float x, float y, float w, char * t, int select)
{
    int len = strlen(t);
    u32 rgba = 0xffffffff, brgba = 0x000000ff;

    if(w < (len + 2) * 12) w = (len + 2) * 12;

    SetFontAutoCenter(0);
    SetCurrentFont(FONT_BUTTON);
    SetFontSize(12, 32);

    if(select == 1) {rgba = 0x000000ff; brgba = 0xffffffff;}
        else
    if(select == -1) {rgba = 0x0000006f; brgba = 0xffffff6f;}
        
    SetFontColor(rgba, 0x0);

    DrawBox(x, y, 0.0f, w, 40, brgba);
    DrawLineBox(x, y, 0.0f, w, 40, rgba);

    DrawString(x + (w - len * 12) / 2, y + 4, t);

    return x + w;

}

float DrawButton2(float x, float y, float w, char * t, int select)
{
    int len = strlen(t);
    u32 rgba = 0xffffffff, brgba = 0x0000c080;

    if(w < (len + 2) * 10) w = (len + 2) * 10;

    SetFontAutoCenter(0);
    SetCurrentFont(FONT_BUTTON);
    SetFontSize(10, 32);

    if(select == 2) {rgba = 0xffffffff; brgba = 0xc01000ff;}
        else
    if(select == 1) {rgba = 0xffffffff; brgba = 0x00c000ff;}
        else
    if(select == -1) {rgba = 0xffffff6f; brgba = 0x00c0006f;}
        
    SetFontColor(rgba, 0x0);

    DrawBox(x, y, 0.0f, w, 40, brgba);
    DrawLineBox(x, y, 0.0f, w, 40, 0xc0c0c0ff);

    DrawString(x + (w - len * 10) / 2, y + 4, t);

    return x + w;

}

static void UTF8_to_Ansi(char *utf8, char *ansi, int len)
{
u8 *ch= (u8 *) utf8;
u8 c;

    *ansi = 0;

	while(*ch!=0 && len>0){

        // 3, 4 bytes utf-8 code 
        if(((*ch & 0xF1)==0xF0 || (*ch & 0xF0)==0xe0) && (*(ch+1) & 0xc0)==0x80){

        *ansi++=' '; // ignore
        len--;
        ch+=2+1*((*ch & 0xF1)==0xF0);
        
        }
        else 
        // 2 bytes utf-8 code	
        if((*ch & 0xE0)==0xc0 && (*(ch+1) & 0xc0)==0x80){
        
        c= (((*ch & 3)<<6) | (*(ch+1) & 63));

        *ansi++=c;
        len--;
        ch++;
	
	    } else {
	
            if(*ch<32) *ch=32;
            *ansi++=*ch;
            len--;
        }

	    ch++;
	}

	while(len>0) {
	    *ansi++=0;
	    len--;
	}
}

float DrawButton1_UTF8(float x, float y, float w, char * t, int select)
{
    char text[1024];

    UTF8_to_Ansi(t, text, 1024);
    return DrawButton1(x, y, w, text, select);
}

float DrawButton2_UTF8(float x, float y, float w, char * t, int select)
{
    char text[1024];

    UTF8_to_Ansi(t, text, 1024);
    return DrawButton2(x, y, w, text, select);
}

void init_twat()
{
    int i;

    for(i = 0; i < 32; i++) {
        m_twat[i].x = (rand() % 640) + 104;
        m_twat[i].y = (rand() % 300) + 106;

        m_twat[i].dx = ((float) ((int) (rand() & 7) - 3)) / 12.0f;
        m_twat[i].dy = ((float) ((int) (rand() & 7) - 3)) / 12.0f;
        m_twat[i].r = 0;
        m_twat[i].rs = ((float) ((int) (rand() & 7) - 3)) / 80.0f;
    }
}

void update_twat()
{
    int i;

    for(i = 0; i < 32; i++) {

        if((rand() & 0x1ff) == 5) {
            m_twat[i].dx = ((float) ((int) (rand() & 7) - 3)) / 12.0f;
            m_twat[i].dy = ((float) ((int) (rand() & 7) - 3)) / 12.0f;
            m_twat[i].rs = ((float) ((int) (rand() & 7) - 3)) / 80.0f;
            
        }

        if(m_twat[i].dx == 0.0f && m_twat[i].dy == 0.0f) {m_twat[i].dy = 0.25f; m_twat[i].dx = (rand() & 1) ? 0.25f : -0.25f;}
        if(m_twat[i].rs == 0.0f) m_twat[i].rs = (rand() & 1) ? .001f : -0.001f;
        
        m_twat[i].x += m_twat[i].dx;
        m_twat[i].y += m_twat[i].dy;
        m_twat[i].r += m_twat[i].rs;
    
        if(i & 1) {
            if(m_twat[i].x < 0)  {
                if(m_twat[i].dx < 0) m_twat[i].dx = -m_twat[i].dx;
            }

            if(m_twat[i].y < 0)  {
                if(m_twat[i].dy < 0) m_twat[i].dy = -m_twat[i].dy;
            }

            if(m_twat[i].x >= 600)  {
                if(m_twat[i].dx > 0) m_twat[i].dx = -m_twat[i].dx;
            }

            if(m_twat[i].y >= 480)  {
                if(m_twat[i].dy > 0) m_twat[i].dy = -m_twat[i].dy;
            }
        } else {
            if(m_twat[i].x < 248)  {
                if(m_twat[i].dx < 0) m_twat[i].dx = -m_twat[i].dx;
            }

            if(m_twat[i].y < 32)  {
                if(m_twat[i].dy < 0) m_twat[i].dy = -m_twat[i].dy;
            }

            if(m_twat[i].x >= 848)  {
                if(m_twat[i].dx > 0) m_twat[i].dx = -m_twat[i].dx;
            }

            if(m_twat[i].y >= 512)  {
                if(m_twat[i].dy > 0) m_twat[i].dy = -m_twat[i].dy;
            }
        
        }
        
        draw_twat(m_twat[i].x, m_twat[i].y, m_twat[i].r);
    }
    
}

void draw_twat(float x, float y, float angle)
{
    int n;

    float ang, angs = 6.2831853071796 / 8, angs2 = 6.2831853071796 / 32;

    MATRIX matrix;
    
    // rotate and translate the sprite
    matrix = MatrixRotationZ(angle);
    matrix = MatrixMultiply(matrix, MatrixTranslation(x , y , 65535.0f));

    // fix ModelView Matrix
    tiny3d_SetMatrixModelView(&matrix);

    tiny3d_SetPolygon(TINY3D_TRIANGLES);

    ang = 0.0f;

    for(n = 0; n <8; n++) {

        tiny3d_VertexPos(4.0f *sinf(ang), 4.0f *cosf(ang), 0);
        tiny3d_VertexColor(0xffffff30);
        tiny3d_VertexPos(7.0f *sinf(ang+angs/2), 7.0f *cosf(ang+angs/2), 0);
        tiny3d_VertexColor(0xff00ff40);
        tiny3d_VertexPos(4.0f *sinf(ang+angs), 4.0f *cosf(ang+angs), 0);
        tiny3d_VertexColor(0xffffff30);

        ang += angs;
    }

    tiny3d_End();

    tiny3d_SetPolygon(TINY3D_POLYGON);

    ang = 0.0f;

    for(n = 0; n <32; n++) {
        tiny3d_VertexPos(3.0f * sinf(ang), 3.0f * cosf(ang), 0);
        if(n & 1) tiny3d_VertexColor(0x80ffff40); else tiny3d_VertexColor(0xffffff40);
        ang += angs2;
    }

    tiny3d_End();

    tiny3d_SetMatrixModelView(NULL); // set matrix identity

}

