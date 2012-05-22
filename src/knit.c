// Source file for the disk image manager
// Compatible with Brother Electroknit KH940 knitting machine
// See README for HOW TO USE
//
// senseitg@gmail.com 2012-May-17

// PUBLIC is for possible addition to library/OF C++ class
// These are requested by Mar Canet

//== DECLARATIONS ====================================================

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>

// nibble access
#define MSN(B) (((unsigned char)B)>>4)
#define LSN(B) (((unsigned char)B)&0x0F)

// pattern info
typedef struct {
	uint16_t id;
	uint16_t width;
	uint16_t height;
	uint32_t offset;
	uint32_t pos;
} hdr_t;

// header mem
static hdr_t ptn;
static uint8_t data[81920];
static uint8_t *p_track=data;
static uint8_t sids[80][12];

// command memory
static char cmds[8192]={0};
static char cmd[256];

// flags
static bool halt=false,stop=false;


//== SERIAL STUFF ====================================================

#ifdef _WIN32

#include <stdio.h>
#include <windows.h>

static HANDLE h_serial;

// windows - open serial port
// device has form "COMn"
// returns true if successful
static bool sopen(char* device) {
	h_serial=CreateFile(device,GENERIC_READ|GENERIC_WRITE,0,0,OPEN_EXISTING,0,0);
	return h_serial!=INVALID_HANDLE_VALUE;
}

// windows - configure serial port
// fmt has form "baud,parity,databits,stopbit", ie: "9600,N,8,1"
// returns true if successful
static bool sconfig(char* fmt) {
	DCB dcb;
	COMMTIMEOUTS cmt;
  // clear dcb  
	memset(&dcb,0,sizeof(DCB));
	dcb.DCBlength=sizeof(DCB);
	// configure serial parameters
	if(!BuildCommDCB(fmt,&dcb)) return false;
	dcb.fOutxCtsFlow=0;
	dcb.fOutxDsrFlow=0;
	dcb.fDtrControl=0;
	dcb.fOutX=0;
	dcb.fInX=0;
	dcb.fRtsControl=0;
	if(!SetCommState(h_serial,&dcb)) return false;
	// configure buffers
	if(!SetupComm(h_serial,1024,1024)) return false;
	// configure timeouts	
	cmt.ReadIntervalTimeout=100;
	cmt.ReadTotalTimeoutMultiplier=100;
	cmt.ReadTotalTimeoutConstant=100;
	cmt.WriteTotalTimeoutConstant=100;
	cmt.WriteTotalTimeoutMultiplier=100;
	if( !SetCommTimeouts(h_serial,&cmt)) return false;
	return true;
}

// windows - read from serial port
// returns bytes actually read
static int32_t sread(void *p_read,uint16_t i_read) {
	DWORD i_actual=0;
	if(!ReadFile(h_serial,p_read,i_read,&i_actual,NULL)) return -1;
	return (int32_t)i_actual;
}

// windows - write to serial port
// returns bytes actually written
static int32_t swrite(void* p_write,uint16_t i_write) {
	DWORD i_actual=0;
	if(!WriteFile(h_serial,p_write,i_write,&i_actual,NULL)) return -1;
	return (int32_t)i_actual;
}

// windows - close serial port
// returns true if successful
static bool sclose() {
	return CloseHandle(h_serial)!=0;
}

#else

#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

static int h_serial;
static struct termios restore;

// posix - open serial port
// device has form "/dev/ttySn"
// returns true if successful
static bool sopen(char* device) {
	h_serial=open(device,O_RDWR|O_NOCTTY|O_NDELAY);
	return h_serial>=0;
}

// posix - configure serial port
// fmt has form "baud,parity,databits,stopbit", ie: "9600,N,8,1"
// returns true if successful
static bool sconfig(char* fmt) {
	struct termios options;
	char* argv[4];
	unsigned char argc;
	char* p_parse;
	// quick and dirty parser
	p_parse=fmt;
	argc=1;
	argv[0]=fmt;
	while(*p_parse!=0) {
		if(*p_parse==',') {
			*p_parse=0;
			if(argc>3) return false;
			argv[argc++]=++p_parse;
		} else p_parse++;
	}
  // get current settings
  tcgetattr(h_serial,&options);
	memcpy(&restore,&options,sizeof(options));
	// configure baudrate
	switch(atoi(argv[0])) {
		case   1200: cfsetispeed(&options,B1200  ); cfsetospeed(&options,B1200  ); break;
		case   2400: cfsetispeed(&options,B2400  ); cfsetospeed(&options,B2400  ); break;
		case   4800: cfsetispeed(&options,B4800  ); cfsetospeed(&options,B4800  ); break;
		case   9600: cfsetispeed(&options,B9600  ); cfsetospeed(&options,B9600  ); break;
		case  19200: cfsetispeed(&options,B19200 ); cfsetospeed(&options,B19200 ); break;
		case  38400: cfsetispeed(&options,B38400 ); cfsetospeed(&options,B38400 ); break;
		case  57600: cfsetispeed(&options,B57600 ); cfsetospeed(&options,B57600 ); break;
		case 115200: cfsetispeed(&options,B115200); cfsetospeed(&options,B115200); break;
		default: return false;
	}
	// configure parity
	switch(argv[1][0]) {
		case 'n': case 'N': options.c_cflag&=~PARENB;                           break;
		case 'o': case 'O': options.c_cflag|= PARENB; options.c_cflag|= PARODD; break;
		case 'e': case 'E': options.c_cflag|= PARENB; options.c_cflag&=~PARODD; break;
		default: return false;
	}
	// configure data bits
	options.c_cflag&=~CSIZE;
	switch(argv[2][0]) {
		case '8': options.c_cflag&=~CSIZE; options.c_cflag|=CS8; break;
		case '7': options.c_cflag&=~CSIZE; options.c_cflag|=CS7; break;
		default: return false;
	}
	// configure stop bits
	switch(argv[3][0]) {
		case '1': options.c_cflag&=~CSTOPB; break;
		case '2': options.c_cflag|= CSTOPB; break;
		default: return false;
	}	
	// configure timeouts
	options.c_cc[VMIN]=0;
	options.c_cc[VTIME]=1;
	options.c_cflag|=CLOCAL|CREAD;
	return tcsetattr(h_serial,TCSANOW,&options)==0;
}

// posix - read from serial port
// returns bytes actually read
static int32_t sread(void* p_read,uint16_t i_read) {
	return (int32_t)read(h_serial,p_read,(int)i_read);
}

// posix - write to serial port
// returns bytes actually written
static int32_t swrite(void* p_write,uint16_t i_write) {
	return (int32_t)write(h_serial,p_write,(int)i_write);
}

// posix - close serial port
// returns true if successful
static bool sclose() {
  if(h_serial>=0) {
	  tcsetattr(h_serial,TCSANOW,&restore); // restore original configuration
	  return close(h_serial)==0;
  }
  return false;
}

#endif


//== UTIL FUNCTIONS ==================================================

// command input routine that handles several
// commands in one input as well as strings
static bool read_cmd(const char *fail) {
	char *ret;
	char *p_src,*p_dst;
	uint16_t len;
	bool out=false;
	if(strlen(cmds)==0) {
		if(fgets(cmds,sizeof(cmds),stdin)==NULL) {
		  strcpy(cmds,fail);
		  out=true;
		} else {
		  len=strlen(cmds);
		  if(cmds[len-1]=='\n') cmds[len-1]=0;
		}
		//count leading spaces
		p_src=cmds;
		while(*p_src==' ') p_src++;
		if(*p_src==0) {
			//just spaces
			cmds[0]=0;
		} else {
			//trim leading spaces
			memmove(cmds,p_src,strlen(p_src)+1);
			//trim trailing spaces
			p_src=cmds+strlen(cmds)-1;
			while(*p_src==' ')*p_src--=0;
		}
	} else {
		out=true;
	}
	//have nothing
	if(strlen(cmds)==0) return false;
	p_dst=cmd;
	p_src=cmds;
	while(*p_src==' ') p_src++;
	if(*p_src=='"') {
		p_src++;
		while(*p_src!='"'&&*p_src!=0) *p_dst++=*p_src++;
		if(*p_src=='"')p_src++;
	} else {
		while(*p_src!=' '&&*p_src!=0) *p_dst++=*p_src++;
	}
	*p_dst=0;
	if(out)puts(cmd);
	memmove(cmds,p_src,strlen(p_src)+1);
	return true;
}

// ctrl^c handler
static void sigint(int z) {
  stop=true;
}

// prints hex data in traditional 16 column format
static void print_hex(unsigned char* mem, uint32_t length) {
	uint8_t count=0;
	while(length--) {
		printf("%02X",*mem++);
		count=(count+1)&0x0F;
		if(count) {
			putchar(' ');
		} else {
			printf("\n");
		}
	}
	if(count) printf("\n");
}

// prints hex data in slim format
static void print_slim_hex(unsigned char* mem, uint32_t length) {
	while(length--) {
		printf("%02X",*mem++);
	}
}

// set bit in *p_data @ bit pointer bp
static void bit_set(uint8_t *p_mem,uint32_t bp){
	p_mem[bp>>3]|=0x80>>(bp&7);
}

// clear bit in *p_data @ bit pointer bp
static void bit_clr(uint8_t *p_mem,uint32_t bp){
	p_mem[bp>>3]&=~(0x80>>(bp&7));
}

// get bit in *p_data @ bit pointer bp
static bool bit_get(uint8_t *p_mem,uint32_t bp){
	return (p_mem[bp>>3]&(0x80>>(bp&7)))!=0;
}

// get nibble in data[] @ nibble pointer np
static uint8_t nib_get(uint32_t np) {
	uint8_t byte=p_track[np>>1];
	return (np&1)?LSN(byte):MSN(byte);
}

// set nibble in data[] @ nibble pointer np
static void nib_set(uint32_t np,uint8_t val) {
	if(np&1) {
		p_track[np>>1]=(p_track[np>>1]&0xF0)|(val);
	} else {
		p_track[np>>1]=(p_track[np>>1]&0x0F)|(val<<4);
	}
}

// get big-endian int16 in data[] @ byte pointer p
static uint16_t int_get(uint32_t p) {
	return (p_track[p+0]<<8)|(p_track[p+1]<<0);
}

// set big-endian int16 in data[] @ byte pointer p
static void int_set(uint32_t p,uint32_t val) {
	p_track[p+0]=(val>>8)&0x00FF;
	p_track[p+1]=(val>>0)&0x00FF;
}


//== FLOPPY DRIVE EMULATION ==========================================

// serial write with error printing
static int32_t eswrite(void* p_write,uint16_t i_write) {
	if(swrite(p_write,i_write)!=i_write) printf("unable to write to serial port\n");
}

// op mode command executer
void exec_op(uint8_t cmd[]) {
	if(cmd[0]==0x08) {
		printf("[op mode] recv 0x08 [fdc mode  ]\n");
	} else {
		printf("[op mode] recv 0x%02X bad/unsupported command\n",cmd[0]);
	}
}

// returns pointer to name of fdc command
static const char* fdc_name(uint8_t cmd) {
	switch(cmd) {
		case 'A':           return "read id   ";
		case 'R':           return "read sect ";
		case 'S':           return "find sect ";
		case 'B': case 'C': return "write id  ";
		case 'W': case 'X': return "write sect";
		default: return "?";
	}
}

// make hex char
static char hexchar(uint8_t c) {
  if(c<10)return '0'+c;
  return 'A'+c-10;
}

// sets successful response code
static void fdc_ok(char *p_ret,uint8_t sect) {
  strcpy(p_ret,"00000000");
  p_ret[2]=hexchar(sect>>4);
  p_ret[3]=hexchar(sect&0xF);
}

// fdc mode command executer
static uint16_t exec_fdc(uint8_t cmd[0]) {
	char ret[9]="80000000";
	uint16_t count=0;
	uint8_t n;
	if(cmd[0]=='F'||cmd[0]=='G') {
		// check length code (but format anyways)
		if(cmd[1]!=5) printf("recv %02X [format    ] unsupported length code 0x%02X\n",cmd[0],cmd[1]);
		// format memory
		memset(data,0x00,80*1024);
		for(n=0;n<80;n++) memset(&sids[n][0],0x00,12);
		// respond ok
		strcpy(ret,"00000000");
		printf("[fdc emu] exec 0x%02X [format    ] resp: %s\n",cmd[0],ret);
	} else {
		if(cmd[1]==0xFF)cmd[1]=0; // physical sector default
		if(cmd[2]==0xFF)cmd[2]=1; // logical sector default
		// check sector validity
		if(cmd[1]<80&&cmd[2]==1) {
			// respond ok
			fdc_ok(ret,cmd[1]);
			// return
			switch(cmd[0]) {
				case 'A': case 'R':           count=   1; break;
				case 'S': case 'B': case 'C': count=  12; break;
				case 'W': case 'X':           count=1024; break;
			}
			printf("[fdc emu] recv 0x%02X [%s] resp: %s expect %i bytes\n",cmd[0],fdc_name(cmd[0]),ret,count);
		} else printf("[fdc emu] recv 0x%02X [%s] resp: %s bad sector %i\\%i\n",cmd[0],fdc_name(cmd[0]),ret,cmd[1],cmd[2]);
	}
	// send response
	eswrite(ret,8);
	return count;
}

// fdc mode command+data executer
static void exec_fdc_data(uint8_t *cmd) {
	char ret[9];
	uint8_t *p_data=&cmd[4];
	uint8_t n;
	switch(cmd[0]) {
		case 'A': 					// read sector id
			if(*p_data=='\x0D') eswrite(&sids[cmd[1]][0],12);
			printf("[fdc emu] exec 0x%02X [%s]\n",cmd[0],fdc_name(cmd[0]));
			return;
		case 'R': 					// read sector data
			if(*p_data=='\x0D') eswrite(&data[(uint16_t)(cmd[1])<<10],1024);
			printf("[fdc emu] exec 0x%02X [%s]\n",cmd[0],fdc_name(cmd[0]));
			return;
		case 'S': 					// find sector
			strcpy(ret,"40000000"); // fail
			for(n=0;n<80;n++) {
				if(memcmp(&sids[n][0],p_data,12)==0) {
					fdc_ok(ret,n); // success
					break;
				}
			}
			break;
		case 'B': case 'C': // write sector id
			memcpy(&sids[cmd[1]][0],p_data,12);
			fdc_ok(ret,cmd[1]);
			break;
		case 'W': case 'X': // write sector data
			memcpy(&data[(uint16_t)(cmd[1])<<10],p_data,1024);
			fdc_ok(ret,cmd[1]);
			break;
	}
	printf("[fdc emu] exec 0x%02X [%s] resp: %s\n",cmd[0],fdc_name(cmd[0]),ret);
	eswrite(ret,8);
}

// PUBLIC: emulate floppy drive
void emulate() {
  uint8_t byte;
  uint8_t state,csum;
  uint16_t count;
 	uint8_t buf[1028],*p_buf;
 	char fmt[]="9600,N,8,1";
	printf("serial device> ");
	if(read_cmd("")) {
  	if(sopen(cmd)) {
  	  printf("serial port open\n");
  	  if(!sconfig(fmt)) {
  	    printf("unable to configure serial port - ignoring\n");
  	  }
	    printf("serial port listening... (ctrl)^C/SIGINT to stop\n");
    	// listen for ctrl^C
    	signal(SIGINT, sigint);
	    while(!stop) {
        if(sread(&byte,1)==1) {
          switch(state) {
            case 0:
              if(byte=='Z') state=1;
              else if(byte!=0x00&&strchr("FGARSBCWX",byte)!=NULL) {
              	p_buf=buf;
              	*p_buf++=csum=byte;
              	buf[1]=buf[2]=0xFF;
              	state=6;
              } else printf("[general] recv 0x%02X bad/unsupported command\n",byte);
              break;
            case 1: // opmode second preamble
              if(byte=='Z') state=2;
             	else printf("[op mode] recv 0x%02X expected preamble 0x5A\n",byte);
             	break;
            case 2: // opmode block format
            	p_buf=buf;
            	*p_buf++=csum=byte;
            	state=3;
            	break;
            case 3: // opmode length of data block
            	state=(count=byte)?4:5;
            	csum+=byte;
            	break;
            case 4: // opmode data block (ignored)
          		csum+=byte;
            	if(--count==0) state=5;
            	break;
            case 5: // opmode checksum
            	if((csum^0xFF)==byte) exec_op(buf);
            	else printf("[op mode] recv 0x%02X expected checksum 0x%02X\n",byte,csum^0xFF);
            	state=0;
            	break;
            case 6: // fdc params
          		if(byte=='\x0D'||byte=='\x0A') {
          			if(*p_buf==0xFF) p_buf--;
          			count=exec_fdc(buf);
          			state=count?7:0;
          			p_buf=&buf[4];
          		} else if(byte==',') {
          			if(++p_buf>&buf[3]) {
          				printf("[fdc emu] recv too many parameters\n");
          				state=0;
          			}
          		} else if(byte>='0'&&byte<='9') {
          			if(*p_buf==0xFF)*p_buf=0;
          			*p_buf*=10;
          			*p_buf+=byte-'0';
          		} else if(byte!=' ') {
          			printf("[fdc emu] recv 0x%02X expected parameter data\n",byte);
          			state=0;
          		}
            	break;
            case 7: // fdc data
            	*p_buf++=byte;
            	if(--count==0) {
            		exec_fdc_data(buf);
            		state=0;
            	}
            	break;
          }
        }
	    }
	    stop=false;
  	  if(sclose()) {
  	    printf("serial port closed\n");
  	  } else  {
  	    printf("unable to close serial port\n");
  	  }
  	} else {
  	  printf("unable to open serial port\n");
  	}
  }
}


//== ACTUAL FUN STUFF (WELL, GUESS THE EMULATOR WAS FUN TOO...) ======


// return nof bytes used (excl header) by currently loaded pattern
static uint16_t calc_size() {
	return ((ptn.height+1)>>1)+(((((ptn.width+3)>>2)*ptn.height)+1)>>1);
}

// decode header and store in global ptn
// return pattern number if successful
static uint16_t decode_header(uint8_t index) {
	uint8_t *hdr=&p_track[index*7];
	if(hdr[0]!=0x55) {
		ptn.pos = 0x7FFF-(index*7);
		ptn.offset =hdr[0]<<8;			
		ptn.offset|=hdr[1];					
		ptn.height =MSN(hdr[2])*100;
		ptn.height+=LSN(hdr[2])*10; 
		ptn.height+=MSN(hdr[3])*1;  
		ptn.width  =LSN(hdr[3])*100;
		ptn.width +=MSN(hdr[4])*10; 
		ptn.width +=LSN(hdr[4])*1;
		if(MSN(hdr[5])!=0) {
		  printf("encountered bad data @ %04X\n", index*7+5);
		  if(halt)exit(1);
		}
		ptn.id =LSN(hdr[5])*100;
		ptn.id+=MSN(hdr[6])*10; 
		ptn.id+=LSN(hdr[6])*1;
		if(ptn.id>=901&&ptn.id<=998) return ptn.id;
	}
	return 0;
}

// find and read pattern with specific pattern number/id
// return true if found
static bool find_pattern(uint32_t ptn_id) {
	uint8_t n;
	for(n=0;n<98;n++) {
		if(decode_header(n)){
			if(ptn.id==ptn_id) {
				return true;
			}
		}
	}
	return false;
}

// decode a pattern from memory and display on screen
static void decode_pattern() {
	uint32_t stride,memo,nptr;
	uint32_t x,y;
	
	stride=(ptn.width+3)>>2; // nof nibbles per row
	memo=(ptn.height+1)&~1;  // nof nibbles for memo

	// calculate nibble pinter
  nptr = 0xFFFF-(ptn.offset<<1)-(memo+stride*(ptn.height-1));
  
  // print out pattern
  for(y=0;y<ptn.height;y++) {
  	for(x=0;x<ptn.width;x++) {
  		putchar((nib_get(nptr-(x>>2))&(1<<(x&3)))?'X':'-');
  	}
  	printf(" 0x");
  	for(x=0;x<ptn.width;x+=4) {
  		printf("%1X",nib_get(nptr-(x>>2)));
  	}
  	printf("\n");
  	nptr+=stride;
  }
}

// convert pattern id as string to int
static uint32_t str_to_id(char *str) {
	uint32_t out=0;
	while(*str!=0){
		if(*str<'0'||*str>'9') return 0;
		out=out*10+*str++-'0';
		if(out>998) return 0;
	}
	if(out<901) return 0;
	return(out);
}

// PUBLIC: format memory, reset track to 1
void format() {
	uint8_t n;
	// set sector ids
	for(n=0;n<80;n++){
		memset(&sids[n][0],0,12);
		sids[n][0]=n<64?n/32:0;
	}
	for(n=0;n<2;n++) {
  	p_track=&data[n<<15];
  	// clear data
  	memset(p_track,0x55,32768-256);
  	memset(&p_track[0x7F00],0x00,256);
  	p_track[0x0005]=0x09; // pattern 901 partial header data
  	p_track[0x0006]=0x01;
  	p_track[0x7F00]=0x01; // next offset
  	p_track[0x7F01]=0x20;
  	p_track[0x7F10]=0x7F; // unknown pointer
  	p_track[0x7F11]=0xF9;
  	p_track[0x7FEA]=0x10; // 1xxx BCD (000=no pattern)
  }
  p_track=data;
}

// format with printout
void cmd_format() {
  format();
	printf("memory initialized, track is 1\n");
}

// PUBLIC: set current track
void set_track(uint8_t track) {
  p_track=&data[(track)<<15];
}

// PUBLIC: get current track
uint8_t get_track() {
  return (p_track-data)>>15;
}

// prompt for set current track
static void cmd_track() {
	printf("current track is %i\n",get_track()+1);
	printf("track #> ");
	if(read_cmd("")) {
		if(strlen(cmd)==1&&(cmd[0]=='1'||cmd[0]=='2')) {
		  set_track(cmd[0]-'1');
		  printf("track is now %s\n",cmd);
		} else {
			printf("need number between 1 and 2\n");
		  if(halt)exit(1);
		}
	}
}

// display patterns contained in memory
static void display() {
	uint8_t n;
	uint32_t ptn_id;
	for(n=0;n<98;n++) {
		if(decode_header(n)) {
			printf("pattern #%i is %ix%i @%04X-0x%04X\n",ptn.id,ptn.width,ptn.height,ptn.offset,ptn.offset+calc_size()-1);
		}
	}
	while(1) {
		printf("pattern #> ");
		if(!read_cmd("")) break;
		if(strcmp(cmd,"d")==0||strcmp(cmd,"done")==0) break;
		ptn_id=str_to_id(cmd);
		if(ptn_id) {
			if(find_pattern(ptn_id)){
				decode_pattern();
			} else {
				printf("pattern number %i does not exist\n",ptn_id);
        if(halt)exit(1);
			}
		} else {
			printf("need done or number between 901 and 998\n");
		  if(halt)exit(1);
		}
	}
}

// write out a disk image or floppy emulator folder
static void writeout() {
	uint8_t n;
	char fn[256];
	uint32_t temp;
	FILE *f;
	printf("filename> ");
	if(read_cmd("")) {
	  temp=strlen(cmd)-1;
	  if(cmd[temp]=='\\'||cmd[temp]=='/') {

	  	//folder
	  	cmd[temp+3]=0;
	  	for(n=0;n<80;n++) {
		  	cmd[temp+1]='0'+(n/10);
		  	cmd[temp+2]='0'+(n%10);
	  		//read id
		  	strcpy(fn,cmd);
		  	strcat(fn,".id");
			  f=fopen(fn,"wb");
			  if(f) {
			  	fwrite(&sids[n][0],1,12,f);
			  	fclose(f);
				} else {
				 	printf("unable to open file %s\n",fn);	
    		  if(halt)exit(1);
				 	break;
				}
	  		//read sector
		  	strcpy(fn,cmd);
		  	strcat(fn,".dat");
			  f=fopen(fn,"wb");
			  if(f) {
			  	fwrite(&data[n<<10],1,1024,f);
			  	fclose(f);
				} else {
				 	printf("unable to open file %s\n",fn);	
    		  if(halt)exit(1);
				 	break;
				}
	  	}
			if(n==80) printf("wrote 80x1k dat+id\n");

		} else {
			f=fopen(cmd,"wb");
			if(f) {
				fwrite(data,1,81920,f);
	  		for(n=0;n<80;n++) {
	  			fwrite(&sids[n][0],1,12,f);
	  		}
				printf("wrote 80k disk image\n");
			} else {
			  printf("unable to open file %s\n",cmd);
  		  if(halt)exit(1);
			}
		}
	}
}

// read in a disk image or floppy emulator folder
static void readin() {
	uint8_t n;
	uint32_t ptn_id;
	char fn[256];
	uint32_t temp;
	FILE *f;
	printf("filename> ");
	if(read_cmd("")) {
	  temp=strlen(cmd)-1;
	  if(cmd[temp]=='\\'||cmd[temp]=='/') {
	  	//folder
	  	cmd[temp+3]=0;
	  	for(n=0;n<80;n++) {
		  	cmd[temp+1]='0'+(n/10);
		  	cmd[temp+2]='0'+(n%10);
	  		//read id
		  	strcpy(fn,cmd);
		  	strcat(fn,".id");
			  f=fopen(fn,"rb");
			  if(f) {
			  	fread(&sids[n][0],1,12,f);
			  	fclose(f);
				} else {
				 	printf("unable to open file %s\n",fn);
    		  if(halt)exit(1);
    		  cmd_format();
				 	break;
				}
	  		//read sector
		  	strcpy(fn,cmd);
		  	strcat(fn,".dat");
			  f=fopen(fn,"rb");
			  if(f) {
			  	fread(&data[n<<10],1,1024,f);
			  	fclose(f);
				} else {
				 	printf("unable to open file %s\n",fn);	
    		  if(halt)exit(1);
    		  cmd_format();
				 	break;
				}
	  	}
			if(n==80) printf("read 80x1k dat+id, track is 1\n");
	  } else {
		  f=fopen(cmd,"rb");
		  if(f) {
		  	fseek(f,0,SEEK_END);
		  	if(ftell(f)!=81920+960&&ftell(f)!=32768) {
		  		printf("bad file size %s\n",cmd);
    		  if(halt)exit(1);
		  	} else {
		  		if(ftell(f)==32768) {
			  		fseek(f,0,0);
			  		fread(data,1,32768,f);
			  		memset(&data[32768],0,81920-32768);
			  		for(n=0;n<80;n++){
			  			memset(&sids[n][0],0,12);
			  			sids[n][0]=n<32?1:0;
			  		}
			  		printf("read 32k blob, track is 1\n");
		  		} else {
			  		fseek(f,0,0);
			  		fread(data,1,81920,f);
			  		for(n=0;n<80;n++) {
			  			fread(&sids[n][0],1,12,f);
			  		}
			  		printf("read 80k disk image, track is 1\n");
		  		}
		  	}
		  	fclose(f);
		  } else {
		  	printf("unable to open file %s\n",cmd);
  		  if(halt)exit(1);
		  }
		}
  	p_track=data;
	}
}

// sample image, return true for "stitch" else false
static bool sample(uint8_t *p_img,uint8_t w,uint8_t x,uint8_t y) {
	return p_img[y*w+x]<0x80;
}

// returns bcd number of column with specified value (ie 1, 10, 100, etc)
static uint8_t bcd_get(uint16_t n,uint16_t value) {
	return (n/value)%10;
}

// PUBLIC: add pattern to memory
uint16_t add_pattern(uint8_t *p_img,uint16_t w,uint16_t h) {
	uint8_t   n;
	uint16_t  ptn_id;
	uint32_t  temp;
	uint16_t  x,y;
	uint8_t  *p_data;
	uint16_t  b_stride;
	uint32_t  bp_last,bp_line,bp_this;
	uint16_t  memo_bytes,data_bytes;
	uint16_t  o_bottom;
	uint8_t   hdr_index;

	// find index&id
	ptn_id=901;
	hdr_index=0;
	for(n=0;n<98;n++) {
		if(decode_header(n)) {
		  hdr_index=n+1;
			if(ptn.id>=ptn_id) ptn_id=ptn.id+1;
	  }
	}

	// display on screen
	for(y=0;y<h;y++) {
		for(x=0;x<w;x++) {
			putchar(sample(p_img,w,x,y)?'X':'-');
		}
		printf("\n");
	}

  // remember bottom offset
	o_bottom=int_get(0x7F00);

  // calculate bytes needed to store pattern
	memo_bytes=(h+1)>>1;
	data_bytes=((((w+3)>>2)*h)+1)>>1;
	
	// Check memory availability (should be 0x2AE, but leave some to be sure)
	if(0x7FFF-(o_bottom+memo_bytes+data_bytes)>=0x02B0&&ptn_id<999) {

  	// make memo data
  	p_data=(uint8_t*)malloc(memo_bytes);
  	memset(p_data,0,memo_bytes);
  	// set memo data
  	memset(p_data,0,memo_bytes);
  	// insert into memory @ PATTERN_PTR1
  	memcpy(&p_track[0x8000-int_get(0x7F00)-memo_bytes],p_data,memo_bytes);
  	// update PATTERN_PTR1
  	int_set(0x7F00,int_get(0x7F00)+memo_bytes);
    // free memo data
    free(p_data);	  	
  	
  	// make pattern data
  	p_data=(uint8_t*)malloc(data_bytes);
  	memset(p_data,0,data_bytes);
  	// stride (bits per row)
  	b_stride=(w+3)&~3;
  	// calculate index of last bit in pattern
  	bp_last=(data_bytes<<3)-1;
		// set pattern data
		for(y=0;y<h;y++) {
      // calculate index of last bit on line
      bp_line=bp_last-b_stride*(h-y-1);
      for(x=0;x<w;x++) {
        // calculate index of the current bit
        bp_this=bp_line-x;
        // sample image
        if(sample(p_img,w,x,y)) bit_set(p_data,bp_this);
      }
    }
  	// insert into memory @ PATTERN_PTR1
  	memcpy(&p_track[0x8000-int_get(0x7F00)-data_bytes],p_data,data_bytes);
  	// update PATTERN_PTR1
  	int_set(0x7F00,int_get(0x7F00)+data_bytes);
    // free pattern data 
  	free(p_data);

  	// write header
  	p_track[hdr_index*7+0]=o_bottom>>8;						  // byte 0
  	p_track[hdr_index*7+1]=o_bottom&0x00FF;					// byte 1
		nib_set(hdr_index*14+ 4,bcd_get(h,100));			// byte 2
		nib_set(hdr_index*14+ 5,bcd_get(h, 10));
		nib_set(hdr_index*14+ 6,bcd_get(h,  1));  		// byte 3
		nib_set(hdr_index*14+ 7,bcd_get(w,100));
		nib_set(hdr_index*14+ 8,bcd_get(w, 10));			// byte 4
		nib_set(hdr_index*14+ 9,bcd_get(w,  1));
		nib_set(hdr_index*14+10,0);							 			// byte 5
		nib_set(hdr_index*14+11,bcd_get(ptn_id,100));
		nib_set(hdr_index*14+12,bcd_get(ptn_id, 10)); // byte 6
		nib_set(hdr_index*14+13,bcd_get(ptn_id,  1));

		// write FINHDR
		hdr_index++;
		nib_set(hdr_index*14+10,0);								 			// byte 5
		nib_set(hdr_index*14+11,bcd_get(ptn_id+1,100));
		nib_set(hdr_index*14+12,bcd_get(ptn_id+1, 10)); // byte 6
		nib_set(hdr_index*14+13,bcd_get(ptn_id+1,  1));

		// write LOADED_PATTERN
		nib_set((0x7FEA<<1)+0,0x01);
		nib_set((0x7FEA<<1)+1,bcd_get(ptn_id,100));
		nib_set((0x7FEA<<1)+2,bcd_get(ptn_id, 10));
		nib_set((0x7FEA<<1)+3,bcd_get(ptn_id, 1));
		
		// write UNK1
		int_set(0x7F02,0x0001);
		
		// copy PATTERN_PTR1 to PATTERN_PTR2
		int_set(0x7F04,int_get(0x7F00));

		// write LAST_BOTTOM
		int_set(0x7F06,o_bottom);

		// write LAST_TOP
		int_set(0x7F0A,int_get(0x7F04)-1);

		// write UNK3
		int_set(0x7F0E,0x8100);

		// write HEADER_PTR
		int_set(0x7F10,0x7FFF-(hdr_index+1)*7+1);

		// check/decode written data, print out info
		if(decode_header(hdr_index-1)) {
		  printf("added #%i is %ix%i @%04X-0x%04X\n",ptn.id,ptn.width,ptn.height,ptn.offset,ptn.offset+calc_size()-1);
		  return ptn.id;
		} else {
		  printf("something bad happened\n");
		}
	}
  if(halt)exit(0);
	return 0;
}

// PUBLIC: read image file
void *image_read(FILE *f,uint16_t *w,uint16_t *h) {
  void *p_img;
  size_t bytes;
  uint8_t temp[4];
  // get fle size
  fseek(f,0,SEEK_END);
  bytes=ftell(f);
  fseek(f,0,0);
  // ensure header
  if(bytes<4) return NULL;
  // read header - 4 bytes
  fread(temp,1,4,f);
  // get size
  *w=(temp[0]<<8)|temp[1];
  *h=(temp[2]<<8)|temp[3];
  // ensure valid size
  if(*w==0||*h==0) return NULL;
  // ensure correct size
  if(bytes!=4+*w**h) return NULL;
  // read picture - w*h bytes: top-down, left-right, 8bpp grayscale
  p_img=malloc(*w**h);
  fread(p_img,1,*w**h,f);
  return p_img;
}

// add pattern from file
static void add_file() {	
	uint8_t *p_img;
	uint16_t w,h;
	FILE *f;
	printf("filename> ");
	if(read_cmd("")) {
	  f=fopen(cmd,"rb");
	  if(f) {

      // read image file
      p_img=(uint8_t*)image_read(f,&w,&h);
	  	fclose(f);
	  	
	  	// TODO: check size, what is machine maximum?
	  	// if(w>???||h>???) {free(p_img);p_img=NULL;}

      if(p_img) {
				// add pattern to memory
				if(!add_pattern(p_img,w,h)) {
      	  printf("not enough memory to store pattern\n");
				}
      	// free loaded data
      	free(p_img);
  		} else {
  		  printf("image does not have the correct format\n");
  		  if(halt)exit(1);
  		}
	  } else {
	  	printf("unable to open file %s\n",cmd);
		  if(halt)exit(1);
	  }
	}
}

// print out and validate non-pattern information
static void info() {
	uint16_t last_pattern,sel_pattern;
	uint8_t n;
	uint8_t rep[5];
	uint16_t addr;
	
	// read selected pattern
	sel_pattern = LSN(p_track[0x7FEA]) * 100
	            + MSN(p_track[0x7FEB]) * 10
	            + LSN(p_track[0x7FEB]) * 1;

	// find last pattern
	for(n=0;n<98;n++) {
		decode_header(n);
	}
	last_pattern=ptn.id;
	
	// Check CONTROL_DATA
	printf("ADDRESS FORMAT     CONTENT         VALUE\n");
	printf("0x7F00  0x0120     write pointer : 0x%04X     ",int_get(0x7F00));
	if(int_get(0x7F00)==ptn.offset+calc_size()) puts("OK"); else printf("FAIL %04X\n",ptn.offset+calc_size());
	printf("0x7F02  0x0000     0x0001        : 0x%04X     ",int_get(0x7F02));
	if(int_get(0x7F02)==1) puts("OK"); else puts("FAIL");
	printf("0x7F04  0x0000     write pointer : 0x%04X     ",int_get(0x7F04));
	if(int_get(0x7F04)==ptn.offset+calc_size()) puts("OK"); else printf("FAIL %04X\n",ptn.offset+calc_size());
	printf("0x7F06  0x0000     loaded tail   : 0x%04X     ",int_get(0x7F06));
	if(int_get(0x7F06)==ptn.offset) puts("OK"); else printf("FAIL %04X\n",ptn.offset);
	printf("0x7F08  0x0000     0x0000        : 0x%04X     ",int_get(0x7F08));
	if(int_get(0x7F08)==0) puts("OK"); else puts("FAIL");
	printf("0x7F0A  0x0000     loaded head   : 0x%04X     ",int_get(0x7F0A));
	if(int_get(0x7F0A)==ptn.offset+calc_size()-1) puts("OK"); else printf("FAIL %04X\n",ptn.offset+calc_size()-1);
	printf("0x7F0C  0x00000000 0x00008100    : 0x%04X%04X ",int_get(0x7F0C),int_get(0x7F0E));
	if(int_get(0x7F0C)==0&&int_get(0x7F0E)==0x8100) puts("OK"); else puts("FAIL");
	printf("0x7F10  0x7FF9     pointer 1     : 0x%04X     ",int_get(0x7F10));
	if(int_get(0x7F10)==ptn.pos-13) puts("OK"); else printf("FAIL %04X\n",ptn.pos-13);
	printf("0x7F12  0x0000     pointer 2     : 0x%04X     ",int_get(0x7F12));
	puts("?");
	printf("0x7F12  0x000000   0x000000      : 0x%04X%02X   ",int_get(0x7F14),p_track[0x7F16]);
	if(int_get(0x7F14)==0&&p_track[0x7F16]==0) puts("OK"); else puts("FAIL");
	printf("0x7FEA  0x1000     loaded pattern: 0x%04X     ",int_get(0x7FEA));
	if(last_pattern==sel_pattern&&MSN(p_track[0x7FEA])==1) puts("OK"); else puts("FAIL");
	printf("0x7FFF  0x00       unknown       : 0x%02X       ", p_track[0x7FFF]);
	puts("?");
	
	puts("");

	// Check AREA0	
	rep[0]=p_track[0x7EE0];
	for(addr=0x7EE0;addr<0x7EE0+7;addr++) if(p_track[addr]!=rep[0]) break;
	if(addr==0x7EE0+7&&((rep[0]==0xAA)||(rep[0]==0x55))) {
		printf("AREA0   0x%02X * 7                              OK\n",rep[0]);
	} else {
		printf("AREA0                                         FAIL\n");
		print_hex(&p_track[0x7FE0],7);
	}

	// Just print AREA1 - don't know how to check
	printf("AREA1   ");
	print_slim_hex(&p_track[0x7EE7],25);
	printf("\n");

	// Just print AREA2 - don't know how to check
	printf("AREA2   ");
	print_slim_hex(&p_track[0x7F17],25);
	printf("\n");

	// Check AREA3
	for(addr=0x7F30;addr<0x7F30+186;addr++) if(p_track[addr]!=00) break;
	if(addr==0x7F30+186) {
		printf("AREA3   0x00 * 186                            OK\n",rep[0]);
	} else {
		printf("AREA3                                         FAIL\n");
		print_hex(&p_track[0x7FE0],7);
	}

	// Check AREA4
	for(addr=0x7FEC;addr<0x7FEC+19;addr++) if(p_track[addr]!=00) break;
	if(addr==0x7FEC+19&&((rep[0]==0xAA)||(rep[0]==0x55))) {
		printf("AREA4   0x00 * 19                             OK\n",rep[0]);
	} else {
		printf("AREA4                                         FAIL\n");
		print_hex(&p_track[0x7FEC],19);
	}
	
	printf("\n");
	
	// Check FINHDR
	printf("FINHDR  ");
	print_slim_hex(&p_track[0x7FFF-ptn.pos+7],7);
	for(n=0;n<5;n++) {
		if(p_track[0x7FFF-ptn.pos+7+n]!=0x55) break;
	}
	if(n==5
	&& MSN(p_track[0x7FFF-ptn.pos+7+5])==0
	&& LSN(p_track[0x7FFF-ptn.pos+7+5])==(((ptn.id+1)/100)%10)
	&& MSN(p_track[0x7FFF-ptn.pos+7+6])==(((ptn.id+1)/ 10)%10)
	&& LSN(p_track[0x7FFF-ptn.pos+7+6])==(((ptn.id+1)/  1)%10)) {
		printf("                        OK\n");
	} else {
		printf("                        FAIL\n");
	}
}

// PUBLIC: add raw file array
bool add_raw(uint8_t p_img[]) {
	return add_pattern(&p_img[4],(p_img[0]<<8)|p_img[1],(p_img[2]<<8)|p_img[3]);
}

// do the nasty
int main(int argc,char**argv) {
	uint8_t n;
	uint8_t *hdr;
	uint32_t ptn_id;
	FILE *f;
	// skip executable
	if(argc) {
		argc--;
		argv++;
	}
	// fetch arguments
	while(argc--) {
		strcat(cmds,*argv++);
		if(argc)strcat(cmds," ");
	}
	// init memory
	cmd_format();
	while(1) {
		printf("> ");
		if(read_cmd("q")) {
			if(strcmp(cmd,"help")==0||strcmp(cmd,"?")==0) {
				printf("?/help      show this\n");
				printf("r/read      read in data from file\n");
				printf("w/write     write out data to file\n");
				printf("f/format    clear all tracks\n");
				printf("t/track     set working track\n");
				printf("a/add       add pattern to track\n");
				printf("s/show      display content of track\n");
				printf("i/info      additional track info\n");
				printf("e/emulate   emulate floppy\n");
				printf("q/quit      end program\n");
				printf("x/halt      halt on errors\n");
			} else if(strcmp(cmd,"quit")==0||strcmp(cmd,"q")==0) {
				printf("See you!\n");
				exit(0);
			} else if(strcmp(cmd,"r")==0||strcmp(cmd,"read")==0) {
				readin();
			} else if(strcmp(cmd,"w")==0||strcmp(cmd,"write")==0) {
				writeout();
			} else if(strcmp(cmd,"t")==0||strcmp(cmd,"track")==0) {
	  		cmd_track();
			} else if(strcmp(cmd,"f")==0||strcmp(cmd,"format")==0) {
	  		cmd_format();
			} else if(strcmp(cmd,"a")==0||strcmp(cmd,"add")==0) {
	  		add_file();
			} else if(strcmp(cmd,"i")==0||strcmp(cmd,"info")==0) {
	  		info();
			} else if(strcmp(cmd,"s")==0||strcmp(cmd,"show")==0) {
				display();
			} else if(strcmp(cmd,"x")==0||strcmp(cmd,"halt")==0) {
	  		halt=!halt;
	  		printf("halt on errors: %s\n",halt?"yes":"no");
			} else if(strcmp(cmd,"e")==0||strcmp(cmd,"emulate")==0) {
        emulate();	  	  
			} else {
				printf("Unrecognized command: %s\n",cmd);
  		  if(halt)exit(1);
			}
		}
	}
}