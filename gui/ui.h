typedef struct listitem_t {
	void *data;
	char *text;
	struct listitem_t *prev;
	struct listitem_t *next;
} listitem_t;

typedef struct {
	char *caption;
	char *text;
	bool valid;
} text_t;

typedef struct {
	char *caption;
	listitem_t *list;
	uint16_t count;
	listitem_t *selected;
	uint16_t top;
} list_t;

typedef struct {
  uint16_t l;
  uint16_t t;
  uint16_t x;
  uint16_t y;
  uint16_t w;
  uint16_t h;
  uint8_t *data;
} grid_t;

typedef struct {
	char *caption;
} button_t;

typedef struct {
	char *caption;
	char *text;
	uint8_t center;
	bool terminal;
} field_t;

typedef struct uiobj_t {
	uint8_t x;
	uint8_t y;
	uint8_t w;
	uint8_t h;
	char type;
	void *obj;
	struct uiobj_t *next;
	struct uiobj_t *link;
	struct view_t *view;
	uint8_t state;
	bool enabled;
} uiobj_t;

typedef struct view_t {
	uint8_t x;
	uint8_t y;
	uint8_t w;
	uint8_t h;
	char *caption;
	bool fullscreen;
	struct uiobj_t *ui;
	uint8_t state;
	struct uiobj_t *focus;
	uiobj_t *ok;
	uiobj_t *cancel;
} view_t;

enum {
	UI_BUTTON='B',
	UI_LIST='L',
	UI_FIELD='F',
	UI_TEXT='T',
	UI_GRID='G',
	UI_END=' '
};

enum {
	UI_MOUSEOVER=SDL_USEREVENT,
	UI_MOUSEOUT=SDL_USEREVENT+1,
	UI_CLICK=SDL_USEREVENT+2,
	UI_CHANGE=SDL_USEREVENT+3
};

extern SDL_mutex *mx_ui;

void capstr(char *p_text);

view_t*     ui_fsview();
view_t*     ui_view(uint8_t w,uint8_t h,char *caption);
void        ui_free_view(view_t *p_view);

uiobj_t*    ui_grid(uint8_t x,uint8_t y,uint8_t w,uint8_t h);
void        ui_grid_set(uiobj_t *p_ui,void *p_data,uint16_t w,uint16_t h);

uiobj_t*    ui_text(uint8_t x,uint8_t y,uint8_t w,char *caption);
void        ui_text_set(uiobj_t *p_ui,char *p_text);
char*       ui_text_get(uiobj_t *p_ui);
void 				ui_text_valid(uiobj_t *p_ui,bool valid);
bool 				ui_text_isvalid(uiobj_t *p_ui);

uiobj_t*    ui_field(uint8_t x,uint8_t y,uint8_t w,uint8_t h,char *caption,bool center,bool terminal);
void        ui_field_add(uiobj_t *p_ui,char*p_text);

uiobj_t*    ui_button(uint8_t x,uint8_t y,char *caption);
void        ui_button_set(uiobj_t *p_ui,char *caption);

uiobj_t*    ui_list(uint8_t x,uint8_t y,uint8_t w,uint8_t h,char *caption);
listitem_t* ui_list_add(uiobj_t *p_ui,char *text, size_t n_data);
void        ui_list_remove(uiobj_t *p_ui,listitem_t *p_item);
void        ui_list_clear(uiobj_t *p_ui);
listitem_t* ui_list_index2item(uiobj_t *p_ui,uint16_t ix);
uint16_t    ui_list_item2index(uiobj_t *p_ui,listitem_t *p_i);
listitem_t* ui_list_selected(uiobj_t *p_ui);
uint16_t    ui_list_selectedindex(uiobj_t *p_ui);
bool 				ui_list_selectindex(uiobj_t *p_ui,uint8_t index);
void 				ui_list_select(uiobj_t *p_ui,listitem_t *p_item);

uiobj_t*    ui_add(view_t *p_view,uiobj_t *p_add);
void        ui_display(view_t *p_view,bool(*events)(SDL_Event *e));
void        ui_free();