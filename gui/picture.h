#ifdef SDL2
#include <stdint.h>
#include <stdbool.h>
enum {
  PIC_OK,         // OK
  PIC_FORMAT,     // UNRECOGNIZED FILE FORMAT
  PIC_LOADING,    // UNABLE TO DECODE FILE
  PIC_CONVERT,    // PICTURE CONVERSION FAILED
  PIC_OVERFLOW,   // TOO MANY COLORS
  PIC_UNDERFLOW,  // ONLY ONE COLOR
  PIC_FULLHEIGHT, // FULL-COLOR MUST BE MULTIPLE OF 2 HIGH
  PIC_SIZE,       // PICTURE IS (WAY) TOO BIG
} pic_error_e;

enum {
  PIC_SINGLE,     // SINGLE-COLOR
  PIC_MULTI,      // MULTI-COLOR
  PIC_FULL        // FULL-COLOR
} pic_mode_e;

uint8_t picture_mode();
uint8_t picture_error();
uint8_t picture_load(const char* filename);
uint16_t picture_width();
uint16_t picture_height();
void picture_convert(uint8_t *p_data);
void picture_free();
#endif