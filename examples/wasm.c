#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <js/glue.h>
#include <js/dom_pk_codes.h>

#include "../agnes.h"

#define RMASK 0x000000ff
#define GMASK 0x0000ff00
#define BMASK 0x00ff0000
#define AMASK 0xff000000

static void get_input(const uint8_t *state, agnes_input_t *out_input);
static void* read_file(const char *filename, size_t *out_len);

static uint32_t pixels[AGNES_SCREEN_WIDTH * AGNES_SCREEN_HEIGHT];

uint8_t keyboard_state[UINT16_MAX] = {0};

static bool onkey(void *userdata, bool pressed, int key, int code, int modifiers) {
    (void)userdata, (void)key, (void)modifiers;
    keyboard_state[code] = pressed;
    if (code == DOM_PK_F12) {
        return 0;
    }
    return 1;
}

static void* read_filepicker_file(char **filename, size_t *len, const char* ext) {
    JS_setFont("bold 20px Roboto");
    JS_fillStyle("white");

    char buf[UINT8_MAX];
    snprintf(buf, sizeof(buf), "Click to browse... (%s)", ext ? ext : ".*");
    JS_fillText(buf, (AGNES_SCREEN_WIDTH - JS_measureTextWidth(buf)) / 2, AGNES_SCREEN_HEIGHT / 2);

    uint8_t *file = JS_openFilePicker(filename, len, ext);
    return file;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s game.nes\n", argv[0]);
        // return 1;
    }

    JS_createCanvas(AGNES_SCREEN_WIDTH, AGNES_SCREEN_HEIGHT, "2d");
    JS_setTitle("agnes");
    JS_addKeyEventListener(NULL, onkey);

    char *ines_name = argc >= 2 ? argv[1] : NULL;

    size_t ines_data_size = 0;
    void* ines_data = argc >= 2 ? read_file(ines_name, &ines_data_size) :
                                read_filepicker_file(&ines_name, &ines_data_size, ".nes");
    if (ines_data == NULL) {
        fprintf(stderr, "Reading %s failed.\n", ines_name);
        return 1;
    }

    agnes_t *agnes = agnes_make();
    if (agnes == NULL) {
        fprintf(stderr, "Making agnes failed.\n");
        return 1;
    }

    bool ok = agnes_load_ines_data(agnes, ines_data, ines_data_size);
    if (!ok) {
        fprintf(stderr, "Loading %s failed.\n", ines_name);
        return 1;
    }

    agnes_input_t input;

    while (true) {
        if (keyboard_state[DOM_PK_ESCAPE]) {
            break;
        }

        get_input(keyboard_state, &input);

        agnes_set_input(agnes, &input, NULL);

        ok = agnes_next_frame(agnes);
        if (!ok) {
            fprintf(stderr, "Getting next frame failed.\n");
            return 1;
        }

        for (int y = 0; y < AGNES_SCREEN_HEIGHT; y++) {
            for (int x = 0; x < AGNES_SCREEN_WIDTH; x++) {
                agnes_color_t c = agnes_get_screen_pixel(agnes, x, y);
                int ix = (y * AGNES_SCREEN_WIDTH) + x;
                uint32_t c_val = c.a << 24 | c.b << 16 | c.g << 8 | c.r;
                pixels[ix] = c_val;
            }
        }

        JS_setPixelsAlpha(pixels);
        JS_requestAnimationFrame();
    }

    agnes_destroy(agnes);
    return 0;
}

static void get_input(const uint8_t *state, agnes_input_t *out_input) {
    memset(out_input, 0, sizeof(agnes_input_t));

    if (state[DOM_PK_Z])           out_input->a = true;
    if (state[DOM_PK_X])           out_input->b = true;
    if (state[DOM_PK_ARROW_LEFT])  out_input->left = true;
    if (state[DOM_PK_ARROW_RIGHT]) out_input->right = true;
    if (state[DOM_PK_ARROW_UP])    out_input->up = true;
    if (state[DOM_PK_ARROW_DOWN])  out_input->down = true;
    if (state[DOM_PK_SHIFT_RIGHT]) out_input->select = true;
    if (state[DOM_PK_ENTER])       out_input->start = true;
}

static void* read_file(const char *filename, size_t *out_len) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        return NULL;
    }
    fseek(fp, 0L, SEEK_END);
    long pos = ftell(fp);
    if (pos < 0) {
        fclose(fp);
        return NULL;
    }
    size_t file_size = pos;
    rewind(fp);
    unsigned char *file_contents = (unsigned char *)malloc(file_size);
    if (!file_contents) {
        fclose(fp);
        return NULL;
    }
    if (fread(file_contents, file_size, 1, fp) < 1) {
        if (ferror(fp)) {
            fclose(fp);
            free(file_contents);
            return NULL;
        }
    }
    fclose(fp);
    *out_len = file_size;
    return file_contents;
}
