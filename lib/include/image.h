// Header file for image management functions
//
// senseitg@gmail.com 2012-May-22
// miguelangel@ajo.es 2015-Ago-22
//

#ifndef __IMAGE_H
#define __IMAGE_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t *p_image;
    uint8_t *p_memo;
    int explicit_memo;
    uint16_t width;
    uint16_t height;
} image_st;

// allocate memory for image
image_st *image_alloc(uint16_t width,uint16_t height,int explicit_memo);

// print image to stdout
void image_print(image_st *image);

// get pixel from image
uint8_t image_sample(image_st *image,uint16_t x,uint16_t y);

// set pixel in image
void image_pset(image_st *image,uint16_t x,uint16_t y,uint8_t pixel);

// read image file
image_st *image_read(FILE *f);

#ifdef __cplusplus
}
#endif

#endif
