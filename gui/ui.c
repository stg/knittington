#include <stdint.h>
#include <stdbool.h>
#ifdef SDL2
#include <SDL2/SDL.h>
#include <SDL2/SDL_keycode.h>
#else
#include <SDL/SDL.h>
#define SDL_Keycode SDLKey
#endif
#include "renderer.h"
#include "ui.h"

/** GENERICS ********************************************************/

static bool was_fullscreen=true;

SDL_mutex *mx_ui;

void ui_init() {
	mx_ui=SDL_CreateMutex();
#ifdef SDL2
	SDL_EventState(SDL_DROPFILE,SDL_ENABLE);
#endif	
}

void ui_free() {
	SDL_DestroyMutex(mx_ui);
}

void ui_enable(uiobj_t*p_ui,bool enable) {
	if(p_ui->enabled!=enable) {
		p_ui->enabled=enable;
		p_ui->state|=2;
	}
}

void capstr(char *p_text) {
	while(*p_text!=0) {
		*p_text=toupper(*p_text);
		if(*p_text=='\\') *p_text='/';
		p_text++;
	}
}

/** VIEWS ***********************************************************/

view_t* ui_fsview() {
	view_t *p_obj=malloc(sizeof(view_t));
	p_obj->x=0;
	p_obj->y=0;
	p_obj->w=40;
	p_obj->h=30;
	p_obj->caption=NULL;
	p_obj->ui=NULL;
	p_obj->fullscreen=true;
	p_obj->state=0;
	p_obj->ok=NULL;
	p_obj->cancel=NULL;
	return p_obj;
}

view_t* ui_view(uint8_t w,uint8_t h,char *caption) {
	view_t *p_obj=malloc(sizeof(view_t));
	p_obj->x=20-w/2;
	p_obj->y=15-h/2;
	p_obj->w=w;
	p_obj->h=h;
	p_obj->caption=malloc(strlen(caption));
	strcpy(p_obj->caption,caption);
	p_obj->fullscreen=false;
	p_obj->ui=NULL;
	p_obj->state=0;
	p_obj->ok=NULL;
	p_obj->cancel=NULL;
	return p_obj;
}

/** GRID EDITOR *****************************************************/

uiobj_t* ui_grid(uint8_t x,uint8_t y,uint8_t w,uint8_t h) {
	uiobj_t *p_ui=malloc(sizeof(uiobj_t));
	grid_t *p_obj=malloc(sizeof(grid_t));
	p_ui->x=x;
	p_ui->y=y;
	p_ui->w=w;
	p_ui->h=h;
	p_obj->l=0;
	p_obj->t=0;
	p_obj->x=0;
	p_obj->y=0;
	p_obj->data=NULL;
	p_ui->type=UI_GRID;
	p_ui->obj=p_obj;
	return p_ui;
}

void ui_grid_set(uiobj_t *p_ui,void *p_data,uint16_t w,uint16_t h) {
  grid_t *p_grid=p_ui->obj;
	p_grid->data=p_data;
	p_grid->l=0;
	p_grid->t=0;
	p_grid->x=0;
	p_grid->y=0;
	p_grid->w=w;
	p_grid->h=h;
	p_ui->state=1;
}

static bool grid_cset(uiobj_t *p_ui,int16_t x,int16_t y,char c,uint8_t bg) {
  grid_t *p_grid=p_ui->obj;
	view_t *p_view=p_ui->view;
	uint8_t ax,ay;
	if((x<0)||(x-(int16_t)p_grid->l>=0)) {
	  if((y<0)||(y-(int16_t)p_grid->t>=0)) {
	    if((x<0)||((x-(int16_t)p_grid->l)<p_ui->w)) {
  	    if((y<0)||((y-(int16_t)p_grid->t)<p_ui->h)) {
  	      if(x==-1) {
  	        ax=p_ui->x+p_view->x-1;
  	      } else if(x==-2) {
  	        ax=p_ui->x+p_view->x+p_ui->w;
  	      } else {
      	    ax=x+p_ui->x+p_view->x-p_grid->l;
  	      }
  	      if(y==-1) {
  	        ay=p_ui->y+p_view->y-1;
  	      } else if(y==-2) {
  	        ay=p_ui->y+p_view->y+p_ui->h;
  	      } else {
      	    ay=y+p_ui->y+p_view->y-p_grid->t;
      	  }
  	      r_char(ax,ay,c,0,bg);
  	      return true;
  	    }
	    }
	  }
	}
	return false;
}

static void grid_focus(uiobj_t *p_ui) {
/*  grid_t *p_grid=p_ui->obj;
	view_t *p_view=p_ui->view;
  if(p_grid->data) {
    r_cins(p_ui->x+p_view->x-p_grid->l+p_grid->x*3+2,p_ui->y+p_view->y-p_grid->t+p_grid->y*3+2);
  }
*/
  grid_t *p_grid=p_ui->obj;
	view_t *p_view=p_ui->view;
  if(p_grid->data) {
    r_cins(p_ui->x+p_view->x-p_grid->l+p_grid->x*2+1,p_ui->y+p_view->y-p_grid->t+p_grid->y*2+1,1);
  }

}

static void grid_draw(uiobj_t *p_ui) {
/*  grid_t *p_grid=p_ui->obj;
	view_t *p_view=p_ui->view;
	uint16_t x,y;
	uint8_t bg;
  if(p_ui->state&9) {
    r_white(p_ui->x+p_view->x-1,p_ui->y+p_view->y-1,p_ui->w+2,p_ui->h+2);
    if(p_grid->data) {
      for(y=1;y<3*p_grid->h+1;y++) {
        grid_cset(p_ui,0,y,'h',0);
        grid_cset(p_ui,p_grid->w*3+1,y,'g',0);
        if(((y-1)%15)==0) {
          grid_cset(p_ui,-1,y  ,'0'+(((y-1)/300)%10),0);
          grid_cset(p_ui,-1,y+1,'0'+(((y-1)/ 30)%10),0);
          grid_cset(p_ui,-1,y+2,'0'+(((y-1)/  3)%10),0);
          grid_cset(p_ui,-2,y  ,'0'+(((y-1)/300)%10),0);
          grid_cset(p_ui,-2,y+1,'0'+(((y-1)/ 30)%10),0);
          grid_cset(p_ui,-2,y+2,'0'+(((y-1)/  3)%10),0);
        }
      }
      for(x=1;x<3*p_grid->w+1;x++) {
        grid_cset(p_ui,x,0,'b',0);
        grid_cset(p_ui,x,p_grid->h*3+1,'y',0);
        if(((x-1)%15)==0) {
          grid_cset(p_ui,x  ,-1,'0'+(((x-1)/300)%10),0);
          grid_cset(p_ui,x+1,-1,'0'+(((x-1)/ 30)%10),0);
          grid_cset(p_ui,x+2,-1,'0'+(((x-1)/  3)%10),0);
          grid_cset(p_ui,x  ,-2,'0'+(((x-1)/300)%10),0);
          grid_cset(p_ui,x+1,-2,'0'+(((x-1)/ 30)%10),0);
          grid_cset(p_ui,x+2,-2,'0'+(((x-1)/  3)%10),0);
        }
      }
      grid_cset(p_ui,0,0,'m',0);
      grid_cset(p_ui,p_grid->w*3+1,0,'c',0);
      grid_cset(p_ui,0,p_grid->h*3+1,'r',0);
      grid_cset(p_ui,p_grid->w*3+1,p_grid->h*3+1,'i',0);
      for(y=0;y<p_grid->h;y++) {
        for(x=0;x<p_grid->w;x++) {
          bg=(p_grid->data[x+y*p_grid->w]<0x80)?1:0;
          grid_cset(p_ui,x*3+1,y*3+1,'t',bg);
          grid_cset(p_ui,x*3+2,y*3+1,'y',bg);
          grid_cset(p_ui,x*3+3,y*3+1,'u',bg);
          grid_cset(p_ui,x*3+1,y*3+2,'g',bg);
          grid_cset(p_ui,x*3+2,y*3+2,'ñ',bg);
          grid_cset(p_ui,x*3+3,y*3+2,'h',bg);
          grid_cset(p_ui,x*3+1,y*3+3,'v',bg);
          grid_cset(p_ui,x*3+2,y*3+3,'b',bg);
          grid_cset(p_ui,x*3+3,y*3+3,'n',bg);
        }
      }
      if(p_grid->l) for(y=0;y<p_ui->h-0;y++) {
        r_char(p_ui->x+p_view->x,p_ui->y+p_view->y+y,'a',0,0);
      }
      if(p_grid->w*3+2-p_grid->l>p_ui->w) for(y=0;y<p_ui->h-0;y++) {
        r_char(p_ui->x+p_view->x+p_ui->w-1,p_ui->y+p_view->y+y,'d',0,0);
      }
      if(p_grid->t) for(x=0;x<p_ui->w-0;x++) {
        r_char(p_ui->x+p_view->x+x,p_ui->y+p_view->y,'w',0,0);
      }
      if(p_grid->h*3+2-p_grid->t>p_ui->h) for(x=0;x<p_ui->w-0;x++) {
        r_char(p_ui->x+p_view->x+x,p_ui->y+p_view->y+p_ui->h-1,'s',0,0);
      }
      if(p_ui->state&8) {
        r_done(p_ui->x+p_view->x-1,p_ui->y+p_view->y-1,p_ui->w+2,p_ui->h+2);
      }
    }
  }
  if(p_ui->state&4) {
    grid_focus(p_ui);
  }
  p_ui->state=0;
*/
  grid_t *p_grid=p_ui->obj;
	view_t *p_view=p_ui->view;
	uint16_t x,y;
	uint8_t bg;
  if(p_ui->state&9) {
    r_white(p_ui->x+p_view->x-1,p_ui->y+p_view->y-1,p_ui->w+2,p_ui->h+2);
    if(p_grid->data) {
      for(y=1;y<2*p_grid->h+1;y++) {
        grid_cset(p_ui,0,y,'h',0);
        grid_cset(p_ui,p_grid->w*2+1,y,'g',0);
        if(((y-1)%10)==0) {
          grid_cset(p_ui,-1,y  ,'0'+(((y-1)/200)%10),0);
          grid_cset(p_ui,-1,y+1,'0'+(((y-1)/ 20)%10),0);
          grid_cset(p_ui,-1,y+2,'0'+(((y-1)/  2)%10),0);
          grid_cset(p_ui,-2,y  ,'0'+(((y-1)/200)%10),0);
          grid_cset(p_ui,-2,y+1,'0'+(((y-1)/ 20)%10),0);
          grid_cset(p_ui,-2,y+2,'0'+(((y-1)/  2)%10),0);
        }
      }
      for(x=1;x<2*p_grid->w+1;x++) {
        grid_cset(p_ui,x,0,'b',0);
        grid_cset(p_ui,x,p_grid->h*2+1,'y',0);
        if(((x-1)%10)==0) {
          grid_cset(p_ui,x  ,-1,'0'+(((x-1)/200)%10),0);
          grid_cset(p_ui,x+1,-1,'0'+(((x-1)/ 20)%10),0);
          grid_cset(p_ui,x+2,-1,'0'+(((x-1)/  2)%10),0);
          grid_cset(p_ui,x  ,-2,'0'+(((x-1)/200)%10),0);
          grid_cset(p_ui,x+1,-2,'0'+(((x-1)/ 20)%10),0);
          grid_cset(p_ui,x+2,-2,'0'+(((x-1)/  2)%10),0);
        }
      }
      grid_cset(p_ui,0,0,'m',0);
      grid_cset(p_ui,p_grid->w*2+1,0,'c',0);
      grid_cset(p_ui,0,p_grid->h*2+1,'r',0);
      grid_cset(p_ui,p_grid->w*2+1,p_grid->h*2+1,'i',0);
      for(y=0;y<p_grid->h;y++) {
        for(x=0;x<p_grid->w;x++) {
          bg=(p_grid->data[x+y*p_grid->w]<0x80)?1:0;
          grid_cset(p_ui,x*2+1,y*2+1,'t',bg);
          grid_cset(p_ui,x*2+2,y*2+1,'u',bg);
          grid_cset(p_ui,x*2+1,y*2+2,'v',bg);
          grid_cset(p_ui,x*2+2,y*2+2,'n',bg);
        }
      }
      if(p_grid->l) for(y=0;y<p_ui->h-0;y++) {
        r_char(p_ui->x+p_view->x,p_ui->y+p_view->y+y,'a',0,0);
      }
      if(p_grid->w*2+2-p_grid->l>p_ui->w) for(y=0;y<p_ui->h-0;y++) {
        r_char(p_ui->x+p_view->x+p_ui->w-1,p_ui->y+p_view->y+y,'d',0,0);
      }
      if(p_grid->t) for(x=0;x<p_ui->w-0;x++) {
        r_char(p_ui->x+p_view->x+x,p_ui->y+p_view->y,'w',0,0);
      }
      if(p_grid->h*2+2-p_grid->t>p_ui->h) for(x=0;x<p_ui->w-0;x++) {
        r_char(p_ui->x+p_view->x+x,p_ui->y+p_view->y+p_ui->h-1,'s',0,0);
      }
      if(p_ui->state&8) {
        r_done(p_ui->x+p_view->x-1,p_ui->y+p_view->y-1,p_ui->w+2,p_ui->h+2);
      }
    }
  }
  if(p_ui->state&4) {
    grid_focus(p_ui);
  }
  p_ui->state=0;
}

static bool grid_key(uiobj_t *p_ui,SDL_Keycode sym) {
/*
  grid_t *p_grid=p_ui->obj;
  switch(sym) {
    case SDLK_LEFT:
      if(p_grid->x>0) {
        p_grid->x--;
        p_ui->state|=4;
      }
      break;
    case SDLK_RIGHT:
      if(p_grid->x<p_grid->w-1) {
        p_grid->x++;
        p_ui->state|=4;
      }
      break;
    case SDLK_UP:
      if(p_grid->y>0) {
        p_grid->y--;
        p_ui->state|=4;
      }
      break;
    case SDLK_DOWN:
      if(p_grid->y<p_grid->h-1) {
        p_grid->y++;
        p_ui->state|=4;
      }
      break;
    case SDLK_SPACE:
      p_grid->data[p_grid->x+p_grid->y*p_grid->w]=p_grid->data[p_grid->x+p_grid->y*p_grid->w]<0x80?0xFF:0x00;
      p_ui->state|=8;
      return true;
  }
  
  if((p_grid->x*3+2)-2<p_grid->l) {
    p_grid->l=(p_grid->x*3+2)-2;
    p_ui->state|=8;
  } else if( (p_grid->x*3+2)+3>p_grid->l+p_ui->w ) {
    p_grid->l=(p_grid->x*3+2)+3-p_ui->w;
    p_ui->state|=8;
  }

  if((p_grid->y*3+2)-2<p_grid->t) {
    p_grid->t=(p_grid->y*3+2)-2;
    p_ui->state|=8;
  } else if( (p_grid->y*3+2)+3>p_grid->t+p_ui->h ) {
    p_grid->t=(p_grid->y*3+2)+3-p_ui->h;
    p_ui->state|=8;
  }
  
  
  return false;
*/
  grid_t *p_grid=p_ui->obj;
  switch(sym) {
    case SDLK_LEFT:
      if(p_grid->x>0) {
        p_grid->x--;
        p_ui->state|=4;
      }
      break;
    case SDLK_RIGHT:
      if(p_grid->x<p_grid->w-1) {
        p_grid->x++;
        p_ui->state|=4;
      }
      break;
    case SDLK_UP:
      if(p_grid->y>0) {
        p_grid->y--;
        p_ui->state|=4;
      }
      break;
    case SDLK_DOWN:
      if(p_grid->y<p_grid->h-1) {
        p_grid->y++;
        p_ui->state|=4;
      }
      break;
    case SDLK_SPACE:
      p_grid->data[p_grid->x+p_grid->y*p_grid->w]=p_grid->data[p_grid->x+p_grid->y*p_grid->w]<0x80?0xFF:0x00;
      p_ui->state|=8;
     	break;
    default:
    	return true;
  }
  
  if((p_grid->x*2+2)-2<p_grid->l) {
    p_grid->l=(p_grid->x*2+2)-2;
    p_ui->state|=8;
  } else if( (p_grid->x*2+2)+2>p_grid->l+p_ui->w ) {
    p_grid->l=(p_grid->x*2+2)+2-p_ui->w;
    p_ui->state|=8;
  }

  if((p_grid->y*2+2)-2<p_grid->t) {
    p_grid->t=(p_grid->y*2+2)-2;
    p_ui->state|=8;
  } else if( (p_grid->y*2+2)+2>p_grid->t+p_ui->h ) {
    p_grid->t=(p_grid->y*2+2)+2-p_ui->h;
    p_ui->state|=8;
  }
  
  return sym=SDLK_SPACE;

}

/** TEXT INPUT ******************************************************/

uiobj_t* ui_text(uint8_t x,uint8_t y,uint8_t w,char *caption) {
	uiobj_t *p_ui=malloc(sizeof(uiobj_t));
	text_t *p_obj=malloc(sizeof(list_t));
	p_ui->x=x;
	p_ui->y=y;
	p_ui->w=w;
	p_ui->h=1;
	p_obj->caption=malloc(strlen(caption)+1);
	strcpy(p_obj->caption,caption);
	p_obj->text=malloc(w-strlen(caption)+1);
	p_obj->text[0]=0;
	p_obj->valid=false;
	p_ui->type=UI_TEXT;
	p_ui->obj=p_obj;
	return p_ui;
}

void ui_text_set(uiobj_t *p_ui,char *p_replace) {
	text_t *p_text=p_ui->obj;
	strcpy(p_text->text,p_replace);
	p_ui->state|=2;
}

char* ui_text_get(uiobj_t *p_ui) {
	text_t *p_text=p_ui->obj;
	return p_text->text;
}

void ui_text_valid(uiobj_t *p_ui,bool valid) {
	text_t *p_text=p_ui->obj;
	if(p_text->valid!=valid) {
		p_text->valid=valid;
		if(p_ui->enabled) p_ui->state|=2;
	}
}

bool ui_text_isvalid(uiobj_t *p_ui) {
	text_t *p_text=p_ui->obj;
	return p_text->valid;
}

static void text_draw(uiobj_t *p_ui) {
	text_t *p_text=p_ui->obj;
	view_t *p_view=p_ui->view;
	if(p_ui->state&3) {
		r_write(p_view->x+p_ui->x,p_view->y+p_ui->y,p_text->caption,p_ui->enabled?(p_text->valid?2:1):0);
		if(p_ui->state&2) {
			r_flash(p_view->x+p_ui->x,p_view->y+p_ui->y,strlen(p_text->caption),1);
		}
	}
	if(p_ui->state&5) {
		r_write(p_view->x+p_ui->x+strlen(p_text->caption),p_view->y+p_ui->y,p_text->text,p_ui->enabled?3:0);
		r_white(
			p_view->x+p_ui->x+strlen(p_text->caption)+strlen(p_text->text),p_view->y+p_ui->y,
			p_ui->w-strlen(p_text->caption)+strlen(p_text->text),1
		);
	}
	p_ui->state=0;
}

static void text_focus(uiobj_t *p_ui) {
	text_t *p_text=p_ui->obj;
	view_t *p_view=p_ui->view;
	r_cins(p_view->x+p_ui->x+strlen(p_text->caption)+strlen(p_text->text),p_view->y+p_ui->y,0);
}

static bool text_key(uiobj_t *p_ui,SDL_Keycode sym) {
	text_t *p_text=p_ui->obj;
	char key[2];
	if((sym>=SDLK_0&&sym<=SDLK_9)||sym==SDLK_BACKSPACE) {
		// numbers and backspace
		if(sym==SDLK_BACKSPACE) {
			if(strlen(p_text->text)) {
				p_text->text[strlen(p_text->text)-1]=0;
				p_ui->state|=4;
				text_focus(p_ui);
				return true;
			}
		} else {
			if(strlen(p_text->caption)+strlen(p_text->text)<p_ui->w) {
				key[0]=sym-SDLK_0+'0';
				key[1]=0;
				strcat(p_text->text,key);
				p_ui->state|=4;
				text_focus(p_ui);
				return true;
			}
		}
	}
	return false;
}

/** LISTS ***********************************************************/

uiobj_t* ui_list(uint8_t x,uint8_t y,uint8_t w,uint8_t h,char *caption) {
	uiobj_t *p_ui=malloc(sizeof(uiobj_t));
	list_t *p_obj=malloc(sizeof(list_t));
	p_ui->x=x;
	p_ui->y=y;
	p_ui->w=w;
	p_ui->h=h;
	p_obj->caption=malloc(strlen(caption)+1);
	strcpy(p_obj->caption,caption);
	p_obj->selected=NULL;
	p_obj->count=0;
	p_obj->top=0;
	p_obj->list=NULL;
	p_ui->type=UI_LIST;
	p_ui->obj=p_obj;
	return p_ui;
}

static void list_scroll(uiobj_t *p_ui,uint16_t top) {
	list_t *p_list=p_ui->obj;
	p_list->top=top;
	if(p_list->count<(p_list->top+p_ui->h-2)) {
		if(p_ui->h-2>p_list->count) p_list->top=0;
		else p_list->top=p_list->count-(p_ui->h-2);
	}
	ui_enable(p_ui->next,p_list->top);
	ui_enable(p_ui->next->next,p_list->count>=p_list->top+p_ui->h-1);
	p_ui->state|=2;
}

static void list_draw(uiobj_t *p_ui) {
	list_t *p_list=p_ui->obj;
	view_t *p_view=p_ui->view;
	listitem_t *p_item;
	bool selected;
	char *p_text;
	uint8_t y;
	if(p_ui->state&1) {
		r_box(p_view->x+p_ui->x,p_view->y+p_ui->y,p_ui->w,p_ui->h,p_list->caption,0,1);
	}
	if(p_ui->state&3) {
		p_text=malloc(p_ui->w-1);
		p_text[p_ui->w-2]=0;
		p_item=ui_list_index2item(p_ui,p_list->top);
		r_white(p_view->x+p_ui->x+1,p_view->y+p_ui->y+1,p_ui->w-2,p_ui->h-2);
		for(y=0;(y<p_ui->h-2)&&p_item;y++) {
			if(strlen(p_item->text)>p_ui->w-2) {
				memcpy(p_text,p_item->text,p_ui->w-2);
    		p_text[p_ui->w-3]='ù';
			} else {
				strcpy(p_text,p_item->text);
			}
			selected=(p_item==p_list->selected);
			p_item=p_item->next;
			r_write(p_view->x+p_ui->x+1,p_view->y+p_ui->y+1+y,p_text,selected?3:0);
		}
		if((p_ui->state&3)==2) r_done(p_view->x+p_ui->x+1,p_view->y+p_ui->y+1,p_ui->w-2,p_ui->h-2);
		free(p_text);
	}

	p_ui->state=0;
}

uint16_t ui_list_item2index(uiobj_t *p_ui,listitem_t *p_i) {
	uint16_t index=0;
	list_t *p_list=p_ui->obj;
	listitem_t *p_item;
	p_item=p_list->list;
	while(p_item) {
		if(p_item==p_i) return index;
		p_item=p_item->next;
		index++;
	}
	return -1;
}

listitem_t* ui_list_index2item(uiobj_t *p_ui,uint16_t ix) {
	uint16_t index=0;
	list_t *p_list=p_ui->obj;
	listitem_t *p_item;
	p_item=p_list->list;
	if(p_item==NULL) return NULL;
	while(index<ix) {
		if(p_item->next==NULL) return NULL;
		p_item=p_item->next;
		index++;
	}
	return p_item;
}

void ui_list_remove(uiobj_t *p_ui,listitem_t *p_item) {
	list_t *p_list=p_ui->obj;

	//printf("REM: %i < %i > %i\n",p_item->prev,p_item,p_item->next);

	if(p_item->prev) {
		p_item->prev->next=p_item->next;
	} else {
		p_list->list=p_item->next;
	}
	if(p_item->next) p_item->next->prev=p_item->prev;
	p_list->count--;
	list_scroll(p_ui,p_list->top);
	if(p_list->selected==p_item) p_list->selected=NULL;

	free(p_item->text);
	if(p_item->data) free(p_item->data);
	free(p_item);
}

void ui_list_clear(uiobj_t *p_ui) {
	list_t *p_list=p_ui->obj;
	while(p_list->count)ui_list_remove(p_ui,p_list->list);
}

listitem_t* ui_list_add(uiobj_t *p_ui,char *text, size_t n_data) {
	list_t *p_list=p_ui->obj;
	listitem_t *p_item=malloc(sizeof(listitem_t));
	listitem_t *p_temp;
	p_item->text=malloc(strlen(text)+1);
	strcpy(p_item->text,text);
	capstr(p_item->text);
	if(n_data) {
		p_item->data=malloc(n_data);
	} else p_item->data=NULL;
	p_item->next=NULL;
	if(p_list->list) {
		p_temp=p_list->list;
		while(p_temp->next) p_temp=p_temp->next;
		p_temp->next=p_item;
		p_item->prev=p_temp;
	} else {
		p_list->list=p_item;
		p_item->prev=NULL;
	}
	p_list->count++;
	list_scroll(p_ui,p_list->top);
	//printf("ADD: %i = %s\n",p_item,text);
	return p_item;
}

bool ui_list_selectindex(uiobj_t *p_ui,uint8_t index) {
	list_t *p_list=p_ui->obj;
	if(p_list->top+index>=p_list->count) return false;
	p_list->selected=ui_list_index2item(p_ui,p_list->top+index);
	//printf("SELECT: %i = %i\n",index,p_list->selected);
	p_ui->state|=2;
	return true;
}

void ui_list_select(uiobj_t *p_ui,listitem_t *p_item) {
	list_t *p_list=p_ui->obj;
	p_list->selected=p_item;
	p_ui->state|=2;
}

listitem_t* ui_list_selected(uiobj_t *p_ui) {
	return ((list_t*)p_ui->obj)->selected;
}

uint16_t ui_list_selectedindex(uiobj_t *p_ui) {
	return ui_list_item2index(p_ui,((list_t*)p_ui->obj)->selected);
}


/** BUTTONS *********************************************************/

uiobj_t* ui_button(uint8_t x,uint8_t y,char *caption) {
	uiobj_t *p_ui=malloc(sizeof(uiobj_t));
	button_t *p_obj=malloc(sizeof(button_t));
	p_ui->x=x;
	p_ui->y=y;
	p_ui->w=strlen(caption)+2;
	p_ui->h=3;
	p_obj->caption=malloc(strlen(caption)+1);
	strcpy(p_obj->caption,caption);
	p_ui->type=UI_BUTTON;
	p_ui->obj=p_obj;
	return p_ui;
}

void ui_button_set(uiobj_t *p_ui,char *caption) {
	button_t *p_button=p_ui->obj;
	if(strlen(caption)==strlen(p_button->caption)) {
		strcpy(p_button->caption,caption);
		p_ui->state|=2;
	}
}

static void button_draw(uiobj_t *p_ui) {
	button_t *p_button=p_ui->obj;
	view_t *p_view=p_ui->view;
	if(p_ui->state&1) {
		r_button(p_view->x+p_ui->x,p_view->y+p_ui->y,p_button->caption,0,p_ui->enabled?2:0);
	} else if(p_ui->state&2) {
		r_write(p_view->x+p_ui->x+1,p_view->y+p_ui->y+1,p_button->caption,p_ui->enabled?2:0);
		r_flash(p_view->x+p_ui->x+1,p_view->y+p_ui->y+1,strlen(p_button->caption),1);
	}
	p_ui->state=0;
}

/** FIELDS **********************************************************/

static void field_draw(uiobj_t *p_ui) {
	field_t *p_field=p_ui->obj;
	view_t *p_view=p_ui->view;
	char *p_line;
	uint8_t n;
	if(p_ui->state&1) {
		if(p_field->caption) {
			r_box(p_view->x+p_ui->x,p_view->y+p_ui->y,p_ui->w,p_ui->h,p_field->caption,0,1);
		}
	}
	if(p_ui->state&5) {
		p_line=malloc(p_ui->w-1);
		memset(p_line,0,p_ui->w-1);
		// clear last line
		if((p_ui->state&4)&&(p_field->terminal)) r_white(p_view->x+p_ui->x+1,p_view->y+p_ui->y+p_ui->h-2,p_ui->w-2,1);
		for(n=0;n<p_ui->h-2;n++) {
			memcpy(p_line,p_field->text+n*(p_ui->w-2),p_ui->w-2);
			r_write(p_view->x+p_ui->x+1,p_view->y+p_ui->y+1+n,p_line,3);
			if((n<p_ui->h-3)&&(p_ui->state&4)&&(p_field->terminal)) {
				r_done(p_view->x+p_ui->x+1,p_view->y+p_ui->y+1+n,p_ui->w-2,1);
			}
		}
	}
	p_ui->state=0;
}

static void ui_field_addline(uiobj_t*p_ui,char*p_line,uint8_t len) {
	field_t*p_field;
	char *p_dst;
	uint8_t n;
	p_field=p_ui->obj;
	memmove(p_field->text,p_field->text+p_ui->w-2,(p_ui->w-2)*(p_ui->h-3));
	if(len>p_ui->w-2)len=p_ui->w-2;
	p_dst=p_field->text+(p_ui->w-2)*(p_ui->h-3);
	if(p_field->center) for(n=0;n<(p_ui->w-2-len)/2;n++) *p_dst++=' ';
	memcpy(p_dst,p_line,len);
	p_dst+=len;
	while(p_dst<p_field->text+(p_ui->w-2)*(p_ui->h-2)) *p_dst++=' ';
}

void ui_field_add(uiobj_t*p_ui,char*p_text) {
	field_t*p_field;
	uint8_t len=0;
	if(p_ui->type!=UI_FIELD) return;
	p_field=p_ui->obj;
	char *p_line=p_text;
	if(!(p_ui->state&1)) p_ui->state|=4;
	while(1) {
		if(*p_text=='\n'||*p_text==0) {
			if(len) ui_field_addline(p_ui,p_line,len);
			if(*p_text==0) return;
			len=0;
			p_text++;
			p_line=p_text;
		} else {
			p_text++;
			len++;
		}
	}
}

uiobj_t* ui_field(uint8_t x,uint8_t y,uint8_t w,uint8_t h,char *caption,bool center,bool terminal) {
	uiobj_t *p_ui=malloc(sizeof(uiobj_t));
	field_t *p_obj=malloc(sizeof(field_t));
	p_ui->x=x;
	p_ui->y=y;
	p_ui->w=w;
	p_ui->h=h;
	if(caption) {
		p_obj->caption=malloc(strlen(caption)+1);
		strcpy(p_obj->caption,caption);
	} else p_obj->caption=NULL;
	p_obj->center=center;
	p_obj->terminal=terminal;
	p_obj->text=malloc((w-2)*(h-2));
	memset(p_obj->text,' ',(w-2)*(h-2));
	p_ui->type=UI_FIELD;
	p_ui->obj=p_obj;
	return p_ui;
}

/** USER INTERFACE **************************************************/

uiobj_t* ui_add(view_t *p_view,uiobj_t *p_add) {
	uiobj_t *p_ui;
	p_add->next=NULL;
	p_add->view=p_view;
	p_add->enabled=true;
	p_add->link=NULL;
	if(p_view->ui) {
		p_ui=p_view->ui;
		while(p_ui->next) p_ui=p_ui->next;
		p_ui->next=p_add;
	} else {
		p_view->ui=p_add;
	}
	if(p_add->type==UI_LIST) {
		p_ui=ui_button(p_add->x+p_add->w,p_add->y,"®");
		ui_add(p_view,p_ui);
		p_ui->link=p_add;
		ui_enable(p_ui,false);
		p_ui=ui_button(p_add->x+p_add->w,p_add->y+p_add->h-3,"¯");
		ui_add(p_view,p_ui);
		p_ui->link=p_add;
		ui_enable(p_ui,false);
	}
	return p_add;
}

void ui_free_view(view_t *p_view) {
	uiobj_t *p_ui,*p_nui;
	void *p_obj;
	p_ui=p_view->ui;
	while(p_ui) {
		p_obj=p_ui->obj;
		switch(p_ui->type) {
			case UI_FIELD:
				if(((field_t*)p_obj)->caption) free(((field_t*)p_obj)->caption);
				free(((field_t*)p_obj)->text);
				break;
			case UI_BUTTON:
				free(((button_t*)p_obj)->caption);
				break;
			case UI_LIST:
				ui_list_clear(p_ui);
				free(((list_t*)p_obj)->caption);
				break;
			case UI_TEXT:
				free(((text_t*)p_obj)->caption);
				free(((text_t*)p_obj)->text);
				break;
		}
		free(p_obj);
		p_nui=p_ui->next;
		free(p_ui);
		p_ui=p_nui;
	}
	free(p_view);
}


static void ui_refresh(view_t *p_view) {
	uiobj_t *p_ui;
	p_ui=p_view->ui;
	while(p_ui) {
		switch(p_ui->type) {
			case UI_FIELD:  field_draw(p_ui);
				break;
			case UI_BUTTON: button_draw(p_ui);
				break;
			case UI_LIST:   list_draw(p_ui);
				break;
			case UI_TEXT:   text_draw(p_ui);
				break;
			case UI_GRID:   grid_draw(p_ui);
				break;
		}
		p_ui=p_ui->next;
	}
}

static void ui_reset(view_t *p_view) {
	uiobj_t *p_ui;
	p_ui=p_view->ui;
	p_view->focus=NULL;
	while(p_ui) {
		p_ui->state=1;
		p_ui=p_ui->next;
	}
}

static void ui_focusprev(view_t *p_view) {
	uiobj_t *p_ui, *p_found=NULL;
	p_ui=p_view->focus;
	do {
		if(p_ui==NULL) p_ui=p_view->ui;
		else p_ui=p_ui->next;
		if(p_ui&&p_ui!=p_view->focus) {
			if(p_ui->enabled) {
				switch(p_ui->type) {
					case UI_TEXT: case UI_GRID:
						p_found=p_ui;
						break;
				}
			}
		}
	} while(p_ui!=p_view->focus);
	if(p_found) {
		p_view->focus=p_found;
		switch(p_found->type) {
			case UI_TEXT:
				text_focus(p_found);
				break;
			case UI_GRID:
				grid_focus(p_found);
				break;
		}
	}
}

static void ui_focusnext(view_t *p_view) {
	uiobj_t *p_ui;
	p_ui=p_view->focus;
	do {
		if(p_ui==NULL) p_ui=p_view->ui;
		else p_ui=p_ui->next;
		if(p_ui) {
			if(p_ui->enabled) {
				switch(p_ui->type) {
					case UI_TEXT:
						p_view->focus=p_ui;
						text_focus(p_ui);
						return;
					case UI_GRID:
						p_view->focus=p_ui;
						grid_focus(p_ui);
						return;
				}
			}
		}
	} while(p_ui!=p_view->focus);
}

static uiobj_t *find(view_t *p_view,uint8_t x,uint8_t y) {
	uiobj_t *p_ui;
	p_ui=p_view->ui;
	while(p_ui) {
		if((p_ui->x+p_view->x)<=x&&(p_ui->x+p_ui->w+p_view->x)>x) {
			if((p_ui->y+p_view->y)<=y&&(p_ui->y+p_ui->h+p_view->y)>y) {
				if(p_ui->enabled) return p_ui;
			}
		}
		p_ui=p_ui->next;
	}
	return NULL;
}

void ui_display(view_t *p_view,bool(*events)(SDL_Event *e)) {
	bool done=false;
	uint8_t x,y,lx,ly;
	uiobj_t *p_ui_over=NULL,*p_ui,*p_ui_down=NULL;
	SDL_Event e;
#ifdef SDL2
  bool drop=false;
#endif
	ui_reset(p_view);
	ui_focusnext(p_view);
	if(!p_view->fullscreen) {
		r_white(p_view->x-3,p_view->y-3,p_view->w+6,p_view->h+6);
		r_box(p_view->x-2,p_view->y-2,p_view->w+4,p_view->h+4,p_view->caption,0,3);
	} else {
 		r_white(0,0,40,30);
	  if(was_fullscreen) {
	    p_view->state=0;
	  } else {
  		p_view->state=1;
	  }
	}
	was_fullscreen=p_view->fullscreen;
	while(!done) {
		SDL_mutexP(mx_ui);
		ui_refresh(p_view);
		if(p_view->state==1) {
			r_done(0,0,40,30);
			p_view->state=2;
		}
		r_draw();
	  while(SDL_PollEvent(&e)) {
	  	switch(e.type) {
#ifdef SDL2
	  		case SDL_DROPFILE:
	  		  done|=events(&e);
	  			SDL_free(e.drop.file);
	  			drop=true;
	  			break;
#endif
		    case SDL_KEYDOWN:
		    	done|=events(&e);
		    	if(e.key.keysym.sym==SDLK_ESCAPE) { //esc
		    		if(p_view->cancel) {
		    			if(p_view->cancel->enabled) {
		  					e.type=UI_CLICK;
		  					e.user.data1=p_view->cancel;
		  					done|=events(&e);
	  					}
		    		}
		    	} if(e.key.keysym.sym==SDLK_RETURN) { //enter
		    		if(p_view->ok) {
		    			if(p_view->ok->enabled) {
		  					e.type=UI_CLICK;
		  					e.user.data1=p_view->ok;
		  					done|=events(&e);
		  				}
		    		}
		    	} if(e.key.keysym.sym==SDLK_TAB) {
		    		ui_focusnext(p_view);
		    	} else {
		    		if(p_view->focus) {
		    			switch(p_view->focus->type) {
		    				case UI_TEXT:
		    					if(text_key(p_view->focus,e.key.keysym.sym)) {
	  								e.type=UI_CHANGE;
	  								e.user.data1=p_view->focus;
		    						done|=events(&e);
		    					}
		    					break;
		    				case UI_GRID:
		    					if(grid_key(p_view->focus,e.key.keysym.sym)) {
	  								e.type=UI_CHANGE;
	  								e.user.data1=p_view->focus;
		    						done|=events(&e);
		    					}
		    					break;
		    			}
		    		}
		    	}
			    break;
	  		case SDL_MOUSEMOTION:
  				x=e.motion.x>>4;
  				y=e.motion.y>>4;
	  			p_ui=find(p_view,x,y);
	  			if(p_ui!=p_ui_over) {
	  				if(p_ui_down) {
	  					if(p_ui==p_ui_down) {
	  						r_offset(p_ui_down->x+p_view->x,p_ui_down->y+p_view->y,p_ui_down->w,p_ui_down->h,2);
		  				} else {
	  						r_offset(p_ui_down->x+p_view->x,p_ui_down->y+p_view->y,p_ui_down->w,p_ui_down->h,0);
		  				}
	  				} else {
		  				if(p_ui_over) {
		  					e.type=UI_MOUSEOUT;
		  					e.user.data1=p_ui_over;
		  					done|=events(&e);
		  				}
		  				if(p_ui) {
		  					e.type=UI_MOUSEOVER;
		  					e.user.data1=p_ui;
		  					done|=events(&e);
		  					if(p_ui->type==UI_BUTTON||p_ui->type==UI_TEXT) {
		  						r_shine(p_ui->x+p_view->x,p_ui->y+p_view->y,p_ui->w,p_ui->h);
		  					}
		  				}
		  			}
	  				p_ui_over=p_ui;
	  			}
	  			if(p_ui) {
	  				if(y!=ly&&p_ui->type==UI_LIST) {
		  				if(x>p_ui->x+p_view->x&&x<p_ui->x+p_view->x+p_ui->w-1
		  				 &&y>p_ui->y+p_view->y&&y<p_ui->y+p_view->y+p_ui->h-1) {
		  					r_shine(p_ui->x+p_view->x+1,y,p_ui->w-2,1);
		  				}
	  				}
	  			}
	  			lx=x;
	  			ly=y;
	  			break;
	  		case SDL_MOUSEBUTTONDOWN:
	  			if(e.button.button==1) {
	  				x=e.motion.x>>4;
	  				y=e.motion.y>>4;
	  				p_ui_down=find(p_view,x,y);
	  				if(p_ui_down) {
		  				switch(p_ui_down->type) {
		  					case UI_GRID:
			  					if(x>p_ui->x+p_view->x&&x<p_ui->x+p_view->x+p_ui->w-1
				  				 &&y>p_ui->y+p_view->y&&y<p_ui->y+p_view->y+p_ui->h-1) {
				  					((grid_t*)p_ui->obj)->x=(x-(p_ui->x+p_view->x)+((grid_t*)p_ui->obj)->l-1)>>1;
				  					((grid_t*)p_ui->obj)->y=(y-(p_ui->y+p_view->y)+((grid_t*)p_ui->obj)->t-1)>>1;
			  						p_view->focus=p_ui_down;
			  						grid_focus(p_ui_down);
			  						grid_key(p_ui_down,SDLK_SPACE);
				  				}
		  					  break;
		  					case UI_BUTTON: case UI_TEXT:
		  						r_offset(p_ui->x+p_view->x,p_ui->y+p_view->y,p_ui->w,p_ui->h,2);
		  						break;
		  					case UI_LIST:
		  						x-=p_ui->x+p_view->x;
		  						y-=p_ui->y+p_view->y;
		  						if(x>0&&x<p_ui->w-1&&y>0&&y<p_ui->h-1) {
		  							if(ui_list_selectindex(p_ui,y-1)) {
		  								e.type=UI_CHANGE;
		  								e.user.data1=p_ui_down;
									    done|=events(&e);
		  							}	
		  						}
		  						p_ui_down=NULL;
		  					default:
		  						p_ui_down=NULL;
		  				}
		  			}
	  			}
	  			break;
	  		case SDL_MOUSEBUTTONUP:
	  			if(e.button.button==1) {
	  				if(p_ui_down) {
		  				r_offset(p_ui_down->x+p_view->x,p_ui_down->y+p_view->y,p_ui_down->w,p_ui_down->h,0);
		  				if(p_ui_over==p_ui_down) {
		  					e.type=UI_CLICK;
		  					e.user.data1=p_ui_down;
		  					done|=events(&e);
		  					if(p_ui_down->type==UI_TEXT) {
		  						p_view->focus=p_ui_down;
		  						text_focus(p_ui_down);
		  					}
		  					if(p_ui_over->link) {
		  						if(p_ui_over->link->type==UI_LIST) {
		  							if(((button_t*)p_ui_over->obj)->caption[0]=='®') {
		  								if(((list_t*)p_ui_over->link->obj)->top>0) {
		  									list_scroll(p_ui_over->link,((list_t*)p_ui_over->link->obj)->top-1);
		  								}
		  							} else {
		  								list_scroll(p_ui_over->link,((list_t*)p_ui_over->link->obj)->top+1);
		  							}
		  						}
		  					}
		  				}
		  				p_ui_down=NULL;
		  			}
					}
					break;
	  		default:
			    done|=events(&e);
	  	}
	    if(done) break;
		}
#ifdef SDL2
		if(drop) {
      e.type=SDL_DROPFILE;
      e.drop.file=NULL;
 		  done|=events(&e);
		}
#endif
		SDL_mutexV(mx_ui);
		SDL_Delay( 10 );
	}
	r_crem();
}
