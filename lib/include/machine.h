// Header file for machine definitions
//
// senseitg@gmail.com 2012-May-22

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "image.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FLIPX
//#define FLIPY

#ifdef FLIPX
#define XCAL(X,W) (((W)-1)-(X))
#else
#define XCAL(X,W) (X)
#endif

#ifdef FLIPY
#define YCAL(Y,H) (((H)-1)-(Y))
#else
#define YCAL(Y,H) (Y)
#endif

// pattern info
typedef struct {
	uint16_t id;
	uint32_t location;
	uint16_t width;
	uint16_t height;
	uint32_t pattern;
} ptndesc_t;

// machine descriptor
typedef struct {
	char code[16];
  char *name;
  uint16_t (*memory_used)(ptndesc_t* p_desc);
  bool (*decode_header)(ptndesc_t* p_desc,uint8_t index);
  void (*decode_pattern)(ptndesc_t* p_desc, image_st *image);
  void (*format)(void);
  void (*set_track)(uint8_t track);
  uint8_t (*get_track)(void);
  bool (*size_check)(uint16_t width,uint16_t height);
  uint16_t (*add_pattern)(image_st *image);
  uint16_t (*free_memory)();
  uint16_t (*needed_memory)(uint16_t w,uint16_t h);
  void (*info)(FILE *output);
  uint16_t pattern_min;
  uint16_t pattern_max;
  uint16_t track_count;
  uint16_t memory;
} machine_t;

// Initialize all machines
// Must be called before machine_Get
void machine_init();

// Get machine descriptor with specific index
// Returns NULL if not found
machine_t *machine_get(uint8_t index);

// Load machine memory from disk
bool machine_load(char *path);

// Save machine memory to disk
bool machine_save(char *path);

// Start floppy drive emulator
bool machine_emulate(char *device,FILE *verbose,void (*fp_event)(uint8_t event,uint8_t data));

// Stop floppy drive emulator
void machine_emulate_stop();

#ifdef __cplusplus
}
#endif
