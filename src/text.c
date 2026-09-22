#include <string.h>

#include "../include/text.h"
#include "../include/framebuffer.h"

// compact 3x5 bitmap font. Each glyph is 5 bytes (one per row), each byte's
// low 3 bits are the pixel columns for that row (bit2=left, bit0=right).
// Small and legible enough for a round 240x240 display at scale>=2;
// covers A-Z, 0-9, space, and a handful of punctuation. Lowercase letters
// are folded to uppercase. Unsupported characters render as blank space.
#define GLYPH_W 3
#define GLYPH_H 5

static const uint8_t *get_glyph(char c)
{
    static const uint8_t G_SPACE[5] = {0, 0, 0, 0, 0};
    static const uint8_t G_A[5] = {2, 5, 7, 5, 5};
    static const uint8_t G_B[5] = {6, 5, 6, 5, 6};
    static const uint8_t G_C[5] = {3, 4, 4, 4, 3};
    static const uint8_t G_D[5] = {6, 5, 5, 5, 6};
    static const uint8_t G_E[5] = {7, 4, 6, 4, 7};
    static const uint8_t G_F[5] = {7, 4, 6, 4, 4};
    static const uint8_t G_G[5] = {3, 4, 5, 5, 3};
    static const uint8_t G_H[5] = {5, 5, 7, 5, 5};
    static const uint8_t G_I[5] = {7, 2, 2, 2, 7};
    static const uint8_t G_J[5] = {1, 1, 1, 5, 2};
    static const uint8_t G_K[5] = {5, 5, 6, 5, 5};
    static const uint8_t G_L[5] = {4, 4, 4, 4, 7};
    static const uint8_t G_M[5] = {5, 7, 5, 5, 5};
    static const uint8_t G_N[5] = {5, 6, 5, 5, 5};
    static const uint8_t G_O[5] = {2, 5, 5, 5, 2};
    static const uint8_t G_P[5] = {6, 5, 6, 4, 4};
    static const uint8_t G_Q[5] = {2, 5, 5, 2, 1};
    static const uint8_t G_R[5] = {6, 5, 6, 5, 5};
    static const uint8_t G_S[5] = {3, 4, 2, 1, 6};
    static const uint8_t G_T[5] = {7, 2, 2, 2, 2};
    static const uint8_t G_U[5] = {5, 5, 5, 5, 2};
    static const uint8_t G_V[5] = {5, 5, 5, 2, 2};
    static const uint8_t G_W[5] = {5, 5, 5, 7, 5};
    static const uint8_t G_X[5] = {5, 5, 2, 5, 5};
    static const uint8_t G_Y[5] = {5, 5, 2, 2, 2};
    static const uint8_t G_Z[5] = {7, 1, 2, 4, 7};
    static const uint8_t G_0[5] = {2, 5, 5, 5, 2};
    static const uint8_t G_1[5] = {2, 6, 2, 2, 7};
    static const uint8_t G_2[5] = {6, 1, 2, 4, 7};
    static const uint8_t G_3[5] = {6, 1, 2, 1, 6};
    static const uint8_t G_4[5] = {5, 5, 7, 1, 1};
    static const uint8_t G_5[5] = {7, 4, 6, 1, 6};
    static const uint8_t G_6[5] = {3, 4, 6, 5, 2};
    static const uint8_t G_7[5] = {7, 1, 2, 4, 4};
    static const uint8_t G_8[5] = {2, 5, 2, 5, 2};
    static const uint8_t G_9[5] = {2, 5, 3, 1, 2};
    static const uint8_t G_DOT[5]    = {0, 0, 0, 0, 2};
    static const uint8_t G_COMMA[5]  = {0, 0, 0, 2, 4};
    static const uint8_t G_COLON[5]  = {0, 2, 0, 2, 0};
    static const uint8_t G_DASH[5]   = {0, 0, 7, 0, 0};
    static const uint8_t G_BANG[5]   = {2, 2, 2, 0, 2};
    static const uint8_t G_QMARK[5]  = {6, 1, 2, 0, 2};
    static const uint8_t G_SLASH[5]  = {1, 1, 2, 4, 4};

    if (c >= 'a' && c <= 'z') c -= 32; // fold lowercase to uppercase

    switch (c) {
        case ' ': return G_SPACE;
        case 'A': return G_A; case 'B': return G_B; case 'C': return G_C;
        case 'D': return G_D; case 'E': return G_E; case 'F': return G_F;
        case 'G': return G_G; case 'H': return G_H; case 'I': return G_I;
        case 'J': return G_J; case 'K': return G_K; case 'L': return G_L;
        case 'M': return G_M; case 'N': return G_N; case 'O': return G_O;
        case 'P': return G_P; case 'Q': return G_Q; case 'R': return G_R;
        case 'S': return G_S; case 'T': return G_T; case 'U': return G_U;
        case 'V': return G_V; case 'W': return G_W; case 'X': return G_X;
        case 'Y': return G_Y; case 'Z': return G_Z;
        case '0': return G_0; case '1': return G_1; case '2': return G_2;
        case '3': return G_3; case '4': return G_4; case '5': return G_5;
        case '6': return G_6; case '7': return G_7; case '8': return G_8;
        case '9': return G_9;
        case '.': return G_DOT;
        case ',': return G_COMMA;
        case ':': return G_COLON;
        case '-': return G_DASH;
        case '!': return G_BANG;
        case '?': return G_QMARK;
        case '/': return G_SLASH;
        default:  return G_SPACE; // unsupported chars render blank
    }
}

int draw_char(int x, int y, char c, uint16_t color, int scale)
{
    const uint8_t *glyph = get_glyph(c);

    for (int row = 0; row < GLYPH_H; row++) {
        uint8_t bits = glyph[row];
        for (int col = 0; col < GLYPH_W; col++) {
            if (bits & (1 << (GLYPH_W - 1 - col))) {
                for (int sy = 0; sy < scale; sy++) {
                    for (int sx = 0; sx < scale; sx++) {
                        fb_set(
                            x + col * scale + sx,
                            y + row * scale + sy,
                            color
                        );
                    }
                }
            }
        }
    }

    return GLYPH_W * scale; // caller adds char-spacing on top of this
}

#define CHAR_SPACING 1 // extra px between glyph cells, before scaling
#define LINE_SPACING 2 // extra px between lines, before scaling

void draw_text_box(int x, int y, int w, int h, const char *text, uint16_t color, int scale)
{
    int glyph_w = GLYPH_W * scale;
    int glyph_h = GLYPH_H * scale;
    int cell_w = glyph_w + CHAR_SPACING * scale;
    int line_h = glyph_h + LINE_SPACING * scale;

    int cursor_x = 0;
    int cursor_y = 0;

    size_t len = strlen(text);
    size_t i = 0;

    while (i < len) {
        // measure the next word (run of non-space chars)
        size_t word_start = i;
        while (i < len && text[i] != ' ' && text[i] != '\n') i++;
        size_t word_len = i - word_start;
        int word_px = (int)word_len * cell_w;

        // wrap to next line if the word won't fit, unless we're already
        // at the start of a line (in which case just let it overflow/clip)
        if (cursor_x > 0 && cursor_x + word_px > w) {
            cursor_x = 0;
            cursor_y += line_h;
        }

        // stop entirely once we've run out of vertical room in the box
        if (cursor_y + glyph_h > h) {
            return;
        }

        for (size_t j = word_start; j < i; j++) {
            if (cursor_x + glyph_w > w) {
                cursor_x = 0;
                cursor_y += line_h;
                if (cursor_y + glyph_h > h) {
                    return;
                }
            }
            draw_char(x + cursor_x, y + cursor_y, text[j], color, scale);
            cursor_x += cell_w;
        }

        if (i < len && text[i] == '\n') {
            cursor_x = 0;
            cursor_y += line_h;
            i++;
            if (cursor_y + glyph_h > h) {
                return;
            }
            continue;
        }

        // consume the space that ended the word, add a space-width gap
        if (i < len && text[i] == ' ') {
            cursor_x += cell_w;
            i++;
        }
    }
}