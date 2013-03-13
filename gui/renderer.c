#ifdef SDL2
#include <SDL2/SDL.h>
#else
#include <SDL/SDL.h>
#endif
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "renderer.h"

typedef struct {
  uint8_t cchar;
  uint8_t nchar;
  uint8_t cfont;
  uint8_t nfont;
  int8_t  ofs;
  int8_t  bgc;
  int16_t tick;
} termchar_t;

static    termchar_t *term;
       			 uint8_t  r_w, r_h;
static   SDL_Surface *font;
static const    char  font_chars[] = "#!\"cù%ñ'()|+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZú®¯³ÄÙÀÚ¿´ÃºÍ¼ÈÉ»awdsgyhbnvtumcri";
static 		   uint8_t  write_index = 0;
#ifdef SDL2
static   SDL_Renderer *renderer;
#endif
static   SDL_Surface *screen;
static   SDL_Surface *bg;
static          char  cx = -1, cy, ct, cs;

#ifdef SDL2
static SDL_PixelFormat fmt_rgb = {
    0,NULL,32,4,{0,0},
		0xFF000000,0x00FF0000,
		0x0000FF00,0x000000FF,
		0,0,0,0,24,16,8,0,0,NULL

};
#else
static SDL_PixelFormat fmt_rgb = {
		NULL,32,4,0,0,0,0,24,16,8,0,
		0xFF000000,0x00FF0000,
		0x0000FF00,0x000000FF,
		0xFF00FF00,255
};
#endif

// Convenience macros
#define ABS( v ) ( v < 0 ? -v : v )
#define TERM( x, y ) term[ (y) * r_w + (x) ]

void r_cins( unsigned char x, unsigned char y, unsigned char style ) {
  cx = x;
  cy = y;
  ct = 0;
  cs = style;
}

void r_crem() {
  cx = -1;
}

void r_clear() {
  memset( term, 0, sizeof( termchar_t ) * r_w * r_h );
}

static trytake(uint8_t*pix,bool taken[32][24],uint8_t sx,uint8_t sy,uint8_t w,uint8_t h) {
	unsigned int x,y;
	uint8_t *p_px,c;
	float o,xf,yf;
	for(y=sy;y<sy+h;y++) {
		for(x=sx;x<sx+w;x++) {
			if(taken[x][y]) return;
		}
	}
	for(y=sy;y<sy+h;y++) {
		for(x=sx;x<sx+w;x++) {
			taken[x][y] = true;
		}
	}
	o=(((float)rand())*64.0/RAND_MAX)-96.0;
	xf=(((float)rand())*2.0/RAND_MAX)-1.0;
	yf=(((float)rand())*2.0/RAND_MAX)-1.0;
	xf/=w;
	yf/=h;
	for(y=sy*20;y<(sy+h)*20;y++) {
		p_px=&pix[2560*y+sx*80];
		for(x=sx*20;x<(sx+w)*20;x++) {
			c=o+(x-(sx*20))*xf+(y-(sy*20))*yf;
			*p_px++=255;
			*p_px++=c;
			*p_px++=c;
			*p_px++=c;
		}
	}		
}

static SDL_Surface *get_bg() {
	SDL_Surface *temp,*bg;
	bool taken[32][24];
	uint8_t n,x,y;
	memset(taken,false,sizeof(taken));
	bg=SDL_CreateRGBSurface(SDL_SWSURFACE,screen->w,screen->h,32,fmt_rgb.Rmask,fmt_rgb.Gmask,fmt_rgb.Bmask,fmt_rgb.Amask);	
	SDL_LockSurface(bg);
	for(n=0;n<  2;n++) trytake(bg->pixels,taken,rand()%(32-5),rand()%(24-5),5,5);
	for(n=0;n<  5;n++) trytake(bg->pixels,taken,rand()%(32-4),rand()%(24-4),4,4);
	for(n=0;n< 10;n++) trytake(bg->pixels,taken,rand()%(32-3),rand()%(24-3),3,3);
	for(n=0;n<250;n++) trytake(bg->pixels,taken,rand()%(32-2),rand()%(24-2),2,2);
	for(y=0;y<24;y++) {
		for(x=0;x<32;x++) {
			trytake(bg->pixels,taken,x,y,1,1);
		}
	}
	SDL_UnlockSurface(bg);
	temp=bg;
#ifdef SDL2
	bg=SDL_ConvertSurface(temp,screen->format,0);
	// SDL2 crashes on SDL_BlitSurface without this...
	SDL_SetColorKey(bg,SDL_TRUE,SDL_MapRGB(screen->format,0xFF,0x00,0xFF));
#else
	bg=SDL_DisplayFormat(temp);
#endif
	SDL_FreeSurface(temp);
	return bg;
}

static SDL_Surface *get_font() {
	SDL_Surface *temp, *font;
	int32_t count;
	uint8_t color,fade;
	uint8_t *p_src,*p_dst;
	uint8_t r,g,b,a;
  temp=SDL_LoadBMP("ui.bmp");
  if(!temp) return NULL;
  font=SDL_ConvertSurface(temp,&fmt_rgb,SDL_SWSURFACE);
  SDL_FreeSurface(temp);
  if(!font) return NULL;
	temp=font;
	font=SDL_CreateRGBSurface(SDL_SWSURFACE,temp->w,temp->h*16*4,32,fmt_rgb.Rmask,fmt_rgb.Gmask,fmt_rgb.Bmask,fmt_rgb.Amask);	
  if(!font) {
  	SDL_FreeSurface(temp);
  	return NULL;
  }
	SDL_LockSurface(font);
	SDL_LockSurface(temp);
	p_dst=font->pixels;
	for(color=0;color<4;color++) {
		for(fade=0;fade<16;fade++) {
			p_src=temp->pixels;
			for(count=0;count<temp->w*temp->h;count++) {
				a=*p_src++;b=*p_src++;g=*p_src++;r=*p_src++;
				if(r==255&&g==255&&b==255) {
					g=0;
				} else if(r>60) {
						if(color!=0&&color!=1) r=(r>>1)+(r>>2);
						if(color!=0&&color!=2) g=(g>>1)+(g>>2);
						if(color!=0&&color!=3) b=(b>>1)+(b>>2);
						if(color!=0) {
							r+=40;
							g+=40;
							b+=40;
						}
						r=255-(((255-r)*fade)/15);
						g=255-(((255-g)*fade)/15);
						b=255-(((255-b)*fade)/15);
				} else {
						if(color!=0&&color!=1) r=(r>>1)+(r>>2);
						if(color!=0&&color!=2) g=(g>>1)+(g>>2);
						if(color!=0&&color!=3) b=(b>>1)+(b>>2);
				}					
				*p_dst++=a;*p_dst++=b;*p_dst++=g;*p_dst++=r;
			}
		}
	}
	SDL_UnlockSurface(font);
	SDL_UnlockSurface(temp);
  SDL_FreeSurface(temp);
  temp=font;
#ifdef SDL2
	font=SDL_ConvertSurface(temp,screen->format,0);
  SDL_SetColorKey(font,SDL_TRUE,SDL_MapRGB(screen->format,0xFF,0x00,0xFF));
#else
	font=SDL_DisplayFormat(temp);
  SDL_SetColorKey(font,SDL_SRCCOLORKEY,SDL_MapRGB(screen->format,0xFF,0x00,0xFF));
#endif
	SDL_FreeSurface(temp);
	return font;
}

#ifdef SDL2
void r_init( SDL_Renderer *ren, SDL_Surface *scr ) {
#else
void r_init( SDL_Surface *scr ) {
#endif
  SDL_Surface* temp;
  char *p_pixel;
#ifdef SDL2
	renderer=ren;
#endif
  screen = scr;
  r_w = screen->w >> 4;
  r_h = screen->h >> 4;
  term = malloc( sizeof( termchar_t ) * r_w * r_h );
  r_clear();
	font=get_font();
	bg=get_bg();
}

void r_draw() {
  unsigned char x, y;
  unsigned char i, j;
  Uint32 SDL_color;
  write_index = 0;
  SDL_Rect srcrect;
  SDL_Rect dstrect;
  SDL_BlitSurface(bg,NULL,screen,NULL);
  srcrect.w = 16;
  srcrect.h = 16;
  for( y = 0; y < r_h; y++ ) {
    for( x = 0; x < r_w; x++ ) {
      if( TERM( x, y ).tick < 15 ) {
        if( ++TERM( x,  y ).tick == 0 ) {
          TERM( x, y ).cchar = TERM( x,  y ).nchar;
          TERM( x, y ).cfont = TERM( x,  y ).nfont;
        }
      }

      if( TERM( x, y ).cchar ) {
        dstrect.x = ( x << 4 ) + TERM( x, y ).ofs;
        dstrect.y = ( y << 4 ) + TERM( x, y ).ofs;
        srcrect.x = TERM( x, y ).cchar << 4;
        srcrect.y = ( TERM( x, y ).tick >= 0 ? TERM( x, y ).tick << 4 : 240 ) + TERM( x, y ).cfont * 256;
        if( TERM( x, y ).bgc ) {
          
          switch(0xF-TERM( x, y ).bgc) {
            case 0x0: SDL_color = SDL_MapRGB(screen->format,0x00,0x00,0x00);
              break;
            case 0x1: SDL_color = SDL_MapRGB(screen->format,0x00,0x00,0x7F);
              break;
            case 0x2: SDL_color = SDL_MapRGB(screen->format,0x00,0x7F,0x00);
              break;
            case 0x3: SDL_color = SDL_MapRGB(screen->format,0x00,0x7F,0x7F);
              break;
            case 0x4: SDL_color = SDL_MapRGB(screen->format,0x7F,0x00,0x00);
              break;
            case 0x5: SDL_color = SDL_MapRGB(screen->format,0x7F,0x00,0x7F);
              break;
            case 0x6: SDL_color = SDL_MapRGB(screen->format,0x7F,0x7F,0x00);
              break;
            case 0x7: SDL_color = SDL_MapRGB(screen->format,0x00,0x00,0xFF);
              break;
            case 0x8: SDL_color = SDL_MapRGB(screen->format,0x00,0xFF,0x00);
              break;
            default : SDL_color = SDL_MapRGB(screen->format,0x7F,0x7F,0x7F);
          }
          
          SDL_FillRect( screen, &dstrect, SDL_color );
        }
        SDL_BlitSurface( font, &srcrect, screen, &dstrect );
      }
    }
  }
  if( cx >= 0 ) { 
    if( cs == 0 ) {
	    srcrect.x = 0;
	    srcrect.y = ( ct > 15 ? 240 : ct << 4 );
	    dstrect.x = cx << 4;
	    dstrect.y = cy << 4;
	    SDL_BlitSurface( font, &srcrect, screen, &dstrect );
	  } else {
	  	for( x = 0; x < 2; x++ ) {
	  		for( y = 0; y < 2; y++ ) {
			    srcrect.x = TERM( x + cx, y + cy ).cchar << 4;;
			    srcrect.y = ( ( ( ct - ( ( x + y ) << 1 ) ) & 31 ) > 15 ? 240 : ( ( ct - ( ( x + y ) << 1 ) ) & 31 ) << 4 );
			    dstrect.x = ( x + cx ) << 4;
			    dstrect.y = ( y + cy ) << 4;
			    SDL_BlitSurface( font, &srcrect, screen, &dstrect );
	  		}
	  	}
	  }
    if( ++ct == 32 ) ct = 0;
  }
#ifdef SDL2
	SDL_RenderPresent(renderer);
#else
	SDL_UpdateRect( screen, 0, 0, 0, 0 );
#endif
}

void r_box( unsigned char x, unsigned char _y, unsigned char w, unsigned char h, char*caption, unsigned char fb, unsigned char fc ) {
	char *buf=malloc(w+1);
	unsigned char y=_y;
	buf[w]=0;
	memset(&buf[1],'Ä',w-2);
	buf[0]='Ú';
	buf[w-1]='¿';
	if(caption) {
		buf[1]='´';
		memset(&buf[2],' ',strlen(caption));
		buf[2+strlen(caption)]='Ã';
	}
	r_write(x,y,buf,fb);
	if(caption) r_write(x+2,y,caption,fc);
	memset(&buf[1],' ',w-2);
	buf[0]='³';
	buf[w-1]='³';
	for(y++;y<_y+h-1;y++) r_write(x,y,buf,fb);
	memset(&buf[1],'Ä',w-2);
	buf[0]='À';
	buf[w-1]='Ù';
	r_write(x,y,buf,fb);
	free(buf);
}

void r_button( unsigned char x, unsigned char y, char*caption, unsigned char fb, unsigned char fc) {
	char *buf;
	unsigned char w;
	w=strlen(caption)+2;
	buf=malloc(w+1);
	buf[w]=0;
	memset(&buf[1],'Í',w-2);
	buf[0]='É';
	buf[w-1]='»';
	r_write(x,y,buf,fb);
	memset(&buf[1],' ',w-2);
	buf[0]='º';
	buf[w-1]='º';
	r_write(x,++y,buf,fb);
	r_write(x+1,y++,caption,fc);
	memset(&buf[1],'Í',w-2);
	buf[0]='È';
	buf[w-1]='¼';
	r_write(x,y,buf,fb);
	free(buf);
}

void r_char( unsigned char x, unsigned char y, char c, unsigned char f, unsigned char b ) {
  unsigned char j;
  for( j = 0; j < sizeof( font_chars ) - 1; j++ ) {
    if( font_chars[ j ] == c || c == ' ' ) {
      if( TERM( x, y ).nchar != j || TERM( x, y ).nfont != f ) {
        TERM( x, y ).nchar = j;
        TERM( x, y ).nfont = f;
        TERM( x, y ).bgc = b;
        TERM( x, y ).tick = -( x + y );
      }
    }
  }
}

void r_write( unsigned char x, unsigned char y, char *s, unsigned char f ) {
  unsigned char i, j, t = 0;
  int len = strlen( s );
  for( i = 0; i < len; i++ ) {
    for( j = 0; j < sizeof( font_chars ) - 1; j++ ) {
      if( font_chars[ j ] == s[ i ] || s[ i ] == ' ' ) {
        if( TERM( x, y ).nchar != j || TERM( x, y ).nfont != f ) {
          TERM( x, y ).nchar = j;
          TERM( x, y ).nfont = f;
          if( t == 0 && write_index == 0 ) {
            TERM( x, y ).cchar = j;
            TERM( x, y ).cfont = f;
            TERM( x, y ).tick = 0;
          } else {
            TERM( x, y ).tick = -( t + write_index );
          }
          t++;
        }
        x++;
        break;
      }
    }
  }
  write_index++;
}

void r_flash( unsigned char _x, unsigned char _y, unsigned char w, unsigned char h ) {
	unsigned char x, y;
	for(y=0;y<h;y++) {
		for(x=0;x<w;x++) {
			TERM(x+_x,y+_y).cchar=TERM(x+_x,y+_y).nchar;
			TERM(x+_x,y+_y).cfont=TERM(x+_x,y+_y).nfont;
			TERM(x+_x,y+_y).tick=0;
		}
	}
}

void r_done( unsigned char _x, unsigned char _y, unsigned char w, unsigned char h ) {
	unsigned char x, y;
	for(y=0;y<h;y++) {
		for(x=0;x<w;x++) {
			TERM(x+_x,y+_y).cchar=TERM(x+_x,y+_y).nchar;
			TERM(x+_x,y+_y).cfont=TERM(x+_x,y+_y).nfont;
			TERM(x+_x,y+_y).tick=15;
		}
	}
}

void r_shine( unsigned char _x, unsigned char _y, unsigned char w, unsigned char h ) {
	unsigned char x, y;
	for(y=0;y<h;y++) {
		for(x=0;x<w;x++) {
			TERM(x+_x,y+_y).cchar=TERM(x+_x,y+_y).nchar;
			TERM(x+_x,y+_y).cfont=TERM(x+_x,y+_y).nfont;
			TERM(x+_x,y+_y).tick=-(x+y);
		}
	}
}

void r_offset( unsigned char _x, unsigned char _y, unsigned char w, unsigned char h, char o ) {
	unsigned char x, y;
	for(y=0;y<h;y++) {
		for(x=0;x<w;x++) {
			TERM(x+_x,y+_y).ofs=o;
		}
	}
}

void r_white( unsigned char _x, unsigned char _y, unsigned char w, unsigned char h ) {
	unsigned char x, y;
	for(y=0;y<h;y++) {
		for(x=0;x<w;x++) {
			memset( &TERM(x+_x,y+_y), 0, sizeof( termchar_t ) );
		}
	}
}

void r_free() {
  free( term );
  SDL_FreeSurface( font );
}

int r_knows( char c ) {
    int j;
    for( j = 0; j < sizeof( font_chars ) - 1; j++ ) {
      if( font_chars[ j ] == c ) return( 1 );
    }
    return( 0 );
}