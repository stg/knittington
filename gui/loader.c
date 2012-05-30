#ifdef SDL2
#include <stdint.h>
#include <stdbool.h>
#include <FreeImage/FreeImage.h>
#include "loader.h"

static FIBITMAP *dib=NULL;
static uint16_t w,h;

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

void image_convert(uint8_t *p_data) {
  uint16_t x,y;
  RGBQUAD quad;
  for(y=0;y<h;y++) {
    for(x=0;x<w;x++) {
      FreeImage_GetPixelColor(dib,x,y,&quad);
      p_data[y*w+x]=(quad.rgbRed+quad.rgbGreen+quad.rgbBlue)/3;
    }
  } 
}

void image_free() {
  if(dib) FreeImage_Unload(dib);
  dib=NULL;
}

#endif