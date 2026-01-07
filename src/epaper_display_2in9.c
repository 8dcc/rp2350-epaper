/*
 * Copyright 2026 8dcc
 *
 * This file is part of rp2350-epaper.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "epaper_display_2in9.h"

#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"

#include "epaper_display_utils.h"

/*
 * Display dimensions.
 *
 * TODO: Avoid using compile-time macros, use 'width' and 'height' members of
 * context.
 */
#define EPD_WIDTH  128
#define EPD_HEIGHT 296

/* Display commands */
#define EPD_CMD_DRIVER_OUTPUT_CONTROL       0x01
#define EPD_CMD_BOOSTER_SOFT_START_CONTROL  0x0C
#define EPD_CMD_DEEP_SLEEP_MODE             0x10
#define EPD_CMD_DATA_ENTRY_MODE_SETTING     0x11
#define EPD_CMD_SW_RESET                    0x12
#define EPD_CMD_MASTER_ACTIVATION           0x20
#define EPD_CMD_DISPLAY_UPDATE_CONTROL_1    0x21
#define EPD_CMD_DISPLAY_UPDATE_CONTROL_2    0x22
#define EPD_CMD_WRITE_RAM                   0x24
#define EPD_CMD_WRITE_VCOM_REGISTER         0x2C
#define EPD_CMD_WRITE_LUT_REGISTER          0x32
#define EPD_CMD_SET_DUMMY_LINE_PERIOD       0x3A
#define EPD_CMD_SET_GATE_TIME               0x3B
#define EPD_CMD_SET_RAM_X_ADDRESS_START_END 0x44
#define EPD_CMD_SET_RAM_Y_ADDRESS_START_END 0x45
#define EPD_CMD_SET_RAM_X_ADDRESS_COUNTER   0x4E
#define EPD_CMD_SET_RAM_Y_ADDRESS_COUNTER   0x4F

/*----------------------------------------------------------------------------*/

/* Framebuffer */
static uint8_t framebuffer[EPD_WIDTH * EPD_HEIGHT / 8];

/* Simple 5x7 font */
static const uint8_t font5x7[][5] = {
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, /* space */
    { 0x00, 0x00, 0x5F, 0x00, 0x00 }, /* ! */
    { 0x00, 0x07, 0x00, 0x07, 0x00 }, /* " */
    { 0x14, 0x7F, 0x14, 0x7F, 0x14 }, /* # */
    { 0x24, 0x2A, 0x7F, 0x2A, 0x12 }, /* $ */
    { 0x23, 0x13, 0x08, 0x64, 0x62 }, /* % */
    { 0x36, 0x49, 0x55, 0x22, 0x50 }, /* & */
    { 0x00, 0x05, 0x03, 0x00, 0x00 }, /* ' */
    { 0x00, 0x1C, 0x22, 0x41, 0x00 }, /* ( */
    { 0x00, 0x41, 0x22, 0x1C, 0x00 }, /* ) */
    { 0x14, 0x08, 0x3E, 0x08, 0x14 }, /* * */
    { 0x08, 0x08, 0x3E, 0x08, 0x08 }, /* + */
    { 0x00, 0x50, 0x30, 0x00, 0x00 }, /* , */
    { 0x08, 0x08, 0x08, 0x08, 0x08 }, /* - */
    { 0x00, 0x60, 0x60, 0x00, 0x00 }, /* . */
    { 0x20, 0x10, 0x08, 0x04, 0x02 }, /* / */
    { 0x3E, 0x51, 0x49, 0x45, 0x3E }, /* 0 */
    { 0x00, 0x42, 0x7F, 0x40, 0x00 }, /* 1 */
    { 0x42, 0x61, 0x51, 0x49, 0x46 }, /* 2 */
    { 0x21, 0x41, 0x45, 0x4B, 0x31 }, /* 3 */
    { 0x18, 0x14, 0x12, 0x7F, 0x10 }, /* 4 */
    { 0x27, 0x45, 0x45, 0x45, 0x39 }, /* 5 */
    { 0x3C, 0x4A, 0x49, 0x49, 0x30 }, /* 6 */
    { 0x01, 0x71, 0x09, 0x05, 0x03 }, /* 7 */
    { 0x36, 0x49, 0x49, 0x49, 0x36 }, /* 8 */
    { 0x06, 0x49, 0x49, 0x29, 0x1E }, /* 9 */
    { 0x00, 0x36, 0x36, 0x00, 0x00 }, /* : */
    { 0x00, 0x56, 0x36, 0x00, 0x00 }, /* ; */
    { 0x08, 0x14, 0x22, 0x41, 0x00 }, /* < */
    { 0x14, 0x14, 0x14, 0x14, 0x14 }, /* = */
    { 0x00, 0x41, 0x22, 0x14, 0x08 }, /* > */
    { 0x02, 0x01, 0x51, 0x09, 0x06 }, /* ? */
    { 0x32, 0x49, 0x79, 0x41, 0x3E }, /* @ */
    { 0x7E, 0x11, 0x11, 0x11, 0x7E }, /* A */
    { 0x7F, 0x49, 0x49, 0x49, 0x36 }, /* B */
    { 0x3E, 0x41, 0x41, 0x41, 0x22 }, /* C */
    { 0x7F, 0x41, 0x41, 0x22, 0x1C }, /* D */
    { 0x7F, 0x49, 0x49, 0x49, 0x41 }, /* E */
    { 0x7F, 0x09, 0x09, 0x09, 0x01 }, /* F */
    { 0x3E, 0x41, 0x49, 0x49, 0x7A }, /* G */
    { 0x7F, 0x08, 0x08, 0x08, 0x7F }, /* H */
    { 0x00, 0x41, 0x7F, 0x41, 0x00 }, /* I */
    { 0x20, 0x40, 0x41, 0x3F, 0x01 }, /* J */
    { 0x7F, 0x08, 0x14, 0x22, 0x41 }, /* K */
    { 0x7F, 0x40, 0x40, 0x40, 0x40 }, /* L */
    { 0x7F, 0x02, 0x0C, 0x02, 0x7F }, /* M */
    { 0x7F, 0x04, 0x08, 0x10, 0x7F }, /* N */
    { 0x3E, 0x41, 0x41, 0x41, 0x3E }, /* O */
    { 0x7F, 0x09, 0x09, 0x09, 0x06 }, /* P */
    { 0x3E, 0x41, 0x51, 0x21, 0x5E }, /* Q */
    { 0x7F, 0x09, 0x19, 0x29, 0x46 }, /* R */
    { 0x46, 0x49, 0x49, 0x49, 0x31 }, /* S */
    { 0x01, 0x01, 0x7F, 0x01, 0x01 }, /* T */
    { 0x3F, 0x40, 0x40, 0x40, 0x3F }, /* U */
    { 0x1F, 0x20, 0x40, 0x20, 0x1F }, /* V */
    { 0x3F, 0x40, 0x38, 0x40, 0x3F }, /* W */
    { 0x63, 0x14, 0x08, 0x14, 0x63 }, /* X */
    { 0x07, 0x08, 0x70, 0x08, 0x07 }, /* Y */
    { 0x61, 0x51, 0x49, 0x45, 0x43 }, /* Z */
    { 0x00, 0x1C, 0x22, 0x41, 0x00 }, /* [ */
    { 0x02, 0x04, 0x08, 0x10, 0x20 }, /* \ */
    { 0x00, 0x41, 0x22, 0x1C, 0x00 }, /* ] */
    { 0x04, 0x02, 0x01, 0x02, 0x04 }, /* ^ */
    { 0x40, 0x40, 0x40, 0x40, 0x40 }, /* _ */
    { 0x00, 0x01, 0x02, 0x04, 0x00 }, /* ` */
    { 0x20, 0x54, 0x54, 0x54, 0x78 }, /* a */
    { 0x7F, 0x48, 0x44, 0x44, 0x38 }, /* b */
    { 0x38, 0x44, 0x44, 0x44, 0x20 }, /* c */
    { 0x38, 0x44, 0x44, 0x48, 0x7F }, /* d */
    { 0x38, 0x54, 0x54, 0x54, 0x18 }, /* e */
    { 0x08, 0x7E, 0x09, 0x01, 0x02 }, /* f */
    { 0x0C, 0x52, 0x52, 0x52, 0x3E }, /* g */
    { 0x7F, 0x08, 0x04, 0x04, 0x78 }, /* h */
    { 0x00, 0x44, 0x7D, 0x40, 0x00 }, /* i */
    { 0x20, 0x40, 0x44, 0x3D, 0x00 }, /* j */
    { 0x7F, 0x10, 0x28, 0x44, 0x00 }, /* k */
    { 0x00, 0x41, 0x7F, 0x40, 0x00 }, /* l */
    { 0x7C, 0x04, 0x18, 0x04, 0x78 }, /* m */
    { 0x7C, 0x08, 0x04, 0x04, 0x78 }, /* n */
    { 0x38, 0x44, 0x44, 0x44, 0x38 }, /* o */
    { 0x7C, 0x14, 0x14, 0x14, 0x08 }, /* p */
    { 0x08, 0x14, 0x14, 0x18, 0x7C }, /* q */
    { 0x7C, 0x08, 0x04, 0x04, 0x08 }, /* r */
    { 0x48, 0x54, 0x54, 0x54, 0x20 }, /* s */
    { 0x04, 0x3F, 0x44, 0x40, 0x20 }, /* t */
    { 0x3C, 0x40, 0x40, 0x20, 0x7C }, /* u */
    { 0x1C, 0x20, 0x40, 0x20, 0x1C }, /* v */
    { 0x3C, 0x40, 0x30, 0x40, 0x3C }, /* w */
    { 0x44, 0x28, 0x10, 0x28, 0x44 }, /* x */
    { 0x0C, 0x50, 0x50, 0x50, 0x3C }, /* y */
    { 0x44, 0x64, 0x54, 0x4C, 0x44 }, /* z */
};

/*----------------------------------------------------------------------------*/

static void epd_2in9_load_lut(const epd_ctx_t* ctx) {
    /* LUT for full refresh (from WeAct Studio / Waveshare examples) */
    static const uint8_t lut_full_update[] = {
        0x50, 0xAA, 0x55, 0xAA, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xFF, 0xFF, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    epd_utils_send_command(ctx, EPD_CMD_WRITE_LUT_REGISTER);
    epd_utils_send_data_buffer(ctx, lut_full_update, sizeof(lut_full_update));
}

static void epd_2in9_set_window(const epd_ctx_t* ctx,
                                uint16_t x_start,
                                uint16_t y_start,
                                uint16_t x_end,
                                uint16_t y_end) {
    epd_utils_send_command(ctx, EPD_CMD_SET_RAM_X_ADDRESS_START_END);
    epd_utils_send_data(ctx, (x_start >> 3) & 0xFF);
    epd_utils_send_data(ctx, (x_end >> 3) & 0xFF);

    epd_utils_send_command(ctx, EPD_CMD_SET_RAM_Y_ADDRESS_START_END);
    epd_utils_send_data(ctx, y_start & 0xFF);
    epd_utils_send_data(ctx, (y_start >> 8) & 0xFF);
    epd_utils_send_data(ctx, y_end & 0xFF);
    epd_utils_send_data(ctx, (y_end >> 8) & 0xFF);
}

static void epd_2in9_set_cursor(const epd_ctx_t* ctx, uint16_t x, uint16_t y) {
    epd_utils_send_command(ctx, EPD_CMD_SET_RAM_X_ADDRESS_COUNTER);
    epd_utils_send_data(ctx, (x >> 3) & 0xFF);

    epd_utils_send_command(ctx, EPD_CMD_SET_RAM_Y_ADDRESS_COUNTER);
    epd_utils_send_data(ctx, y & 0xFF);
    epd_utils_send_data(ctx, (y >> 8) & 0xFF);
}

bool epd_2in9_init_display(const epd_ctx_t* ctx) {
    /* Reset and initialize display */
    epd_2in9_reset(ctx);

    epd_utils_send_command(ctx, EPD_CMD_SW_RESET);
    sleep_ms(10); /* Wait for reset to complete before checking BUSY */
    epd_utils_wait_until_idle(ctx);

    epd_utils_send_command(ctx, EPD_CMD_DRIVER_OUTPUT_CONTROL);
    epd_utils_send_data(ctx, (EPD_HEIGHT - 1) & 0xFF);
    epd_utils_send_data(ctx, ((EPD_HEIGHT - 1) >> 8) & 0xFF);
    epd_utils_send_data(ctx, 0x00);

    epd_utils_send_command(ctx, EPD_CMD_BOOSTER_SOFT_START_CONTROL);
    epd_utils_send_data(ctx, 0xD7);
    epd_utils_send_data(ctx, 0xD6);
    epd_utils_send_data(ctx, 0x9D);

    epd_utils_send_command(ctx, EPD_CMD_WRITE_VCOM_REGISTER);
    epd_utils_send_data(ctx, 0xA8);

    epd_utils_send_command(ctx, EPD_CMD_SET_DUMMY_LINE_PERIOD);
    epd_utils_send_data(ctx, 0x1A);

    epd_utils_send_command(ctx, EPD_CMD_SET_GATE_TIME);
    epd_utils_send_data(ctx, 0x08);

    epd_utils_send_command(ctx, EPD_CMD_DATA_ENTRY_MODE_SETTING);
    epd_utils_send_data(ctx, 0x03);

    /* Load the LUT (Look-Up Table) for display refresh waveform */
    epd_2in9_load_lut(ctx);

    epd_utils_send_command(ctx, EPD_CMD_DISPLAY_UPDATE_CONTROL_1);
    epd_utils_send_data(ctx, 0x00);
    epd_utils_send_data(ctx, 0x80);

    /* Clear framebuffer */
    memset(framebuffer, 0xFF, sizeof(framebuffer));

    return true;
}

void epd_2in9_reset(const epd_ctx_t* ctx) {
    gpio_put(ctx->pins.res, 1);
    sleep_ms(200);
    gpio_put(ctx->pins.res, 0);
    sleep_ms(5);
    gpio_put(ctx->pins.res, 1);
    sleep_ms(200);
}

void epd_2in9_clear(const epd_ctx_t* ctx, uint8_t color) {
    epd_2in9_fill(ctx, color);
    epd_2in9_flush(ctx);
}

void epd_2in9_flush(const epd_ctx_t* ctx) {
    epd_2in9_set_window(ctx, 0, 0, ctx->width - 1, ctx->height - 1);
    epd_2in9_set_cursor(ctx, 0, 0);

    epd_utils_send_command(ctx, EPD_CMD_WRITE_RAM);
    epd_utils_send_data_buffer(ctx, framebuffer, sizeof(framebuffer));

    epd_utils_send_command(ctx, EPD_CMD_DISPLAY_UPDATE_CONTROL_2);
    epd_utils_send_data(ctx, 0xF7); /* Full update with LUT from register */

    epd_utils_send_command(ctx, EPD_CMD_MASTER_ACTIVATION);
    epd_utils_wait_until_idle(ctx);
}

void epd_2in9_sleep(const epd_ctx_t* ctx) {
    epd_utils_send_command(ctx, EPD_CMD_DEEP_SLEEP_MODE);
    epd_utils_send_data(ctx, 0x01);
}

void epd_2in9_fill(const epd_ctx_t* ctx, uint8_t color) {
    (void)ctx; /* Unused for now */
    memset(framebuffer, color, sizeof(framebuffer));
}

void epd_2in9_draw_pixel(const epd_ctx_t* ctx,
                         uint16_t x,
                         uint16_t y,
                         uint8_t color) {
    if (x >= ctx->width || y >= ctx->height)
        return;

    uint32_t addr = (x / 8) + y * (ctx->width / 8);
    uint8_t bit   = 7 - (x % 8);

    if (color == EPD_COLOR_BLACK)
        framebuffer[addr] &= ~(1 << bit);
    else
        framebuffer[addr] |= (1 << bit);
}

void epd_2in9_draw_line(const epd_ctx_t* ctx,
                        uint16_t x0,
                        uint16_t y0,
                        uint16_t x1,
                        uint16_t y1,
                        uint8_t color) {
    int16_t dx  = abs(x1 - x0);
    int16_t dy  = abs(y1 - y0);
    int16_t sx  = x0 < x1 ? 1 : -1;
    int16_t sy  = y0 < y1 ? 1 : -1;
    int16_t err = dx - dy;

    for (;;) {
        epd_2in9_draw_pixel(ctx, x0, y0, color);

        if (x0 == x1 && y0 == y1)
            break;

        int16_t e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void epd_2in9_draw_rect(const epd_ctx_t* ctx,
                        uint16_t x,
                        uint16_t y,
                        uint16_t width,
                        uint16_t height,
                        uint8_t color) {
    epd_2in9_draw_line(ctx, x, y, x + width - 1, y, color);
    epd_2in9_draw_line(ctx,
                       x + width - 1,
                       y,
                       x + width - 1,
                       y + height - 1,
                       color);
    epd_2in9_draw_line(ctx,
                       x + width - 1,
                       y + height - 1,
                       x,
                       y + height - 1,
                       color);
    epd_2in9_draw_line(ctx, x, y + height - 1, x, y, color);
}

void epd_2in9_draw_filled_rect(const epd_ctx_t* ctx,
                               uint16_t x,
                               uint16_t y,
                               uint16_t width,
                               uint16_t height,
                               uint8_t color) {
    for (uint16_t i = 0; i < height; i++)
        for (uint16_t j = 0; j < width; j++)
            epd_2in9_draw_pixel(ctx, x + j, y + i, color);
}

void epd_2in9_draw_char(const epd_ctx_t* ctx,
                        uint16_t x,
                        uint16_t y,
                        char c,
                        uint8_t color) {
    if (c < ' ' || c > 'z')
        c = ' ';

    const uint8_t* glyph = font5x7[c - ' '];

    for (uint8_t col = 0; col < 5; col++) {
        uint8_t line = glyph[col];
        for (uint8_t row = 0; row < 8; row++) {
            if (line & 0x01)
                epd_2in9_draw_pixel(ctx, x + col, y + row, color);

            line >>= 1;
        }
    }
}

void epd_2in9_draw_str(const epd_ctx_t* ctx,
                       uint16_t x,
                       uint16_t y,
                       const char* str,
                       uint8_t color) {
    uint16_t cur_x = x;
    while (*str != '\0') {
        epd_2in9_draw_char(ctx, cur_x, y, *str, color);
        cur_x += 6;
        str++;
    }
}
