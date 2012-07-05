// Source file for picture loading and conversion
//
// Picture import uses FreeImage libray
// Integer RGB2HSV courtesy of http://en.literateprograms.org
// Full-color import algorithm described by Andrew Salomone
//
// senseitg@gmail.com 2012-Jul-02

#ifdef SDL2
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <FreeImage.h>
#include "picture.h"
#include <stdio.h> // todo remove
#include <string.h> // todo remove

#define ABS(n) (((n)<0)?(-(n)):((n)))
#define MIN3(x,y,z)  ((y) <= (z) ? ((x) <= (y) ? (x) : (y)) : ((x) <= (z) ? (x) : (z)))
#define MAX3(x,y,z)  ((y) >= (z) ? ((x) >= (y) ? (x) : (y)) : ((x) >= (z) ? (x) : (z)))

// byte order access fixes bug in FreeImage
#define RED(q)   ((q)->rgbBlue)
#define GREEN(q) ((q)->rgbGreen)
#define BLUE(q)  ((q)->rgbRed)

#define MAX_COLORS 8

typedef struct {
  uint8_t h;        /* Hue degree between 0 and 255 */
  uint8_t s;        /* Saturation between 0 (gray) and 255 */
  uint8_t v;        /* Value between 0 (black) and 255 */
} hsv_color;

typedef struct {
  uint8_t r, g, b;  /* Channel intensities between 0 and 255 */
} rgb_color;

typedef struct {
  bool      used;
  rgb_color color;
  uint8_t   memo;
} color_list;

static FIBITMAP *dib=NULL;
static uint16_t w,h,h_original;

static color_list color[MAX_COLORS];
static uint8_t color_mode;

static hsv_color rgb_to_hsv(rgb_color rgb) {
  uint8_t rgb_min, rgb_max;
  hsv_color hsv;
  //integer find min and max RGB value
  rgb_min = MIN3(rgb.r, rgb.g, rgb.b);
  rgb_max = MAX3(rgb.r, rgb.g, rgb.b);    
  //compute value
  hsv.v = rgb_max;
  if (hsv.v == 0) {
    hsv.h = hsv.s = 0;
  } else {
    //integer compute saturation
    hsv.s = 255*((uint16_t)(rgb_max - rgb_min))/hsv.v;
    if (hsv.s == 0) {
      hsv.h = 0;
    } else {
      //integer compute hue
      if (rgb_max == rgb.r) {
        hsv.h = 0 + 43*(rgb.g - rgb.b)/(rgb_max - rgb_min);
      } else if (rgb_max == rgb.g) {
        hsv.h = 85 + 43*(rgb.b - rgb.r)/(rgb_max - rgb_min);
      } else /* rgb_max == rgb.b */ {
        hsv.h = 171 + 43*(rgb.r - rgb.g)/(rgb_max - rgb_min);
      }
    }
  }
  return hsv;
}

// calculates "weight" of a color, used to determine memo order
// generally: bright colors first
//            colors of similar brightness as follows:
//              reds to greens via yellow
//              greens to blues via cyan
//              blues to reds via purple
static uint8_t weight(color_list* color) {
  hsv_color hsv = rgb_to_hsv(color->color);
  uint16_t h_weight = ((uint16_t)(0xFF-hsv.h) * (uint16_t)( hsv.s)) >> 4;
  uint16_t v_weight = ((uint16_t)hsv.v) << 7;
  return (h_weight + v_weight) >> 8;
}

// "liberal" color matcher
static bool match(RGBQUAD* quad, rgb_color* color) {
  uint8_t r, g, b;
  r = ABS((int16_t)RED(quad)   - (int16_t)color->r);
  g = ABS((int16_t)GREEN(quad) - (int16_t)color->g);
  b = ABS((int16_t)BLUE(quad)  - (int16_t)color->b);
  return (r < 16) && (g < 16) && (b < 16);
}

static uint8_t analyze() {
  uint16_t x, y;
  uint8_t c;
  uint8_t color_count = 0;
  uint8_t row_colors;
  uint16_t h_add = 0;
  RGBQUAD quad, quad_two;
  memset(color, 0, sizeof(color));
  color_list swap;
#ifdef DEBUG
  printf("analyzing %ix%i picture\n",w,h);
#endif  
  // find colors
  for(y = 0; y < h; y++) {
    for(x = 0; x < w; x++) {
      FreeImage_GetPixelColor(dib, x, y, &quad);
      for(c = 0; c < MAX_COLORS; c++) {
        if(color[c].used) {
          if(match(&quad, &color[c].color)) break;
        } else {
          color[c].used = TRUE;
          color[c].color.r = RED(&quad);
          color[c].color.g = GREEN(&quad);
          color[c].color.b = BLUE(&quad);
          color_count++;
          break;
        }
      }
      if(c == MAX_COLORS) return 1;
    }
  }
#ifdef DEBUG
  printf("found %i colors\n",color_count);
#endif
  // blank design? bail!
  if(color_count < 2) return 2;
  // sort colors
  for(x = 0; x < color_count - 1; x++) {
    for(y = x + 1; y < color_count; y++) {
      if(weight(&color[x]) < weight(&color[y])) {
        swap = color[x];
        color[x] = color[y];
        color[y] = swap;
      }
    }
  }
#ifdef DEBUG
  for(c = 0; c < color_count; c++) {
    printf("%02X%02X%02X weighs %i\n",color[c].color.r,color[c].color.g,color[c].color.b,weight(&color[c]));
  }
#endif
  // multi-color pattern?
  if(color_count > 2) {
    // set memo
    for(c = 0; c < color_count; c++) {
      color[c].memo = c + 1;
    }
    color_mode = 1;
    // check if full-color mode is required
    for(y = 0; y < h; y++) {
      row_colors = 1; // row always has main yarn
      for(c = 1; c < color_count; c++) {
        for(x = 0; x < w; x++) {
          FreeImage_GetPixelColor(dib, x, y, &quad);
          if(match(&quad, &color[c].color)) {
            row_colors++;
            break;
          }
        }
      }
      if(row_colors > 2) {
        // full-color required!
        color_mode = 2;
        break;
      }
    }
#ifdef DEBUG
  if(color_mode == 2) {
    printf("mode is full-color\n");
  } else {
    printf("mode is multi-color\n");
  }
#endif

    // is this a full-color pattern?
    if(color_mode == 2) {
      // not multiple of two? bail!
      if((h & 1) == 1) return 3;
      // calculate required height of converted full-color pattern
      for(y = 0; y < h; y += 2) {
        row_colors = 0;
        for(c = 0; c < color_count; c++) {
          for(x = 0; x < w; x++) {
            FreeImage_GetPixelColor(dib, x, y, &quad);
            FreeImage_GetPixelColor(dib, x, y + 1, &quad_two);
            if(match(&quad, &color[c].color) || match(&quad_two, &color[c].color)) {
              row_colors++;
              break;
            }
          }
        }
        h_add += (row_colors - 1) * 2; // every additional color needs two more rows
      }
      h_original = h;
      h += h_add; // increase height
#ifdef DEBUG
    printf("height expanded %i to %i\n",h_original,h);
#endif
    } else {
      color[0].memo = 0xFF;
    }
  } else {
#ifdef DEBUG
  printf("mode is single-color\n");
#endif
    color_mode = 0;
    color[0].memo = 0xFF;
  }
  return 0;
}

uint8_t picture_load(const char* filename) {
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
        return analyze();
        //return true;
      }
    }
  }
  return ;
}

uint16_t picture_width() {
  return w;
}

uint16_t picture_height() {
  return h;
}


static uint8_t match_color(RGBQUAD *p_quad) {
  uint8_t c;
  for(c = 0; c < MAX_COLORS; c++) {
    if(color[c].used) {
      if(match(p_quad, &color[c].color)) return color[c].memo;
    }
  }
  // we should NEVER get here
  exit(1);
}


void picture_convert(uint8_t *p_data) {
  uint16_t x, y, y_out = 0;
  uint8_t c;
  RGBQUAD quad, quad_two;
  if(color_mode != 2) {
    // single-/multi-color patterns
    for(y = 0; y < h; y++) {
      for(x = 0; x < w; x++) {
        FreeImage_GetPixelColor(dib,x,y,&quad);
        p_data[(h-(y+1))*w+x]=match_color(&quad);
      }
    }
  } else {
    // full-color patterns
    for(y = 0; y < h_original; y += 2) {
      for(c = 0; c < MAX_COLORS; c++) {
        if(color[c].used) {
          for(x = 0; x < w; x++) {
            FreeImage_GetPixelColor(dib, x, y, &quad);
            FreeImage_GetPixelColor(dib, x, y + 1, &quad_two);
            if(match(&quad, &color[c].color) || match(&quad_two, &color[c].color)) break;
          }
          if(x < w) {
            for(x = 0; x < w; x++) {
              FreeImage_GetPixelColor(dib,x,y,&quad);
              p_data[(h-(y_out+1))*w+x] = (match_color(&quad) == color[c].memo) ? color[c].memo : 0xFF;
            }
            y_out++;
            for(x = 0; x < w; x++) {
              FreeImage_GetPixelColor(dib,x,y+1,&quad);
              p_data[(h-(y_out+1))*w+x] = (match_color(&quad) == color[c].memo) ? color[c].memo : 0xFF;
            }
            y_out++;
          }
        }
      }
    }
  }
#ifdef DEBUG
  printf("conversion rows %i\n",y_out);
#endif  
}

void picture_free() {
  if(dib) FreeImage_Unload(dib);
  dib=NULL;
}

#endif