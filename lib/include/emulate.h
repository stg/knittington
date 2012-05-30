// Header for Tandy PDD1 floppy drive emulator
//
// senseitg@gmail.com 2012-May-22

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

enum {
  EMU_ERROR,    // error        data=errcode
  EMU_OPMODE,   // op-mode req. data=0
  EMU_FORMAT,   // format req.  data=0:ok 1:wrong code
  EMU_READID,   // read id      data=sector
  EMU_WRITEID,  // write id     data=sector
  EMU_READ,     // read         data=sector
  EMU_WRITE,    // write        data=sector
  EMU_FIND,     // search       data=80:not found <80:found
  EMU_READY     // ready        data=0
} emu_event_e;

enum {
  EMU_ERR_WRITE,  // serial write error
  EMU_ERR_CMD,    // unsupported command
  EMU_ERR_SECTOR, // bad sector id
  EMU_ERR_CFG,    // serial port configuration fail
  EMU_ERR_OPEN,   // serial port open fail
  EMU_ERR_CLOSE,  // serial port close fail
  EMU_ERR_DATA    // unexpected data
} emu_err_event_e;

// start emulate floppy drive
// device specifies device for sopen, see serial.c/h
// verbose specifies FILE* for verbose output, such as stdout
// emulate is blocking
bool emulate(char *device,uint8_t *p_sect_data,uint8_t *p_sids_data,FILE *verbose,void (*fp_event)(uint8_t event,uint8_t data));

// stop emulating floppy drive
// since emulate is blocking, this must be called from a separate process
void emulate_stop();

#ifdef __cplusplus
}
#endif
