// C++ wrapper class for library functions
//
// senseitg@gmail.com 2012-May-22

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "machine.h"
#include "emulate.h"

class knit {
public:
    knit();
		machine_t *driver_get(uint8_t index);
		bool memory_load(char *path);
		bool memory_save(char *path);
		void emulate_start(char *device,bool verbose);
		void emulate_stop();
};