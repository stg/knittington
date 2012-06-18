#ifdef SDL2
#include <stdint.h>
#include <stdbool.h>
#include <FreeImage.h>
#include "loader.h"
#include <stdio.h> // todo remove
#include <string.h> // todo remove

#define ABS(n) (((n)<0)?(-(n)):((n)))

// byte order access fixes bug in FreeImage
#define RED(q)   (q->rgbBlue)
#define GREEN(q) (q->rgbGreen)
#define BLUE(q)  (q->rgbRed)

static FIBITMAP *dib=NULL;
static uint16_t w,h;
static RGBQUAD colors[16] = {
  {.rgbRed=0x00,.rgbGreen=0x00,.rgbBlue=0x00}, // 0 black
  {.rgbRed=0x00,.rgbGreen=0x00,.rgbBlue=0x7F}, // 1 dark blue
  {.rgbRed=0x00,.rgbGreen=0x7F,.rgbBlue=0x00}, // 2 dark green
  {.rgbRed=0x7F,.rgbGreen=0x00,.rgbBlue=0x00}, // 3 dark red
  {.rgbRed=0x00,.rgbGreen=0x00,.rgbBlue=0x00},
  {.rgbRed=0x00,.rgbGreen=0x00,.rgbBlue=0x00},
  {.rgbRed=0x00,.rgbGreen=0x00,.rgbBlue=0x00},
  {.rgbRed=0x00,.rgbGreen=0x00,.rgbBlue=0x00},
  {.rgbRed=0x00,.rgbGreen=0x00,.rgbBlue=0x00},
  {.rgbRed=0x00,.rgbGreen=0x00,.rgbBlue=0x00},
  {.rgbRed=0x00,.rgbGreen=0x00,.rgbBlue=0x00},
  {.rgbRed=0x00,.rgbGreen=0x00,.rgbBlue=0x00},
  {.rgbRed=0x00,.rgbGreen=0x00,.rgbBlue=0x00},
  {.rgbRed=0x00,.rgbGreen=0x00,.rgbBlue=0x00},
  {.rgbRed=0x00,.rgbGreen=0x00,.rgbBlue=0x00},
  {.rgbRed=0xFF,.rgbGreen=0xFF,.rgbBlue=0xFF}  // F white
};

bool image_loadpicture(const char* filename) {
  FREE_IMAGE_FORMAT fif=FIF_UNKNOWN;
  int flag=0;
  fif=FreeImage_GetFileType(filename,0);
  if(fif==FIF_UNKNOWN) {
    fif=FreeImage_GetFIFFromFilename(filename);
  }
  if((fif!=FIF_UNKNOWN)&&FreeImage_FIFSupportsReading(fif)) {
    dib=FreeImage_Load(fif,filename,flag);
    if(dib) {
      w=FreeImage_GetWidth(dib);
      h=FreeImage_GetHeight(dib);
      if(w>4000||h>4000) {
        FreeImage_Unload(dib);
        dib=NULL;
      } else {
        return true;
      }
    }
  }
  return false;
}

uint16_t image_width() {
  return w;
}

uint16_t image_height() {
  return h;
}

static uint8_t match_color(RGBQUAD *p_quad) {
  uint16_t diff,best=0xFFFF;
  uint8_t n,i=0x0F;
  for(n=0;n<16;n++) {
    diff = ABS((RED(p_quad))  -((int16_t)colors[n].rgbRed  ))
         + ABS((GREEN(p_quad))-((int16_t)colors[n].rgbGreen))
         + ABS((BLUE(p_quad)) -((int16_t)colors[n].rgbBlue ));
    if(diff<best) {
      best=diff;
      i=n;
    }
  }
  return i==0x0F?0xFF:i;
}

void image_convert(uint8_t *p_data) {
  uint16_t x,y;
  RGBQUAD quad;
  for(y=0;y<h;y++) {
    for(x=0;x<w;x++) {
      FreeImage_GetPixelColor(dib,x,y,&quad);
      p_data[(h-(y+1))*w+x]=match_color(&quad);
    }
  }
}

void image_free() {
  if(dib) FreeImage_Unload(dib);
  dib=NULL;
}

#endif