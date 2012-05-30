#ifdef SDL2
void r_init( SDL_Renderer *ren, SDL_Surface *scr );
#else
void r_init( SDL_Surface *scr );
#endif
void r_clear();
void r_draw();
void r_write( unsigned char x, unsigned char y, char *s, unsigned char f );
void r_white( unsigned char x, unsigned char y, unsigned char w, unsigned char h );
void r_cins( unsigned char x, unsigned char y );
void r_crem();
void r_box( unsigned char x, unsigned char y, unsigned char w, unsigned char h, char*caption, unsigned char fb, unsigned char fc);
void r_button( unsigned char x, unsigned char y, char*caption, unsigned char fb, unsigned char fc);
void r_flash( unsigned char x, unsigned char y, unsigned char w, unsigned char h );
void r_shine( unsigned char x, unsigned char y, unsigned char w, unsigned char h );
void r_done( unsigned char x, unsigned char y, unsigned char w, unsigned char h );
void r_offset( unsigned char _x, unsigned char _y, unsigned char w, unsigned char h, char ox, char oy );
int  r_knows( char c );
void r_free();