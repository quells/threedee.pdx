#include "pd_api.h"

static PlaydateAPI* pd = NULL;

static int threelib_dealloc(lua_State* L);
static int threelib_init(lua_State* L);
static int threelib_set_background(lua_State* L);
static int threelib_add_triangle(lua_State* L);
static int threelib_update_triangle(lua_State* L);
static int threelib_remove_triangle(lua_State* L);
static int threelib_load_model(lua_State* L);
static int threelib_draw(lua_State* L);

static const lua_reg threelib[] = {
	{ "__gc", threelib_dealloc },
	{ "init", threelib_init },
	{ "setBackground", threelib_set_background },
	{ "add", threelib_add_triangle },
	{ "update", threelib_update_triangle },
	{ "remove", threelib_remove_triangle },
	{ "load_model", threelib_load_model },
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
	int x, y;
} Int2;

typedef struct {
	float x, y, z;
} Float3;

typedef struct {
	Float3 p0;
	Float3 p1;
	Float3 p2;
	int8_t color;
} Triangle;

typedef struct {
	int count;
	Triangle* triangles;
	int8_t* fb;
	int8_t bg;

	float camera_width;
	float camera_f;
} Triangles;

static int threelib_dealloc(lua_State* L) {
	Triangles* ts = pd->lua->getArgObject(1, "threelib.triangles", NULL);

	if (ts->triangles) pd->system->realloc(ts->triangles, 0);
	pd->system->realloc(ts->fb, 0);
	pd->system->realloc(ts, 0);

	pd->system->logToConsole("dealloc");
	return 0;
}

static int threelib_init(lua_State* L) {
	Triangles* ts = pd->system->realloc(NULL, sizeof(Triangles));
	ts->count = 0;
	ts->triangles = NULL;

	// zero frame buffer
	ts->bg = 127;
	int num_pixels = LCD_COLUMNS * LCD_ROWS;
	ts->fb = pd->system->realloc(NULL, sizeof(float) * num_pixels);
	for (int i = 0; i < num_pixels; i++) {
		ts->fb[i] = 0;
	}

	// setup camera
	ts->camera_width = 25.0; // Super 35 strip width in mm
	ts->camera_f = 35.0; // mm focal length

	pd->lua->pushObject(ts, "threelib.triangles", 0);

	pd->system->logToConsole("triangles init");
	return 1;
}

static int threelib_set_background(lua_State* L) {
	Triangles* ts = pd->lua->getArgObject(1, "threelib.triangles", NULL);
	ts->bg = pd->lua->getArgInt(2);
	return 0;
}

static int threelib_add_triangle(lua_State* L) {
	Triangles* ts = pd->lua->getArgObject(1, "threelib.triangles", NULL);
	float x1 = pd->lua->getArgFloat(2);
	float y1 = pd->lua->getArgFloat(3);
	float z1 = pd->lua->getArgFloat(4);
	float x2 = pd->lua->getArgFloat(5);
	float y2 = pd->lua->getArgFloat(6);
	float z2 = pd->lua->getArgFloat(7);
	float x3 = pd->lua->getArgFloat(8);
	float y3 = pd->lua->getArgFloat(9);
	float z3 = pd->lua->getArgFloat(10);
	int color = pd->lua->getArgInt(11);
	
	ts->count++;
	ts->triangles = pd->system->realloc(ts->triangles, sizeof(Triangle) * ts->count);
	Triangle* t = &ts->triangles[ts->count - 1];
	t->p0.x = x1; t->p0.y = y1; t->p0.z = z1;
	t->p1.x = x2; t->p1.y = y2; t->p1.z = z2;
	t->p2.x = x3; t->p2.y = y3; t->p2.z = z3;
	t->color = color;
	
	return 0;
}

static int threelib_update_triangle(lua_State* L) {
	Triangles* ts = pd->lua->getArgObject(1, "threelib.triangles", NULL);
	int idx = pd->lua->getArgInt(2) - 1;
	if (idx < 0 || ts->count <= idx) return 0;

	float x1 = pd->lua->getArgFloat(3);
	float y1 = pd->lua->getArgFloat(4);
	float z1 = pd->lua->getArgFloat(5);
	float x2 = pd->lua->getArgFloat(6);
	float y2 = pd->lua->getArgFloat(7);
	float z2 = pd->lua->getArgFloat(8);
	float x3 = pd->lua->getArgFloat(9);
	float y3 = pd->lua->getArgFloat(10);
	float z3 = pd->lua->getArgFloat(11);
	int color = pd->lua->getArgInt(12);
	
	Triangle* t = &ts->triangles[idx];
	t->p0.x = x1; t->p0.y = y1; t->p0.z = z1;
	t->p1.x = x2; t->p1.y = y2; t->p1.z = z2;
	t->p2.x = x3; t->p2.y = y3; t->p2.z = z3;
	t->color = color;
	return 0;
}

static int threelib_remove_triangle(lua_State* L) {
	Triangles* ts = pd->lua->getArgObject(1, "threelib.triangles", NULL);
	int idx = pd->lua->getArgInt(2) - 1;
	if (idx < 0 || ts->count <= idx) return 0;
	
	for (int i = idx + 1; i < ts->count; i++) {
		ts->triangles[i-1] = ts->triangles[i];
	}

	ts->count--;
	pd->system->realloc(ts->triangles, sizeof(Triangle) * ts->count);
	
	return 0;
}

void threelib_draw_t2(int8_t* fb, Int2* a, Int2* b, Int2* c, int8_t color) {
	// sort points by y
	if (b->y < a->y) {
		threelib_draw_t2(fb, b, a, c, color);
		return;
	}
	if (c->y < b->y) {
		threelib_draw_t2(fb, a, c, b, color);
		return;
	}
	
	//   A
	//  ....
	//  ......B
	// .....
	// C.

	if ((c->y < 0) || (LCD_ROWS <= a->y)) {
		return;
	}

	int ldx = (a->x <= c->x) ? c->x - a->x : a->x - c->x;
	int ldy = a->y - c->y; // points are already sorted
	int lsx = (a->x <= c->x) ? 1 : -1;
	int rdx = (a->x <= b->x) ? b->x - a->x : a->x - b->x;
	int rdy = a->y - b->y; // points are already sorted
	int rsx = (a->x <= b->x) ? 1 : -1;
	const int sy = 1;
	int lerr = ldx + ldy;
	int rerr = rdx + rdy;
	int lx, rx;
	lx = rx = a->x;
	int ly, ry;
	ly = ry = a->y;
	int loop = 0;
	int draw = 3; // bit flag l r
	while (1) {
		if (draw == 3) {
			if (ry == b->y) {
				// switch r from AB to BC
				rdx = (b->x <= c->x) ? c->x - b->x : b->x - c->x;
				rdy = b->y - c->y; // points are already sorted
				rsx = (b->x <= c->x) ? 1 : -1;
				rerr = rdx + rdy;
				rx = b->x;
				ry = b->y;
			}

			if ((0 <= ly) && (ly < LCD_ROWS)) {
				int yoff = ly * LCD_COLUMNS;
				// lx is not necessarily less than or equal to rx
				int lo, hi;
				if (lx < rx) {
					lo = lx;
					hi = rx;
				} else {
					lo = rx;
					hi = lx;
				}
				for (int x = lo; x <= hi; x++) {
					if (x < 0 || LCD_COLUMNS <= x) continue; // offscreen
					fb[x + yoff] = color;
				}
			}
			draw = 0;
		}

		if ((c->y <= ly) && (c->y <= ry)) {
			return;
		}
		if (loop++ > 1000) {
			pd->system->logToConsole("oops");
			return;
		}

		if ((draw & 2) == 0) {
			int lerr2 = 2 * lerr;
			if (lerr2 >= ldy) {
				lerr += ldy;
				lx += lsx;
			}
			if (lerr2 <= ldx) {
				lerr += ldx;
				ly += sy;
				draw |= 2;
			}
		}
		if ((draw & 1) == 0) {
			int rerr2 = 2 * rerr;
			if (rerr2 >= rdy) {
				rerr += rdy;
				rx += rsx;
			}
			if (rerr2 <= rdx) {
				rerr += rdx;
				ry += sy;
				draw |= 1;
			}
		}
	}
}

const int HALF_SCREEN_WIDTH = LCD_COLUMNS / 2;
const int HALF_SCREEN_HEIGHT = LCD_ROWS / 2;

void threelib_draw_fb(Triangles* ts) {
	int8_t* fb = ts->fb;

	// draw background - 0 black 127 white
	int fb_idx = 0;
	for (int y = 0; y < LCD_ROWS; y++) {
		for (int x = 0; x < LCD_COLUMNS; x++) {
			fb[fb_idx] = ts->bg;
			fb_idx++;
		}
	}

	// TODO: back face culling; viewport culling
	Int2* t2 = pd->system->realloc(NULL, sizeof(Int2) * ts->count * 3);
	float vp2screen = LCD_COLUMNS / ts->camera_width;
	for (int t_idx = 0; t_idx < ts->count; t_idx++) {
		int t2_idx = 3 * t_idx;
		// world space to viewport
		// TODO: apply camera rotation and translation
		float iz = 1.0f / ts->triangles[t_idx].p0.z;
		float x0 = (ts->triangles[t_idx].p0.x - HALF_SCREEN_WIDTH) * ts->camera_f * iz;
		float y0 = (ts->triangles[t_idx].p0.y - HALF_SCREEN_HEIGHT) * ts->camera_f * iz;
		iz = 1.0f / ts->triangles[t_idx].p1.z;
		float x1 = (ts->triangles[t_idx].p1.x - HALF_SCREEN_WIDTH) * ts->camera_f * iz;
		float y1 = (ts->triangles[t_idx].p1.y - HALF_SCREEN_HEIGHT) * ts->camera_f * iz;
		iz = 1.0f / ts->triangles[t_idx].p2.z;
		float x2 = (ts->triangles[t_idx].p2.x - HALF_SCREEN_WIDTH) * ts->camera_f * iz;
		float y2 = (ts->triangles[t_idx].p2.y - HALF_SCREEN_HEIGHT) * ts->camera_f * iz;
		// viewport to screen
		t2[t2_idx].x     = (int)(x0 * vp2screen) + HALF_SCREEN_WIDTH;
		t2[t2_idx].y     = (int)(y0 * vp2screen) + HALF_SCREEN_HEIGHT;
		t2[t2_idx + 1].x = (int)(x1 * vp2screen) + HALF_SCREEN_WIDTH;
		t2[t2_idx + 1].y = (int)(y1 * vp2screen) + HALF_SCREEN_HEIGHT;
		t2[t2_idx + 2].x = (int)(x2 * vp2screen) + HALF_SCREEN_WIDTH;
		t2[t2_idx + 2].y = (int)(y2 * vp2screen) + HALF_SCREEN_HEIGHT;
	}
	
	// draw triangles
	for (int t_idx = 0; t_idx < ts->count; t_idx++) {
		Triangle* t = ts->triangles + t_idx;
		int t2_idx = 3 * t_idx;
		threelib_draw_t2(fb, t2 + t2_idx, t2 + t2_idx + 1, t2 + t2_idx + 2, t->color);
	}
	
	pd->system->realloc(t2, 0); // free screen space triangles

	// dither - 0 black else white
	// Atkinson
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
	Triangles* ts = pd->lua->getArgObject(1, "threelib.triangles", NULL);
	threelib_draw_fb(ts);
	
	uint8_t* frame = pd->graphics->getFrame();
	int bitpos = 0x80; // 1000 0000
	int fb_idx = 0;
	for (int y = 0; y < LCD_ROWS; y++) {
		uint8_t* row = &frame[y * LCD_ROWSIZE];
		uint8_t chunk = 0x00; // black
		for (int x = 0; x < LCD_COLUMNS; x++) {
			if (ts->fb[fb_idx]) {
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

int read_vlq(SDFile* f) {
	char* buf = pd->system->realloc(NULL, 5);
	char* buf_ptr = buf;
	int n = 0;
	for (; n < 5; n++) {
		if (pd->file->read(f, buf_ptr, 1) != 1) {
			pd->system->realloc(buf, 0);
			return -1;
		}
		pd->system->logToConsole("%d %x", n, *buf_ptr);
		if ((*buf_ptr & 0x80) == 0) {
			break;
		}
		buf_ptr++;
	}
	int result = 0;
	for (; n >= 0; n--) {
		result <<= 7;
		pd->system->logToConsole("%d %x", n, *buf_ptr);
		result |= *buf_ptr;
		buf_ptr--;
	}

	pd->system->realloc(buf, 0);
	return result;
}

static int threelib_load_model(lua_State* L) {
	Triangles* ts = pd->lua->getArgObject(1, "threelib.triangles", NULL);
	const char* filename = pd->lua->getArgString(2);

	SDFile* f = pd->file->open(filename, kFileRead);
	if (f == NULL) {
		const char* ferr = pd->file->geterr();
		pd->system->logToConsole("failed to open %s: %s", filename, ferr);
		goto cleanup;
	}

	char got_magic[5];
	if (pd->file->read(f, &got_magic, 4) != 4) {
		pd->system->logToConsole("failed to read magic bytes of %s", filename);
		goto cleanup;
	}
	got_magic[4] = 0;
	const char* want_magic = "KW3D";
	if (strncmp(got_magic, want_magic, 4) != 0) {
		pd->system->logToConsole("magic bytes of %s did not match expected value: %s", filename, got_magic);
		goto cleanup;
	}

	char version[2];
	if (pd->file->read(f, &version, 2) != 2) {
		pd->system->logToConsole("failed to read version of %s", filename);
		goto cleanup;
	}

	int num_materials = read_vlq(f);
	if (num_materials == -1) {
		const char* ferr = pd->file->geterr();
		pd->system->logToConsole("failed to read vlq num_materials in %s: %s", filename, ferr);
		goto cleanup;
	}
	pd->system->logToConsole("%d materials", num_materials);
	if (pd->file->seek(f, 2*num_materials, SEEK_CUR) != 0) {
		pd->system->logToConsole("failed to seek past materials of %s", filename);
		goto cleanup;
	}

	int num_meshes = read_vlq(f);
	if (num_meshes == -1) {
		const char* ferr = pd->file->geterr();
		pd->system->logToConsole("failed to read vlq num_meshes in %s: %s", filename, ferr);
		goto cleanup;
	}
	pd->system->logToConsole("%d meshes", num_meshes);

	for (int mesh_idx = 0; mesh_idx < num_meshes; mesh_idx++) {
		int material_idx = read_vlq(f);
		if (material_idx == -1) {
			const char* ferr = pd->file->geterr();
			pd->system->logToConsole("failed to read vlq material_idx in %s: %s", filename, ferr);
			goto cleanup;
		}
		pd->system->logToConsole("material %d", material_idx);

		int num_vertices = read_vlq(f);
		if (num_vertices == -1) {
			const char* ferr = pd->file->geterr();
			pd->system->logToConsole("failed to read vlq num_vertices in %s: %s", filename, ferr);
			goto cleanup;
		}
		pd->system->logToConsole("%d vertices", num_vertices);

		float* vertices = pd->system->realloc(NULL, 3 * num_vertices * 4); // xyz f32
		if (pd->file->read(f, vertices, 3 * num_vertices * 4) == -1) {
			const char* ferr = pd->file->geterr();
			pd->system->logToConsole("failed to read vertices in %s: %s", filename, ferr);
			pd->system->realloc(vertices, 0);
			goto cleanup;
		}
		for (int v = 0; v < 3 * num_vertices; v += 3) {
			float x = *(vertices + v);
			float y = *(vertices + v + 1);
			float z = *(vertices + v + 2);
			pd->system->logToConsole("vertex %f %f %f", x, y ,z);
		}
		pd->system->realloc(vertices, 0);

		int num_normals = read_vlq(f);
		if (num_normals == -1) {
			const char* ferr = pd->file->geterr();
			pd->system->logToConsole("failed to read vlq num_normals in %s: %s", filename, ferr);
			goto cleanup;
		}
		pd->system->logToConsole("%d normals", num_normals);

		float* normals = pd->system->realloc(NULL, 3 * num_normals * 4); // xyz f32
		if (pd->file->read(f, normals, 3 * num_normals * 4) == -1) {
			const char* ferr = pd->file->geterr();
			pd->system->logToConsole("failed to read normals in %s: %s", filename, ferr);
			pd->system->realloc(normals, 0);
			goto cleanup;
		}
		for (int v = 0; v < 3 * num_vertices; v += 3) {
			float x = *(normals + v);
			float y = *(normals + v + 1);
			float z = *(normals + v + 2);
			pd->system->logToConsole("normal %f %f %f", x, y ,z);
		}
		pd->system->realloc(normals, 0);

		int num_triangles = read_vlq(f);
		if (num_triangles == -1) {
			const char* ferr = pd->file->geterr();
			pd->system->logToConsole("failed to read vlq num_triangles in %s: %s", filename, ferr);
			goto cleanup;
		}
		pd->system->logToConsole("%d triangles", num_triangles);
	}

cleanup:
	if (f != NULL) {
		if (pd->file->close(f) == -1) {
			const char* cerr = pd->file->geterr();
			pd->system->logToConsole("failed to close %s: %s", filename, cerr);
		}
	}
	return 0;
}
