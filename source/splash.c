#include "utils.h"
#include "main.h"
#include "libfont2.h"
#include "gfx.h"

#include <math.h>
#include "spu_soundlib.h"

static int tablaseno[16384];
static int tablacoseno[16384];

#define  FLOAT_FIX (16384)
#define  PID 6.283185307179586476925286766559

static void init_tabsenofunc()
{
    int n;


	for(n = 0; n < 16384; n++) 
		tablaseno[n] = (int) ((double) FLOAT_FIX * sin((PID * (float) n) / 16384.0));
	

	for(n=0;n<16384;n++) 
		tablacoseno[n] = (int) ((double) FLOAT_FIX * cos((PID * (float) n) / 16384.0));	
}

static int sin_int(int ang)  // fast sin (ang=16384= 360 degrees)
{
	int n = ang;

	if( n < 0) n = 16384 - n;
	n &= 16383;

    return(tablaseno[n]);
}

static int cosin_int(int ang)
{
	int n=ang;

	if(n < 0) n = 16384 - n;
	n &= 16383;

    return(tablacoseno[n]);
}


static struct _fireworks {
    float x,y;
    int force;
    u32 color;
} fireworks[8];

static struct _particles_d {
    float dx,dy;
} particles_d[256];

s16 stars[256][2];

void init_fireworks()
{
    int n, m;
    float f;

    init_tabsenofunc();
        
    // fix random sequence

    srand(1);

    // fireworks init
    for(m = 0; m < 8; m++)
        fireworks[m].force=-1;

    // stars init

    for(n = 0; n < 256; n++) {
        
        int ang = rand() & 16383;
        
        stars[n][0] = (rand() % (848 + 128)) - 128;
        stars[n][1] = (rand() % 512);

        f = ((float)((1 + (rand() & 255)) * 2)) / 128.0f;

        particles_d[n].dx = f *((float) (sin_int((ang) & 16383)) / 16384.0f);
        particles_d[n].dy = f *((float) (cosin_int((ang) & 16383)) / 16384.0f);
    
    }

    SND_Pause(0);

}

#include "sound_ogg_bin.h"

// allocate voices for fireworks (from 8 to 15)

void snd_explo(int voice, int pos)
{
		
	if(SND_StatusVoice(8 + voice) == SND_UNUSED) {
	
        int l,r;

        // sound balance
		l=((pos - 848 / 2) <  100) ? 128 : 80;
		r=((pos - 848 / 2) > -100) ? 128 : 80;
        
        // play voice with 40 ms of delay (distance simulation of fireworks)
		SND_SetVoice(8 + voice, VOICE_MONO_16BIT,
            8000, 40, (void *) (sound_ogg_bin + 1), sound_ogg_bin_size, l, r, NULL);
	}

}

void fun_fireworks()
{
    int n,m;
    u32 color2;
    s16 xx,yy;
    float f;

    static u32 frame = 0;

    // draw the background stars
    

    for(n = 0; n < 256; n++)
    {

        xx = (s16) (stars[n][0]);
        yy = (s16) (stars[n][1]);

        if(n & 1) color2 = 0xffffffff;
        else color2 = 0xcfcfcfff;
        
        if((rand() & 31) != 1) // tililar
            {
        
            if(!(xx < 0 || xx >= 848 ||  yy < 0 ||  yy >= 512)) {

                tiny3d_SetPolygon(TINY3D_TRIANGLES);
                
                tiny3d_VertexPos(xx, yy, 65535.0f); // note pos is always the first element of the vertex
                tiny3d_VertexColor(color2);

                tiny3d_VertexPos(xx-1.0f, yy+1.0f, 65535.0f); // note color element is not necessary repeat here
                tiny3d_VertexPos(xx+1.0f, yy+1.0f, 65535.0f); 

                tiny3d_VertexPos(xx-1.0f, yy+0.25f, 65535.0f); // note pos is always the first element of the vertex

                tiny3d_VertexPos(xx+1.0f, yy+0.25f, 65535.0f); // note color element is not necessary repeat here
                tiny3d_VertexPos(xx, yy+1.25f, 65535.0f); 
                tiny3d_End();
            }
            
        }
    }

    

    DrawTextBox(-1024, -1024, -65535.0, 1, 1, 0);


    for(m = 0; m < 8; m++) {
                    
        if(fireworks[m].force <= -1 && ((rand() >> 8) & 15) == 0) {

            fireworks[m].x = 848 / 2 + (rand() & 511) - 255;
            fireworks[m].y = 512 / 2 + (rand() & 255) - 127;
            fireworks[m].force = 127;

            // set color for fireworks

            switch((rand() >> 8) % 5) {
                case 0:
                    fireworks[m].color = 0xffffff;
                break;

                case 1:
                    fireworks[m].color = 0xff2f2f;
                break;

                case 2:
                    fireworks[m].color = 0x2fff2f;
                break;

                case 3:
                    fireworks[m].color = 0xffff2f;
                break;

                case 4:
                    fireworks[m].color = 0x3f3fff;
                break;
                }

            // try assing voice to firework (0 to 7)
            //if(!pause_voices)
                 snd_explo(m, fireworks[m].x);

            }


        f = ((float) (128 - fireworks[m].force)) / 2.0f;
        
        if(fireworks[m].force >= 0) {
            int alpha;
                
            color2 = fireworks[m].color;
            
            alpha = fireworks[m].force;

            if(alpha < 0) alpha = 0;
            color2 = (color2 << 8) | ((alpha << 1) & 0xff);
            
            

            for(n = 0; n < 256; n++) {
                xx = (s16) (fireworks[m].x + particles_d[n].dx * f);
                yy = (s16) (fireworks[m].y + particles_d[n].dy * f);

                if(!(xx < 0 || xx >= 848 || yy < 0 ||  yy >= 512)) {
                    
                    tiny3d_SetPolygon(TINY3D_TRIANGLES);

                    tiny3d_VertexPos(xx, yy, 0.0f); // note pos is always the first element of the vertex
                    tiny3d_VertexColor(color2);

                    tiny3d_VertexPos(xx-1.0f, yy+1.0f, 0.0f); // note color element is not necessary repeat here
                    tiny3d_VertexPos(xx+1.0f, yy+1.0f, 0.0f); 

                    tiny3d_VertexPos(xx-1.0f, yy + 0.25f, 0.0f); // note pos is always the first element of the vertex

                    tiny3d_VertexPos(xx+1.0f, yy + 0.25f, 0.0f); // note color element is not necessary repeat here
                    tiny3d_VertexPos(xx, yy+1.25f, 0.0f); 
                    tiny3d_End();
                
                }
            
            }

        

        DrawTextBox(-1024, -1024, -65535.0, 1, 1, 0);

        if(frame & 1)
            fireworks[m].force -= 2;
        }

    }

    frame++;
}

static void txt_special(int text_x, int text_y, char *txt, u32 frame)
{
    int m;
   
    SetFontSize(20, 32);
    if(text_x <= -1024) 
        text_x = (848 - 20 * strlen(txt))/2;

    for( m = 0; m < strlen(txt); m++) {

        switch((m + (frame >> 2)) & 3) {
            case 0:
                SetFontColor(0x4fff4fff, 0x00000000);
            break;
            
            case 1:
                SetFontColor(0x4f4fffff, 0x00000000);
            break;
            
            case 2:
                SetFontColor(0xffff4fff, 0x00000000);
            break;
            
            case 3:
                SetFontColor(0xff4f4fff, 0x00000000);
            break;
        }

        int y= text_y + (4 * sin_int((((m +((frame >> 2) & 15)) * 2048)) & 16383)) / 16384;
        
        text_x = DrawFormatString(text_x, y, "%c", txt[m]);

    }
}

void splash()
{
    
    sleep(2);

    SetFontAutoCenter(0);
    SetFontColor(0xffffffff, 0x00000000);

}

void splash2(char *year)
{
    int n;

    if((new_pad | old_pad) & BUTTON_L1) return;

    init_fireworks();

    u32 frame = 0;

    
    for(n = 0; n < 25 * 60; n++)
    {

        cls2();
        frame++;
        fun_fireworks();

        SetFontAutoCenter(0);
        SetCurrentFont(FONT_BUTTON);

        txt_special(-1024, 512/2 - 96, "Merry Christmas!", frame);

        txt_special(-1024 , 512/2 - 32, "And", frame);

        txt_special(-1024, 512/2 + 32, "Happy New Year!", frame);

        txt_special(-1024, 512/2 + 96, year, frame);
        
        SetFontColor(0xffffffff, 0x00000001);

        tiny3d_Flip();

        ps3pad_read();
        if(new_pad & (BUTTON_CROSS | BUTTON_CIRCLE | BUTTON_TRIANGLE)) break;

    }
    
    SetFontAutoCenter(0);

    SetFontColor(0xffffffff, 0x00000000);

    cls2();
    tiny3d_Flip();



}
