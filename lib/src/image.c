// Source file for image management functions
//
// Currently supports only proprietary RAW format
//
// senseitg@gmail.com 2012-May-22

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "image.h"
#include "string.h"

image_st *image_alloc(uint16_t width,uint16_t height,int explicit_memo) {

    // allocate structure and memory for image and memo
    image_st *image = (image_st*)calloc(1, sizeof(image_st) + width*height +
                                           height);
    // image goes right after the structure
    image->p_image = ((uint8_t*)image)+sizeof(image_st);
    // memo goes right after the image
    image->p_memo = image->p_image + (width*height);

    // store image parameters
    image->width = width;
    image->height = height;
    image->explicit_memo = explicit_memo;

    return image;
}

// print image to stdout
void image_print(image_st *image) {
	uint16_t x,y;
	uint8_t sample, memo;
	for(y=0;y<image->height;y++) {
		memo=0;
            for(x=0;x<image->width;x++) {
              sample=image_sample(image, x, y);
		  if(memo==0&&sample!=0xFF) memo=sample&0xF;
			putchar(sample!=0xFF?'X':'-');
		}
        if (image->explicit_memo)
            memo = image->p_memo[y];

		printf(" %01X\n",memo);
	}
}

// get pixel from image
uint8_t image_sample(image_st *image, uint16_t x, uint16_t y){
	return image->p_image[y*image->width + x];
}

// set pixel in image
void image_pset(image_st *image, uint16_t x, uint16_t y, uint8_t pixel){
	image->p_image[y*image->width + x] = pixel;
}

int _get_file_size(FILE *f)
{
    size_t bytes;
    fseek(f,0,SEEK_END);
    bytes=ftell(f);
    fseek(f,0,0);
    return bytes;
}
uint8_t _translate_memo(uint8_t img_memo)
{
    uint8_t blinking = img_memo & 0x80;
    uint8_t digit    = img_memo & 0x0F;


    if (blinking && digit<=4)
        return digit+0xA; 
    
    if (!blinking && digit<=9)
        return digit;

    fprintf(stderr,
            "WARNING: memo with blinking=%d and digit=%1X is not available.\n",
            blinking, digit);
    
    return 0;
}
image_st *image_read(FILE *f){
  uint16_t w, h, y;
  image_st *image;
  uint8_t *p_img;
  uint8_t *p_memo;
  size_t bytes;
  int explicit_memo;
  uint8_t temp[4];

  bytes = _get_file_size(f);

  // ensure header
  if(bytes<4) return NULL;
  // read header - 4 bytes
  fread(temp,1,4,f);

  explicit_memo = (temp[0] & 0x80) ? 1:0;

  // get size
  w = ((temp[0]<<8)|temp[1]) & 0x7fff;
  h = (temp[2]<<8)|temp[3];

  // ensure valid size
  if(w==0||h==0) return NULL;

  // ensure correct size
  if(bytes!=(4 + w*h + h*explicit_memo)) return NULL;

  // read picture - w*h bytes: top-down, left-right, 8bpp grayscale
  image = image_alloc(w, h, explicit_memo);
  p_img = image->p_image;
  p_memo = image->p_memo;
  for (y=0; y<h; y++)
  {
    // if explicit memo is enabled, first byte in the row is memo
    if (explicit_memo) {
        *p_memo++ = _translate_memo(fgetc(f));
    }
    // remaining bytes are image
    fread(p_img, 1, w, f);
    p_img += w;
  }
  return image;
}
