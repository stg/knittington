#include <stdint.h>
#include <stdbool.h>
#ifdef SDL2
#include <SDL2/SDL.h>
#else
#include <SDL/SDL.h>
#endif
#include "renderer.h"
#include "ui.h"
#include "machine.h"
#include "emulate.h"
#include "picture.h"

// screen surface
#ifdef SDL2
static SDL_Window* window;
static SDL_Renderer* renderer;
#endif
static SDL_Surface  *screen;

// ui enums
enum {
	VIEW_TOP,
	VIEW_MACH,
	VIEW_ADD,
	VIEW_EDIT,
	VIEW_EMU,
	VIEW_MSG
};

enum {
	TOP_LIST,
	TOP_NEW,
	TOP_ADD,
	TOP_EDIT,
	TOP_DEL,
	TOP_EMU,
	TOP_OUT,
	TOP_INFO,
	TOP_COUNT
};

enum {
	MACH_LIST,
	MACH_OK,
	MACH_CANCEL,
	MACH_COUNT
};

enum {
	ADD_W,
	ADD_H,
	ADD_OK,
	ADD_CANCEL,
	ADD_COUNT
};

enum {
  EDIT_GRID,
	EDIT_OK,
	EDIT_COUNT
};

enum {
	EMU_LIST,
	EMU_OK,
	EMU_CANCEL,
	EMU_COUNT
};

enum {
	MSG_TEXT,
	MSG_OK,
	MSG_COUNT
};

// ui views
static view_t  *vtop,*vmach,*vadd,*vedit,*vemu,*vmsg;
static uiobj_t *top[TOP_COUNT],*mach[MACH_COUNT],*add[ADD_COUNT],*edit[EDIT_COUNT],*emu[EMU_COUNT],*msg[MSG_COUNT];

// currently selected view
static uint8_t view=VIEW_TOP;

// in emulate mode
static bool emulating=false;

// time to quit?
static bool quit=false;

// currently selected machine
static machine_t *p_mach;

#ifdef SDL2
// dropping files
static uint16_t stat_add=0,stat_size=0,stat_format=0;
#endif

// sets names of all items in pattern list
static void list_fill() {
	listitem_t *p_item;
	uint16_t w,h,ix=0;
	p_item=((list_t*)top[TOP_LIST]->obj)->list;
	while(p_item) {
		w=((uint16_t*)p_item->data)[0];
		h=((uint16_t*)p_item->data)[1];
		sprintf(p_item->text,"%i IS %iX%i",p_mach->pattern_min+ix++,w,h);
		p_item=p_item->next;
	}
	ui_enable(top[TOP_EMU],((list_t*)top[TOP_LIST]->obj)->count>0);	
}

// build disk image under current machine
static bool build_image() {
	char text[60];
	uint16_t w,h;
	uint16_t ptn;
	listitem_t *p_item;
	p_item=((list_t*)top[TOP_LIST]->obj)->list;

	p_mach->format();
	p_mach->set_track(0);

	ptn=p_mach->pattern_min;
	while(p_item) {
		w=((uint16_t*)p_item->data)[0];
		h=((uint16_t*)p_item->data)[1];
		if(!p_mach->add_pattern(&((uint8_t*)p_item->data)[4],w,h)) {
			sprintf(text,"OUT OF MEMORY WHEN ADDING \nPATTERN %i. REMOVE SOME\nPATTERNS AND TRY AGAIN.",ptn);
			ui_field_add(msg[MSG_TEXT],text);
			return false;
		}
		ptn++;
		p_item=p_item->next;
	}

	return true;
}

// event handler for top view
static bool top_event(SDL_Event *event) {
	uiobj_t *p_ui;
	p_ui=event->user.data1;
	listitem_t *p_item;
	char text[200];
	switch( event->type ) {
#ifdef SDL2
		case SDL_DROPFILE:
			if(p_mach) {
  			if(event->drop.file) {
  			  if(picture_load(event->drop.file)==0) {
  			    if(p_mach->size_check(picture_width(),picture_height())) {
  			      p_item=ui_list_add(top[TOP_LIST],"PPP IS WWWXHHH",4+picture_width()*picture_height());
  		  		  ((uint16_t*)p_item->data)[0]=picture_width();
  		  		  ((uint16_t*)p_item->data)[1]=picture_height();
  		  		  picture_convert(&((uint8_t*)p_item->data)[4]);
  		  		  list_fill();
  			      stat_add++;
  			    } else {
  			      stat_size++;
  			    }
  			    picture_free();
  			  } else {
  			    stat_format++;
  			  }
    		} else {
    		  sprintf(text,"SUCCESSFULLY ADDED: %i\nTOO BIG FOR MACHINE: %i\nUNRECOGNIZED FORMAT: %i",stat_add,stat_size,stat_format);
    		  ui_field_add(msg[MSG_TEXT],text);
    		  stat_add=0;
    		  stat_size=0;
    		  stat_format=0;
    			view=VIEW_MSG;
    			return true;
    		}
			} else {
  			ui_field_add(msg[MSG_TEXT],"NOT READY TO ACCEPT IMAGES\nAT THIS TIME. PLEASE SELECT\nMACHINE USING FORMAT FIRST.");
  			view=VIEW_MSG;
  			return true;
			}
	  	break;
#endif
	  case SDL_QUIT:
	    quit = 1; // Set time to quit
	    return true;
	  case UI_CLICK:
	  	if(p_ui==top[TOP_NEW]) {
	  		view=VIEW_MACH;
	  		return true;
	  	} else if(p_ui==top[TOP_ADD]) {
	  		ui_text_set(add[ADD_W],"");
	  		ui_text_set(add[ADD_H],"");
	  		ui_text_valid(add[ADD_W],false);
	  		ui_text_valid(add[ADD_H],false);
	  		ui_enable(add[ADD_OK],false);
	  		view=VIEW_ADD;
	  		return true;
	  	} else if(p_ui==top[TOP_EDIT]) {
        p_item=ui_list_selected(top[TOP_LIST]);
        ui_grid_set(edit[EDIT_GRID],&((uint8_t*)p_item->data)[4],((uint16_t*)p_item->data)[0],((uint16_t*)p_item->data)[1]);
	  	  view=VIEW_EDIT;
	  	  return true;
	  	} else if(p_ui==top[TOP_DEL]) {
				ui_list_remove(top[TOP_LIST],ui_list_selected(top[TOP_LIST]));
	  		ui_enable(top[TOP_EDIT],false);
	  		ui_enable(top[TOP_DEL],false);
	  		list_fill();
	  	} else if(p_ui==top[TOP_EMU]) {
	  		if(emulating) {
	  			machine_emulate_stop();
	  		} else {
		  		view=build_image()?VIEW_EMU:VIEW_MSG;
		  		return true;
	  		}
	  	}
	  	break;
	  case UI_CHANGE:
	  	if(p_ui==top[TOP_LIST]&&!emulating) {
	  		ui_enable(top[TOP_EDIT],true);
	  		ui_enable(top[TOP_DEL],true);
	  	}
	  	break;
    case SDL_KEYDOWN:
    	switch(event->key.keysym.sym==SDLK_ESCAPE) {
    		case 1:
			    quit = 1; // Set time to quit
			    return true;
    	}
    	break;
	}
	return false;
}

// event handler for machine select view
static bool mach_event(SDL_Event *event) {
	uiobj_t *p_ui=event->user.data1;
	switch( event->type ) {
	  case SDL_QUIT:
	    quit = 1; // Set time to quit
	    return true;
	  case UI_CLICK:
	  	if(p_ui==mach[MACH_OK]||p_ui==mach[MACH_CANCEL]) {
		  	if(p_ui==mach[MACH_OK]) {
					p_mach=machine_get(ui_list_selectedindex(mach[MACH_LIST]));
					ui_list_clear(top[TOP_LIST]);
					ui_enable(top[TOP_ADD],true);
					ui_enable(top[TOP_EDIT],false);
					ui_enable(top[TOP_DEL],false);
					ui_enable(top[TOP_EMU],false);
					ui_field_add(top[TOP_INFO],"DRAG AND DROP\nPICTURES TO THIS\nWINDOW TO ADD\nTHEM TO THE\nLIST OF PATTERNS");
		  	}
		  	view=VIEW_TOP;
		  	return true;
	  	}
	  	break;
	  case UI_CHANGE:
			if(p_ui==mach[MACH_LIST]) {
	  		ui_enable(mach[MACH_OK],true);
	  	}
	  	break;
	}
	return false;
}

// event handler for add pattern view
static bool add_event(SDL_Event *event) {
	uiobj_t *p_ui;
	p_ui=event->user.data1;
	char text[60];
	uint16_t w,h;
	listitem_t *p_item;
	switch( event->type ) {
	  case SDL_QUIT:
	    quit = 1; // Set time to quit
	    return true;
	  case UI_CLICK:
	  	if(p_ui==add[ADD_OK]||p_ui==add[ADD_CANCEL]) {
		  	if(p_ui==add[ADD_OK]) {
		  		w=atoi(ui_text_get(add[ADD_W]));
		  		h=atoi(ui_text_get(add[ADD_H]));
		  		if(!p_mach->size_check(w,h)) {
		  			sprintf(text,"NEW PATTERN OF SIZE\n%i X %i IS TOO BIG\nFOR THE SELECTED MACHINE.",w,h);
		  			ui_field_add(msg[MSG_TEXT],text);
		  			view=VIEW_MSG;
		  			return true;
		  		}
		  		p_item=ui_list_add(top[TOP_LIST],"PPP IS WWWXHHH",4+w*h);
		  		((uint16_t*)p_item->data)[0]=w;
		  		((uint16_t*)p_item->data)[1]=h;
		  		memset(&((uint8_t*)p_item->data)[4],0xFF,w*h);
		  		list_fill();
		  	}
		  	view=VIEW_TOP;
		  	return true;
	  	}
	  	break;
	  case UI_CHANGE:
			if(p_ui==add[ADD_W]||p_ui==add[ADD_H]) {
				if(strlen(ui_text_get(p_ui))>0) {
					ui_text_valid(p_ui,atoi(ui_text_get(p_ui))>0);
				} else {
					ui_text_valid(p_ui,false);
				}
				ui_enable(add[ADD_OK],ui_text_isvalid(add[ADD_W])&&ui_text_isvalid(add[ADD_H]));
	  	}
	  	break;
	}
	return false;
}

// event handler for edit pattern view
static bool edit_event(SDL_Event *event) {
	uiobj_t *p_ui;
	p_ui=event->user.data1;
	switch( event->type ) {
	  case SDL_QUIT:
	    quit = 1; // Set time to quit
	    return true;
	  case UI_CLICK:
	  	if(p_ui==edit[EDIT_OK]) {
	  	  view=VIEW_TOP;
	  	  return true;
	    }
	}
	return false;
}

// handles messages from emulator
static void emu_message(uint8_t event,uint8_t data) {
	char msg[40];
	switch(event) {
  	case EMU_ERROR:    // error        data=errcode
  		switch(data) {
			  case EMU_ERR_WRITE:  // serial write error
			  	sprintf(msg,"SERIAL PORT WRITE FAIL!");
		  		break;
			  case EMU_ERR_CMD:    // unsupported command
			  	sprintf(msg,"UNSUPPORTED COMMAND!");
		  		break;
			  case EMU_ERR_SECTOR: // bad sector id
			  	sprintf(msg,"BAD SECTOR ID!");
		  		break;
			  case EMU_ERR_CFG:    // serial port configuration fail
			  	sprintf(msg,"SERIAL CONFIGURATION FAIL!");
		  		break;
			  case EMU_ERR_OPEN:   // serial port open fail
			  	sprintf(msg,"SERIAL OPEN FAIL!");
		  		break;
			  case EMU_ERR_CLOSE:  // serial port close fail
			  	sprintf(msg,"SERIAL CLOSE FAIL!");
		  		break;
			  case EMU_ERR_DATA:   // unexpected data
			  	sprintf(msg,"UNEXPECTED DATA!");
		  		break;
			  default:
			  	sprintf(msg,"UNKNOWN ERROR!");
			  	break;
  		}
  		break;
  	case EMU_OPMODE:   // op-mode req. data=0
			sprintf(msg,"MODE SELECT");
  		break;
  	case EMU_FORMAT:   // format req.  data=0:ok 1:wrong code
			sprintf(msg,"FORMAT %s",data?"BAD CODE":"OK");
  		break;
  	case EMU_READID:   // read id      data=sector
			sprintf(msg,"READ ID %i",data);
  		break;
  	case EMU_WRITEID:  // write id     data=sector
			sprintf(msg,"WRITE ID %i",data);
  		break;
  	case EMU_READ:     // read         data=sector
			sprintf(msg,"READ SECTOR %i",data);
  		break;
  	case EMU_WRITE:    // write        data=sector
			sprintf(msg,"WRITE SECTOR %i",data);
  		break;
  	case EMU_FIND:     // search       data=80:not found <80:found
			if(data==80) {
				sprintf(msg,"SECTOR NOT FOUND");
			} else {
				sprintf(msg,"FOUND SECTOR %i",data);
			}
  		break;
  	case EMU_READY:    // ready        data=0
			sprintf(msg,"READY");
  		break;
  	default:
			sprintf(msg,"UNKNOWN MESSAGE");
  		break;
	}
	SDL_mutexP(mx_ui);
	ui_field_add(top[TOP_OUT],msg);
	SDL_mutexV(mx_ui);
}

// emulation thread (machine_emulate is blocking)
static int emu_thread(void *device) {
	SDL_mutexP(mx_ui);
	ui_field_add(top[TOP_OUT],"STARTING EMULATORù");
	SDL_mutexV(mx_ui);
	machine_emulate(device,NULL,emu_message);
	SDL_mutexP(mx_ui);
	ui_field_add(top[TOP_OUT],"EMULATOR STOPPED");
	ui_enable(top[TOP_NEW],true);
	ui_enable(top[TOP_ADD],true);
	ui_enable(top[TOP_EDIT],ui_list_selected(top[TOP_LIST])!=NULL);
	ui_enable(top[TOP_DEL],ui_list_selected(top[TOP_LIST])!=NULL);
	ui_button_set(top[TOP_EMU],"START EMULATOR");
	ui_field_add(top[TOP_INFO],"DRAG AND DROP\nPICTURES TO THIS\nWINDOW TO ADD\nTHEM TO THE\nLIST OF PATTERNS");
	SDL_mutexV(mx_ui);
	emulating=false;
}

// event handler for start emulator view
static bool emu_event(SDL_Event *event) {
	uiobj_t *p_ui=event->user.data1;
	switch( event->type ) {
	  case SDL_QUIT:
	    quit = 1; // Set time to quit
	    return true;
	  case UI_CLICK:
	  	if(p_ui==emu[EMU_OK]||p_ui==emu[EMU_CANCEL]) {
		  	if(p_ui==emu[EMU_OK]) {
					ui_enable(top[TOP_NEW],false);
					ui_enable(top[TOP_ADD],false);
					ui_enable(top[TOP_EDIT],false);
					ui_enable(top[TOP_DEL],false);
					emulating=true;
					ui_button_set(top[TOP_EMU],"STOP  EMULATOR");
					ui_field_add(top[TOP_INFO],"EMULATOR\nIS RUNNING\nYOU CAN NOW\nLOAD DATA ON\nTHE MACHINE");
#ifdef SDL2
					SDL_CreateThread(emu_thread,"emulator",ui_list_selected(emu[EMU_LIST])->data);
#else					
					SDL_CreateThread(emu_thread,ui_list_selected(emu[EMU_LIST])->data);
#endif
		  	}
		  	view=VIEW_TOP;
		  	return true;
	  	}
	  	break;
	  case UI_CHANGE:
			if(p_ui==emu[EMU_LIST]) {
	  		ui_enable(emu[EMU_OK],true);
	  	}
	  	break;
	}
	return false;
}

// event handler for message view
static bool msg_event(SDL_Event *event) {
	uiobj_t *p_ui=event->user.data1;
	switch( event->type ) {
	  case SDL_QUIT:
	    quit = 1; // Set time to quit
	    return true;
	  case UI_CLICK:
	  	if(p_ui==msg[MSG_OK]) {
		  	view=VIEW_TOP;
		  	return true;
	  	}
	  	break;
	}
	return false;
}

void add_port(char *p_name,char *p_device) {
	listitem_t *p_item;
	p_item=ui_list_add(emu[EMU_LIST],p_name,strlen(p_device)+1);
	strcpy(p_item->data,p_device);
}

// do the nasty
int main( int argc, char *argv[] ) {
	uint8_t n;
	listitem_t *p_item;
	char *p_text;

	// initialize sdl
	if(SDL_Init(SDL_INIT_VIDEO)) return 1;
#ifdef SDL2
	window=SDL_CreateWindow("Knitting machine interface",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,640,480,SDL_WINDOW_SHOWN);
	renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE);
	screen=SDL_GetWindowSurface(window);
#else
  SDL_WM_SetCaption("Knitting machine interface","Knitting machine interface");
  screen = SDL_SetVideoMode(640,480,24,0);
#endif  

  // initialize ui and renderer
  ui_init();
#ifdef SDL2
	r_init(renderer,screen);
#else
	r_init(screen);
#endif

	// build top view
	vtop=ui_fsview();
	top[TOP_LIST]=ui_add(vtop,ui_list  (1,1,16,22,"PATTERNS"));
	top[TOP_NEW] =ui_add(vtop,ui_button(22, 1,"    FORMAT    "));
	top[TOP_ADD] =ui_add(vtop,ui_button(22, 4," NEW  PATTERN "));
	top[TOP_EDIT]=ui_add(vtop,ui_button(22, 7," EDIT PATTERN "));
	top[TOP_DEL] =ui_add(vtop,ui_button(22,10,"DELETE PATTERN"));
	top[TOP_EMU] =ui_add(vtop,ui_button(22,13,"START EMULATOR"));
	top[TOP_OUT] =ui_add(vtop,ui_field (1,24,38,5,"EMULATOR",false,true));
	top[TOP_INFO]=ui_add(vtop,ui_field (21,17,18,7,NULL,true,false));
	ui_enable(top[TOP_ADD],false);
	ui_enable(top[TOP_EDIT],false);
	ui_enable(top[TOP_DEL],false);
	ui_enable(top[TOP_EMU],false);

	// build machine select view
	vmach=ui_view(20,15,"SELECT MACHINE");
	mach[MACH_LIST]   =ui_add(vmach,ui_list(0,0,17,12,"MACHINES"));
	mach[MACH_OK]     =ui_add(vmach,ui_button(8, 12,"OK"));
	mach[MACH_CANCEL] =ui_add(vmach,ui_button(12, 12,"CANCEL"));
	ui_enable(mach[MACH_OK],false);
	vmach->ok=mach[MACH_OK];
	vmach->cancel=mach[MACH_CANCEL];

	// build add pattern view
	vadd=ui_view(20,5,"ADD PATTERN");
	add[ADD_W]      =ui_add(vadd,ui_text(0,0,10,"WIDTH :"));
	add[ADD_H]      =ui_add(vadd,ui_text(0,1,10,"HEIGHT:"));
	add[ADD_OK]     =ui_add(vadd,ui_button(8, 3,"OK"));
	add[ADD_CANCEL] =ui_add(vadd,ui_button(12, 3,"CANCEL"));
	ui_enable(add[ADD_OK],false);
	vadd->ok=add[ADD_OK];
	vadd->cancel=add[ADD_CANCEL];

	// build edit pattern view
	vedit=ui_fsview();
	edit[EDIT_GRID]=ui_add(vedit,ui_grid(1,1,38,23));
	edit[EDIT_OK]=ui_add(vedit,ui_button(18,26,"OK"));

	// build start emulator view
	vemu=ui_view(30,15,"SELECT COMMUNICATIONS PORT");
	emu[EMU_LIST]   =ui_add(vemu,ui_list(0,0,27,12,"PORTS"));
	emu[EMU_OK]     =ui_add(vemu,ui_button(18, 12,"OK"));
	emu[EMU_CANCEL] =ui_add(vemu,ui_button(22, 12,"CANCEL"));
	ui_enable(emu[EMU_OK],false);
	vemu->ok=emu[EMU_OK];
	vemu->cancel=emu[EMU_CANCEL];
	senum(add_port); // enumerate serial ports

	// build message view
	vmsg=ui_view(30,8,"OHAI, YOU HAS MESSAGE");
	msg[MSG_TEXT]   =ui_add(vmsg,ui_field(0,0,30,5,NULL,true,true));
	msg[MSG_OK]     =ui_add(vmsg,ui_button(13, 5,"OK"));
	vmsg->ok=msg[MSG_OK];

	// enumerate available machines, add to machine select view
	machine_init();
	for(n=0;n<255;n++) {
		p_mach=machine_get(n);
		if(p_mach==NULL)break;
		p_text=malloc(strlen(p_mach->code)+strlen(p_mach->name)+2);
		strcpy(p_text,p_mach->code);
		strcat(p_text," ");
		strcat(p_text,p_mach->name);
		p_item=ui_list_add(mach[MACH_LIST],p_text,strlen(p_mach->name)+1);
		free(p_text);
		strcpy(p_item->data,p_mach->name);
	}

	// initial info text
	ui_field_add(top[TOP_INFO],"CLICK FORMAT\nTO SELECT AND\nBEGIN WORKING\nWITH A SPECIFIC\nKNITTING MACHINE");

	// main loop: view switching
	while(!quit) {
		switch(view) {
			case VIEW_TOP:  ui_display(vtop,top_event);
				break;
			case VIEW_MACH: ui_display(vmach,mach_event);
				break;
			case VIEW_ADD:  ui_display(vadd,add_event);
				break;
			case VIEW_EDIT: ui_display(vedit,edit_event);
				break;
			case VIEW_EMU:  ui_display(vemu,emu_event);
				break;
			case VIEW_MSG:  ui_display(vmsg,msg_event);
				break;
		}
	}

	// free up allocated ui memory
	ui_free_view(vtop);
	ui_free_view(vmach);
	ui_free_view(vadd);
	ui_free_view(vedit);
	ui_free_view(vemu);
	ui_free_view(vmsg);
	ui_free();
	
	
	// free up allocated renderer memory
	r_free();

}