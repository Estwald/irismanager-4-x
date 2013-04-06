#ifndef GFX_H
#define GFX_H

#include <tiny3d.h>
#include "libfont2.h"

void DrawAdjustBackground(u32 rgba);
void DrawBox(float x, float y, float z, float w, float h, u32 rgba);
void DrawTextBox(float x, float y, float z, float w, float h, u32 rgba);

float DrawButton1(float x, float y, float w, char * t, int select);
float DrawButton2(float x, float y, float w, char * t, int select);

float DrawButton1_UTF8(float x, float y, float w, char * t, int select);
float DrawButton2_UTF8(float x, float y, float w, char * t, int select);

void init_twat();
void update_twat();
void draw_twat(float x, float y, float angle);
void draw_twat2(float x, float y, float angle);

#endif