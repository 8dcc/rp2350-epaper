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

/*----------------------------------------------------------------------------*/

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
    memset(ctx->framebuffer, 0xFF, ctx->framebuffer_size);

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

void epd_2in9_flush(const epd_ctx_t* ctx) {
    epd_2in9_set_window(ctx, 0, 0, ctx->width - 1, ctx->height - 1);
    epd_2in9_set_cursor(ctx, 0, 0);

    epd_utils_send_command(ctx, EPD_CMD_WRITE_RAM);
    epd_utils_send_data_buffer(ctx, ctx->framebuffer, ctx->framebuffer_size);

    epd_utils_send_command(ctx, EPD_CMD_DISPLAY_UPDATE_CONTROL_2);
    epd_utils_send_data(ctx, 0xF7); /* Full update with LUT from register */

    epd_utils_send_command(ctx, EPD_CMD_MASTER_ACTIVATION);
    epd_utils_wait_until_idle(ctx);
}

void epd_2in9_sleep(const epd_ctx_t* ctx) {
    epd_utils_send_command(ctx, EPD_CMD_DEEP_SLEEP_MODE);
    epd_utils_send_data(ctx, 0x01);
}

void epd_2in9_clear(const epd_ctx_t* ctx, uint8_t color) {
    uint8_t fill_value;

    switch (color) {
        case EPD_COLOR_BLACK:
            fill_value = 0x00;
            break;

        case EPD_COLOR_WHITE:
            fill_value = 0xFF;
            break;

        default:
            EPD_LOG("Invalid color enumerator (%d).", color);
            return;
    }

    memset(ctx->framebuffer, fill_value, ctx->framebuffer_size);
}

void epd_2in9_draw_pixel(const epd_ctx_t* ctx,
                         uint16_t x,
                         uint16_t y,
                         uint8_t color) {
    if (x >= ctx->width || y >= ctx->height)
        return;

    uint32_t addr = (x / 8) + y * (ctx->width / 8);
    uint8_t bit   = 7 - (x % 8);

    switch (color) {
        case EPD_COLOR_BLACK:
            ctx->framebuffer[addr] &= ~(1 << bit);
            break;

        case EPD_COLOR_WHITE:
            ctx->framebuffer[addr] |= (1 << bit);
            break;

        default:
            EPD_LOG("Invalid color enumerator (%d).", color);
            return;
    }
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
    const uint8_t* glyph = font_get_glyph(c);
    for (uint8_t glyph_x = 0; glyph_x < FONT_WIDTH; glyph_x++) {
        const uint8_t line = glyph[col];
        for (uint8_t glyph_y = 0; glyph_y < FONT_HEIGHT; glyph_y++)
            if (line & (1 << row))
                epd_2in9_draw_pixel(ctx, x + glyph_x, y + glyph_y, color);
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
