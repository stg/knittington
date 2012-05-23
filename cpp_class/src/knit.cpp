// C++ wrapper class for library functions
//
// Requires inclusion of lib/src/*.c in project
//
// senseitg@gmail.com 2012-May-22

#include "main-class.h"

knit::knit() {
	machine_init();
}

machine_t *knit::driver_get(uint8_t index) {
	return machine_get(index);
}

bool knit::memory_load(char *path) {
	return machine_load(path);
}

bool knit::memory_save(char *path) {
	return machine_save(path);
}

void knit::emulate_start(char *device,bool verbose) {
	machine_emulate(device,verbose?stdout:NULL);
}

void knit::emulate_stop() {
	emulate_stop();
}