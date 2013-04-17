#include "gfx.h"

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <math.h>
#include "utils.h"
#include "ttf_render.h"

struct {
    float x, y, dx, dy, r, rs;

} m_twat[6];

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

static void DrawBox2(float x, float y, float z, float w, float h, u32 rgba, u32 rgba2)
{
    tiny3d_SetPolygon(TINY3D_QUADS);
       
    tiny3d_VertexPos(x    , y    , z);
    tiny3d_VertexColor(rgba);

    tiny3d_VertexPos(x + w, y    , z);
    tiny3d_VertexColor(rgba2);

    tiny3d_VertexPos(x + w, y + h, z);
    tiny3d_VertexColor(rgba);

    tiny3d_VertexPos(x    , y + h, z);
    tiny3d_VertexColor(rgba2);

    tiny3d_End();
}

/*

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

*/

float DrawButton1_UTF8(float x, float y, float w, char * t, int select)
{
    int len = 0;
    u32 rgba = 0xffffffff, brgba = 0x101010ff;
    u32 brgba2 = 0x404040ff;

    set_ttf_window(0, 0, 848, 512, WIN_SKIP_LF);
    len = display_ttf_string(0, 0, t, 0, 0, 16, 32);

    if(w < (len + 24)) w = (len + 24);

    if(select == 1) {rgba = 0x000000ff; brgba2 = 0xc0c0c0ff;brgba = 0xffffffff;}
        else
    if(select == -1) {rgba = 0x0000006f; brgba = brgba2 = 0xffffff6f;}
        

    DrawBox2(x, y, 0.0f, w, 40, brgba, brgba2);
    DrawLineBox(x, y, 0.0f, w, 40, rgba);

    //DrawString(x + (w - len * 12) / 2, y + 4, t);

    display_ttf_string(x + (w - len) / 2, y + 4, t, rgba, 0, 16, 32);

    return x + w;

}

float DrawButton2_UTF8(float x, float y, float w, char * t, int select)
{
    int len = 0;
    u32 rgba = 0xffffffff, brgba = 0x0030d0ff;
    u32 brgba2 = 0x003080ff;

    set_ttf_window(0, 0, 848, 512, WIN_SKIP_LF);
    len = display_ttf_string(0, 0, t, 0, 0, 14, 32);

    if(w < (len + 20)) w = (len + 20);

    if(select == 2) {rgba = 0xffffffff; brgba = 0xd01000ff;brgba2 = 0x801000ff;}
        else
    if(select == 1) {rgba = 0xffffffff; brgba = 0x00d000ff;brgba2 = 0x008030ff;}
        else
    if(select == -1) {rgba = 0xffffff6f; brgba= 0x0040006f; brgba2 = 0x00d0006f;}
        
    DrawBox2(x, y, 0.0f, w, 40, brgba, brgba2);
    DrawLineBox(x, y, 0.0f, w, 40, 0xc0c0c0ff);

    //DrawString(x + (w - len * 10) / 2, y + 4, t);
    display_ttf_string(x + (w - len) / 2, y + 4, t, rgba, 0, 14, 32);

    return x + w;

}


void init_twat()
{
    m_twat[0].r = m_twat[1].r = m_twat[2].r = 0.0f;
    m_twat[3].r = m_twat[4].r = m_twat[5].r = 0.0f;
}

void update_twat()
{
    
    float x = 840.0f - 170.0f, y = 512.0f - 180.0f;

    m_twat[0].r += .01f;
    m_twat[1].r -= .01f;
    m_twat[2].r += .01f;

    draw_twat(x + 70.0f, y - 70.0f, m_twat[0].r + 0.196349540875f);
    draw_twat(x, y, m_twat[1].r);
    draw_twat(x + 70.0f, y + 70.0f, m_twat[2].r + 0.196349540875f);


    x= 240.0f; y = 512.0f - 120.0f;
    draw_twat2(x + 35.0f, y - 35.0f, m_twat[0].r + 0.196349540875f);
    draw_twat2(x, y, m_twat[1].r);
    draw_twat2(x + 35.0f, y + 35.0f, m_twat[2].r + 0.196349540875f);

    x= 848 - 280.0f; y = 512.0f - 120.0f;
    draw_twat2(x, y + 35.0f, m_twat[2].r + 0.196349540875f);


    x = 840.0f - 170.0f; y = 180.0f + 50.0f;
    draw_twat(x + 70.0f, y - 70.0f, m_twat[1].r);


    x = 848.0f / 2.0f - 70.0f; y = 512.0f / 2.0f;

    draw_twat(x + 70.0f, y - 70.0f, m_twat[0].r + 0.196349540875f);
    draw_twat(x, y, m_twat[1].r);
    draw_twat(x + 70.0f, y + 70.0f, m_twat[2].r + 0.196349540875f);

    x = 100.0f; y = 200.0f;

    m_twat[3].r -= .01f;
    m_twat[4].r += .01f;
    m_twat[5].r -= .01f;

    draw_twat(x, y - 70.0f, m_twat[3].r + 0.196349540875f);
    draw_twat(x + 70.0f, y, m_twat[4].r);
    draw_twat(x, y + 70.0f, m_twat[5].r + 0.196349540875f);

    x= 848 - 280.0f; y = 140.0f;

    draw_twat2(x, y - 35.0f, m_twat[3].r + 0.196349540875f);
    draw_twat2(x + 35.0f, y, m_twat[4].r);
    draw_twat2(x, y + 35.0f, m_twat[5].r + 0.196349540875f);

    x= 215.0f + 60.0f; y = 140.0f;

    draw_twat2(x, y - 35.0f, m_twat[3].r + 0.196349540875f);

    x = 100.0f; y = 512.0f - 170.0f - 40.0f;
    draw_twat(x, y + 70.0f, m_twat[4].r);
}

void draw_twat(float x, float y, float angle)
{
    int n;

    float ang, angs = 6.2831853071796 / 16, angs2 = 6.2831853071796 / 32;

    MATRIX matrix;
    
    // rotate and translate the sprite
    matrix = MatrixRotationZ(angle);
    matrix = MatrixMultiply(matrix, MatrixTranslation(x , y , 65535.0f));

    // fix ModelView Matrix
    tiny3d_SetMatrixModelView(&matrix);

    tiny3d_SetPolygon(TINY3D_TRIANGLES);

    ang = 0.0f;

    for(n = 0; n < 16; n++) {

        tiny3d_VertexPos(40.0f *sinf(ang), 40.0f *cosf(ang), 1000);
        tiny3d_VertexColor(0xffffff18);
        tiny3d_VertexPos(58.0f *sinf(ang+angs/2), 58.0f *cosf(ang+angs/2), 1000);
        tiny3d_VertexColor(0xffffff18);
        tiny3d_VertexPos(40.0f *sinf(ang+angs), 40.0f *cosf(ang+angs), 1000);
        tiny3d_VertexColor(0xffffff18);

        ang += angs;
    }

    tiny3d_End();

    tiny3d_SetPolygon(TINY3D_POLYGON);

    ang = 0.0f;

    tiny3d_VertexPos(0.0f * sinf(ang), 0.0f * cosf(ang), 1000);
    tiny3d_VertexColor(0xffffff18);
    for(n = 0; n < 33; n++) {
        tiny3d_VertexPos(30.0f * sinf(ang), 30.0f * cosf(ang), 1000);
        if(n & 1) tiny3d_VertexColor(0x80ffff18); else tiny3d_VertexColor(0xffffff18);
        ang += angs2;
    }

    tiny3d_End();

    tiny3d_SetMatrixModelView(NULL); // set matrix identity

}

void draw_twat2(float x, float y, float angle)
{
    int n;

    float ang, angs = 6.2831853071796 / 16, angs2 = 6.2831853071796 / 32;

    MATRIX matrix;
    
    // rotate and translate the sprite
    matrix = MatrixRotationZ(angle);
    matrix = MatrixMultiply(matrix, MatrixTranslation(x , y , 65535.0f));

    // fix ModelView Matrix
    tiny3d_SetMatrixModelView(&matrix);

    tiny3d_SetPolygon(TINY3D_TRIANGLES);

    ang = 0.0f;

    for(n = 0; n < 16; n++) {

        tiny3d_VertexPos(20.0f *sinf(ang), 20.0f *cosf(ang), 1000);
        tiny3d_VertexColor(0xffffff18);
        tiny3d_VertexPos(28.0f *sinf(ang+angs/2), 28.0f *cosf(ang+angs/2), 1000);
        tiny3d_VertexColor(0xffffff18);
        tiny3d_VertexPos(20.0f *sinf(ang+angs), 20.0f *cosf(ang+angs), 1000);
        tiny3d_VertexColor(0xffffff18);

        ang += angs;
    }

    tiny3d_End();

    tiny3d_SetPolygon(TINY3D_POLYGON);

    ang = 0.0f;

    tiny3d_VertexPos(0.0f * sinf(ang), 0.0f * cosf(ang), 1000);
    tiny3d_VertexColor(0xffffff18);
    for(n = 0; n < 33; n++) {
        tiny3d_VertexPos(15.0f * sinf(ang), 15.0f * cosf(ang), 1000);
        if(n & 1) tiny3d_VertexColor(0x80ffff18); else tiny3d_VertexColor(0xffffff18);
        ang += angs2;
    }

    tiny3d_End();

    tiny3d_SetMatrixModelView(NULL); // set matrix identity

}

