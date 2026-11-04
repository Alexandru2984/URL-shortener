#include "qr.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define QR_MAX_VERSION 6
#define QR_MAX_SIZE (17 + 4 * QR_MAX_VERSION)
#define QR_MAX_CODEWORDS 172

struct qr_version_spec {
    int size;
    int total_codewords;
    int data_codewords;
    int ec_codewords_per_block;
    int num_blocks;
    int align_center;
};

static const struct qr_version_spec k_qr_specs[QR_MAX_VERSION] = {
    { 21,  26,  19,  7, 1,  0 },
    { 25,  44,  34, 10, 1, 18 },
    { 29,  70,  55, 15, 1, 22 },
    { 33, 100,  80, 20, 1, 26 },
    { 37, 134, 108, 26, 1, 30 },
    { 41, 172, 136, 18, 2, 34 }
};

static uint8_t gf256_mul(uint8_t x, uint8_t y) {
    uint8_t z = 0;
    for (int i = 7; i >= 0; i--) {
        z = (uint8_t)((z << 1U) ^ ((z >> 7U) * 0x11DU));
        z ^= (uint8_t)(((y >> (unsigned int)i) & 1U) * x);
    }
    return z;
}

static void rs_compute_remainder(const uint8_t *data, int data_len,
                                 int degree, uint8_t *result) {
    uint8_t gen[32];
    memset(gen, 0, sizeof(gen));
    gen[degree - 1] = 1U;

    uint8_t root = 1U;
    for (int i = 0; i < degree; i++) {
        for (int j = 0; j < degree; j++) {
            gen[j] = gf256_mul(gen[j], root);
            if (j + 1 < degree) {
                gen[j] ^= gen[j + 1];
            }
        }
        root = gf256_mul(root, 0x02U);
    }

    memset(result, 0, (size_t)degree);
    for (int i = 0; i < data_len; i++) {
        uint8_t factor = data[i] ^ result[0];
        memmove(result, result + 1, (size_t)(degree - 1));
        result[degree - 1] = 0;
        for (int j = 0; j < degree; j++) {
            result[j] ^= gf256_mul(gen[j], factor);
        }
    }
}

static void append_bits(uint8_t *buf, int *bit_len, unsigned int val, int num_bits) {
    for (int i = num_bits - 1; i >= 0; i--) {
        if ((val >> (unsigned int)i) & 1U) {
            buf[*bit_len >> 3] |= (uint8_t)(1U << (7 - (*bit_len & 7)));
        }
        (*bit_len)++;
    }
}

static void set_function_module(uint8_t modules[QR_MAX_SIZE][QR_MAX_SIZE],
                                uint8_t is_func[QR_MAX_SIZE][QR_MAX_SIZE],
                                int x, int y, int is_dark) {
    modules[y][x] = (uint8_t)(is_dark ? 1U : 0U);
    is_func[y][x] = 1U;
}

static void draw_finder_pattern(uint8_t modules[QR_MAX_SIZE][QR_MAX_SIZE],
                                uint8_t is_func[QR_MAX_SIZE][QR_MAX_SIZE],
                                int size, int cx, int cy) {
    for (int dy = -4; dy <= 4; dy++) {
        for (int dx = -4; dx <= 4; dx++) {
            int xx = cx + dx;
            int yy = cy + dy;
            if (xx < 0 || xx >= size || yy < 0 || yy >= size) continue;
            int dist = abs(dx) > abs(dy) ? abs(dx) : abs(dy);
            set_function_module(modules, is_func, xx, yy, (dist != 2 && dist != 4));
        }
    }
}

static void draw_alignment_pattern(uint8_t modules[QR_MAX_SIZE][QR_MAX_SIZE],
                                   uint8_t is_func[QR_MAX_SIZE][QR_MAX_SIZE],
                                   int cx, int cy) {
    for (int dy = -2; dy <= 2; dy++) {
        for (int dx = -2; dx <= 2; dx++) {
            int dist = abs(dx) > abs(dy) ? abs(dx) : abs(dy);
            set_function_module(modules, is_func, cx + dx, cy + dy, dist != 1);
        }
    }
}

static void draw_format_bits(uint8_t modules[QR_MAX_SIZE][QR_MAX_SIZE],
                             uint8_t is_func[QR_MAX_SIZE][QR_MAX_SIZE],
                             int size, int mask) {
    // ECC Level L indicator is 01 (binary)
    int data = (1 << 3) | mask;
    int rem = data;
    for (int i = 0; i < 10; i++) {
        rem = (rem << 1) ^ ((rem >> 9) * 0x537);
    }
    int bits = ((data << 10) | rem) ^ 0x5412;

    for (int i = 0; i <= 5; i++) {
        set_function_module(modules, is_func, 8, i, (bits >> i) & 1);
    }
    set_function_module(modules, is_func, 8, 7, (bits >> 6) & 1);
    set_function_module(modules, is_func, 8, 8, (bits >> 7) & 1);
    set_function_module(modules, is_func, 7, 8, (bits >> 8) & 1);
    for (int i = 9; i < 15; i++) {
        set_function_module(modules, is_func, 14 - i, 8, (bits >> i) & 1);
    }

    for (int i = 0; i < 8; i++) {
        set_function_module(modules, is_func, size - 1 - i, 8, (bits >> i) & 1);
    }
    for (int i = 8; i < 15; i++) {
        set_function_module(modules, is_func, 8, size - 15 + i, (bits >> i) & 1);
    }
    set_function_module(modules, is_func, 8, size - 8, 1);
}

static int mask_bit(int mask, int x, int y) {
    switch (mask) {
        case 0: return (x + y) % 2 == 0;
        case 1: return y % 2 == 0;
        case 2: return x % 3 == 0;
        case 3: return (x + y) % 3 == 0;
        case 4: return (y / 2 + x / 3) % 2 == 0;
        case 5: return (x * y) % 2 + (x * y) % 3 == 0;
        case 6: return ((x * y) % 2 + (x * y) % 3) % 2 == 0;
        default: return ((x + y) % 2 + (x * y) % 3) % 2 == 0;
    }
}

static int compute_penalty(const uint8_t modules[QR_MAX_SIZE][QR_MAX_SIZE], int size) {
    int penalty = 0;

    // Adjacent modules in row/col having same color
    for (int y = 0; y < size; y++) {
        int run_color = modules[y][0];
        int run_len = 1;
        for (int x = 1; x < size; x++) {
            if (modules[y][x] == run_color) {
                run_len++;
                if (run_len == 5) penalty += 3;
                else if (run_len > 5) penalty += 1;
            } else {
                run_color = modules[y][x];
                run_len = 1;
            }
        }
    }
    for (int x = 0; x < size; x++) {
        int run_color = modules[0][x];
        int run_len = 1;
        for (int y = 1; y < size; y++) {
            if (modules[y][x] == run_color) {
                run_len++;
                if (run_len == 5) penalty += 3;
                else if (run_len > 5) penalty += 1;
            } else {
                run_color = modules[y][x];
                run_len = 1;
            }
        }
    }

    // 2x2 blocks of same color
    for (int y = 0; y < size - 1; y++) {
        for (int x = 0; x < size - 1; x++) {
            uint8_t c = modules[y][x];
            if (c == modules[y][x + 1] && c == modules[y + 1][x] && c == modules[y + 1][x + 1]) {
                penalty += 3;
            }
        }
    }
    return penalty;
}

static int is_valid_hex_color(const char *color) {
    if (!color || color[0] != '#' || strlen(color) != 7U) return 0;
    for (size_t i = 1U; i < 7U; i++) {
        char c = color[i];
        int is_hex = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
        if (!is_hex) return 0;
    }
    return 1;
}

char *qr_generate_svg_themed(const char *text, int border_modules,
                             const char *fg_hex, const char *bg_hex) {
    if (!text || !text[0]) return NULL;
    if (border_modules < 1 || border_modules > 16) border_modules = 4;
    const char *safe_fg = is_valid_hex_color(fg_hex) ? fg_hex : "#0f172a";
    const char *safe_bg = is_valid_hex_color(bg_hex) ? bg_hex : "#ffffff";

    size_t text_len = strlen(text);
    const struct qr_version_spec *spec = NULL;
    for (int i = 0; i < QR_MAX_VERSION; i++) {
        // Byte mode overhead: 4 bits mode + 8 bits length = 12 bits (2 bytes rounded)
        size_t max_bytes = (size_t)(k_qr_specs[i].data_codewords - 2);
        if (text_len <= max_bytes) {
            spec = &k_qr_specs[i];
            break;
        }
    }
    if (!spec) return NULL;

    uint8_t data_codewords[QR_MAX_CODEWORDS];
    memset(data_codewords, 0, sizeof(data_codewords));
    int bit_len = 0;

    // Mode 0100 (Byte), 8-bit char count, then raw bytes
    append_bits(data_codewords, &bit_len, 0x4U, 4);
    append_bits(data_codewords, &bit_len, (unsigned int)text_len, 8);
    for (size_t i = 0; i < text_len; i++) {
        append_bits(data_codewords, &bit_len, (unsigned char)text[i], 8);
    }

    int capacity_bits = spec->data_codewords * 8;
    int terminator = capacity_bits - bit_len;
    if (terminator > 4) terminator = 4;
    append_bits(data_codewords, &bit_len, 0U, terminator);
    if (bit_len % 8 != 0) {
        append_bits(data_codewords, &bit_len, 0U, 8 - (bit_len % 8));
    }

    uint8_t pad_byte = 0xECU;
    while (bit_len < capacity_bits) {
        append_bits(data_codewords, &bit_len, pad_byte, 8);
        pad_byte = (pad_byte == 0xECU) ? 0x11U : 0xECU;
    }

    uint8_t all_codewords[QR_MAX_CODEWORDS];
    memset(all_codewords, 0, sizeof(all_codewords));
    if (spec->num_blocks == 1) {
        memcpy(all_codewords, data_codewords, (size_t)spec->data_codewords);
        rs_compute_remainder(data_codewords, spec->data_codewords,
                             spec->ec_codewords_per_block,
                             all_codewords + spec->data_codewords);
    } else {
        int block_data_len = spec->data_codewords / 2;
        uint8_t ec0[32];
        uint8_t ec1[32];
        rs_compute_remainder(data_codewords, block_data_len, spec->ec_codewords_per_block, ec0);
        rs_compute_remainder(data_codewords + block_data_len, block_data_len,
                             spec->ec_codewords_per_block, ec1);
        int idx = 0;
        for (int i = 0; i < block_data_len; i++) {
            all_codewords[idx++] = data_codewords[i];
            all_codewords[idx++] = data_codewords[block_data_len + i];
        }
        for (int i = 0; i < spec->ec_codewords_per_block; i++) {
            all_codewords[idx++] = ec0[i];
            all_codewords[idx++] = ec1[i];
        }
    }

    uint8_t base_modules[QR_MAX_SIZE][QR_MAX_SIZE];
    uint8_t is_func[QR_MAX_SIZE][QR_MAX_SIZE];
    memset(base_modules, 0, sizeof(base_modules));
    memset(is_func, 0, sizeof(is_func));

    int size = spec->size;
    for (int i = 0; i < size; i++) {
        set_function_module(base_modules, is_func, 6, i, i % 2 == 0);
        set_function_module(base_modules, is_func, i, 6, i % 2 == 0);
    }

    draw_finder_pattern(base_modules, is_func, size, 3, 3);
    draw_finder_pattern(base_modules, is_func, size, size - 4, 3);
    draw_finder_pattern(base_modules, is_func, size, 3, size - 4);

    if (spec->align_center > 0) {
        draw_alignment_pattern(base_modules, is_func, spec->align_center, spec->align_center);
    }
    draw_format_bits(base_modules, is_func, size, 0);

    // Place codewords in zig-zag pattern
    int bit_idx = 0;
    int total_bits = spec->total_codewords * 8;
    for (int right = size - 1; right >= 1; right -= 2) {
        if (right == 6) right = 5;
        for (int vert = 0; vert < size; vert++) {
            for (int j = 0; j < 2; j++) {
                int x = right - j;
                int upward = ((right + 1) & 2) == 0;
                int y = upward ? (size - 1 - vert) : vert;
                if (!is_func[y][x] && bit_idx < total_bits) {
                    int bit = (all_codewords[bit_idx >> 3] >> (7 - (bit_idx & 7))) & 1;
                    base_modules[y][x] = (uint8_t)bit;
                    bit_idx++;
                }
            }
        }
    }

    // Choose best mask out of 0..7
    int best_penalty = 0x7fffffff;
    uint8_t best_modules[QR_MAX_SIZE][QR_MAX_SIZE];
    memset(best_modules, 0, sizeof(best_modules));

    for (int mask = 0; mask < 8; mask++) {
        uint8_t candidate[QR_MAX_SIZE][QR_MAX_SIZE];
        memcpy(candidate, base_modules, sizeof(candidate));
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                if (!is_func[y][x] && mask_bit(mask, x, y)) {
                    candidate[y][x] ^= 1U;
                }
            }
        }
        draw_format_bits(candidate, is_func, size, mask);
        int penalty = compute_penalty(candidate, size);
        if (penalty < best_penalty) {
            best_penalty = penalty;
            memcpy(best_modules, candidate, sizeof(best_modules));
        }
    }

    // Render SVG string
    int view_size = size + border_modules * 2;
    size_t max_svg_len = 512U + (size_t)(size * size * 20);
    char *svg = malloc(max_svg_len);
    if (!svg) return NULL;

    int offset = snprintf(
        svg, max_svg_len,
        "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 %d %d\" "
        "shape-rendering=\"crispEdges\" role=\"img\" aria-label=\"QR Code\">"
        "<rect width=\"100%%\" height=\"100%%\" fill=\"%s\"/>"
        "<path fill=\"%s\" d=\"",
        view_size, view_size, safe_bg, safe_fg);
    if (offset < 0 || (size_t)offset >= max_svg_len) {
        free(svg);
        return NULL;
    }

    size_t pos = (size_t)offset;
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            if (!best_modules[y][x]) continue;
            int written = snprintf(svg + pos, max_svg_len - pos,
                                   "M%d,%dh1v1h-1z", x + border_modules, y + border_modules);
            if (written < 0 || pos + (size_t)written + 32U >= max_svg_len) {
                free(svg);
                return NULL;
            }
            pos += (size_t)written;
        }
    }

    int tail = snprintf(svg + pos, max_svg_len - pos, "\"/></svg>");
    if (tail < 0 || pos + (size_t)tail >= max_svg_len) {
        free(svg);
        return NULL;
    }
    return svg;
}

char *qr_generate_svg(const char *text, int border_modules) {
    return qr_generate_svg_themed(text, border_modules, "#0f172a", "#ffffff");
}
