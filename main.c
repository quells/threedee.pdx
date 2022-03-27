#include "pd_api.h"

static PlaydateAPI* pd = NULL;

static int threelib_dealloc(lua_State* L);
static int threelib_init(lua_State* L);
static int threelib_draw(lua_State* L);

static const lua_reg threelib[] = {
	{ "__gc", threelib_dealloc },
	{ "init", threelib_init },
	{ "draw", threelib_draw },
	{ NULL, NULL }
};

int eventHandler(PlaydateAPI* playdate, PDSystemEvent event, uint32_t arg) {
	if (event == kEventInitLua) {
		pd = playdate;
		
		const char* err;
		if (!pd->lua->registerClass("threelib.triangles", threelib, NULL, 0, &err)) {
			pd->system->logToConsole("%s:%i: registerClass failed, %s", __FILE__, __LINE__, err);
		}
		
		pd->system->logToConsole("threelib init lua");
	}
	
	return 0;
}

typedef struct {
	int x1, y1, z1;
	int x2, y2, z2;
	int x3, y3, z3;
} Triangle;

typedef struct {
	int count;
	Triangle* triangles;
	int8_t* fb;
	int8_t bg;
} Triangles;

static int threelib_dealloc(lua_State* L) {
	Triangles* t = pd->lua->getArgObject(1, "threelib.triangles", NULL);

	if (t->triangles) pd->system->realloc(t->triangles, 0);
	pd->system->realloc(t->fb, 0);
	pd->system->realloc(t, 0);

	pd->system->logToConsole("dealloc");
	return 0;
}

static int threelib_init(lua_State* L) {
	Triangles* t = pd->system->realloc(NULL, sizeof(Triangles));
	t->count = 0;
	t->triangles = NULL;

	// clear framebuffer
	t->bg = 0;
	int num_pixels = LCD_COLUMNS * LCD_ROWS;
	t->fb = pd->system->realloc(NULL, sizeof(float) * num_pixels);
	for (int i = 0; i < num_pixels; i++) {
		t->fb[i] = 0;
	}

	pd->lua->pushObject(t, "threelib.triangles", 0);

	pd->system->logToConsole("triangles init");
	return 1;
}

void threelib_draw_fb(Triangles *t) {
	int8_t* fb = t->fb;

	// draw triangles - 0 black 127 white
	int fb_idx = 0;
	for (int y = 0; y < LCD_ROWS; y++) {
		for (int x = 0; x < LCD_COLUMNS; x++) {
			fb[fb_idx] = t->bg;

			fb_idx++;
		}
	}

	// dither - 0 black else white
	// atkinson
	//   * 1 1
	// 1 1 1  
	//   1
	fb_idx = 0;
	int8_t v, out, err;
	for (int y = 0; y < LCD_ROWS; y++) {
		for (int x = 0; x < LCD_COLUMNS; x++) {
			v = fb[fb_idx];
			out = (64 < v) ? 127 : 0;
			fb[fb_idx] = out;
			err = (v - out) / 8;
			if (x < LCD_COLUMNS-1) {
				fb[fb_idx + 1] += err;
				if (x < LCD_COLUMNS-2) {
					fb[fb_idx + 2] += err;
				}
			}
			if (y < LCD_ROWS-1) {
				if (0 < x) {
					fb[fb_idx + LCD_COLUMNS - 1] += err;
				}
				fb[fb_idx + LCD_COLUMNS] += err;
				if (x < LCD_COLUMNS-1) {
					fb[fb_idx + LCD_COLUMNS + 1] += err;
				}
				if (y < LCD_ROWS-2) {
					fb[fb_idx + LCD_COLUMNS*2] += err;
				}
			}

			fb_idx++;
		}
	}
}

static int threelib_draw(lua_State* L) {
	Triangles* t = pd->lua->getArgObject(1, "threelib.triangles", NULL);
	threelib_draw_fb(t);
	t->bg++;
	if (127 == t->bg) {
		t->bg = 0;
	}
	
	uint8_t* frame = pd->graphics->getFrame();
	int bitpos = 0x80; // 1000 0000
	int fb_idx = 0;
	for (int y = 0; y < LCD_ROWS; y++) {
		uint8_t* row = &frame[y * LCD_ROWSIZE];
		uint8_t chunk = 0x00; // black
		for (int x = 0; x < LCD_COLUMNS; x++) {
			if (t->fb[fb_idx]) {
				chunk ^= bitpos;
			}

			fb_idx++;
			bitpos >>= 1;
			if (!bitpos) {
				row[x >> 3] = chunk;
				bitpos = 0x80; // 1000 0000
				chunk = 0x00; // black
			}
		}
	}
	pd->graphics->markUpdatedRows(0, LCD_ROWS);
	
	return 0;
}
