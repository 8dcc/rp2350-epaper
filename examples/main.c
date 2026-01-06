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

#include <stdio.h>
#include "pico/stdlib.h"

#include "epaper_display.h"

/*
 * Compile-time pin definitions for the 'pin_config' argument of 'epd_init'.
 */
#define PIN_MOSI 11
#define PIN_SCK  10
#define PIN_CS   9
#define PIN_DC   8
#define PIN_RES  12
#define PIN_BUSY 13

/*----------------------------------------------------------------------------*/

static void demo_text(epd_ctx_t* ctx) {
    printf("Drawing text...\n");
    epd_fill(ctx, EPD_COLOR_WHITE);
    epd_draw_str(ctx, 10, 10, "Hello Pico 2 W", EPD_COLOR_BLACK);
    epd_draw_str(ctx, 10, 30, "WeAct 2.9\" EPD", EPD_COLOR_BLACK);
    epd_draw_str(ctx, 10, 50, "296x128 pixels", EPD_COLOR_BLACK);
    epd_flush(ctx);
    sleep_ms(3000);
}

static void demo_shapes(epd_ctx_t* ctx) {
    printf("Drawing shapes...\n");
    epd_fill(ctx, EPD_COLOR_WHITE);

    /* Rectangle */
    epd_draw_rect(ctx, 10, 10, 50, 30, EPD_COLOR_BLACK);

    /* Filled rectangle */
    epd_draw_filled_rect(ctx, 70, 10, 50, 30, EPD_COLOR_BLACK);

    /* Lines */
    epd_draw_line(ctx, 10, 60, 120, 60, EPD_COLOR_BLACK);
    epd_draw_line(ctx, 10, 70, 120, 100, EPD_COLOR_BLACK);
    epd_draw_line(ctx, 120, 70, 10, 100, EPD_COLOR_BLACK);

    epd_flush(ctx);
    sleep_ms(3000);
}

static void demo_pattern(epd_ctx_t* ctx) {
    printf("Drawing pixel pattern...\n");
    epd_fill(ctx, EPD_COLOR_WHITE);

    for (uint16_t i = 0; i < ctx->width; i += 4) {
        for (uint16_t j = 0; j < 128; j += 4) {
            epd_draw_pixel(ctx, i, j, EPD_COLOR_BLACK);
        }
    }

    epd_draw_str(ctx, 30, 110, "Pixel Grid", EPD_COLOR_BLACK);
    epd_flush(ctx);
    sleep_ms(3000);
}

static void demo_animation(epd_ctx_t* ctx) {
    printf("Running animation...\n");

    for (int i = 0; i < 5; i++) {
        epd_fill(ctx, EPD_COLOR_WHITE);

        /* Moving rectangle */
        uint16_t x = i * 20;
        epd_draw_filled_rect(ctx, x, 50, 30, 30, EPD_COLOR_BLACK);

        char frame_text[20];
        sprintf(frame_text, "Frame %d", i + 1);
        epd_draw_str(ctx, 10, 10, frame_text, EPD_COLOR_BLACK);

        epd_flush(ctx);
        sleep_ms(1000);
    }
}

/*----------------------------------------------------------------------------*/

int main(void) {
    /* Initialize stdio for USB serial */
    stdio_init_all();
    sleep_ms(2000); /* Wait for USB to stabilize */

    printf("\n=== WeAct Studio 2.9\" E-Paper Demo ===\n");
    printf("Initializing display...\n");

    /* Initialize E-Paper display */
    const epd_pin_config_t pin_config = { .sck  = PIN_SCK,
                                          .mosi = PIN_MOSI,
                                          .cs   = PIN_CS,
                                          .dc   = PIN_DC,
                                          .res  = PIN_RES,
                                          .busy = PIN_BUSY };
    epd_ctx_t display_ctx;
    if (!epd_init(&display_ctx, &pin_config, EPD_MODEL_2IN9)) {
        printf("Failed to initialize E-Paper Display. Halting.");
        return 1;
    }

    /* Clear display */
    printf("Clearing display...\n");
    epd_clear(&display_ctx, EPD_COLOR_WHITE);
    sleep_ms(2000);

    /* Run demos */
    demo_text(&display_ctx);
    demo_shapes(&display_ctx);
    demo_pattern(&display_ctx);
    demo_animation(&display_ctx);

    /* Final screen */
    printf("Displaying final screen...\n");
    epd_fill(&display_ctx, EPD_COLOR_WHITE);
    epd_draw_str(&display_ctx, 20, 50, "Demo Complete", EPD_COLOR_BLACK);
    epd_draw_str(&display_ctx, 20, 70, "Press RESET to", EPD_COLOR_BLACK);
    epd_draw_str(&display_ctx, 20, 90, "run again", EPD_COLOR_BLACK);
    epd_flush(&display_ctx);

    /* Put display to sleep */
    printf("Putting display to sleep...\n");
    sleep_ms(2000);
    epd_sleep(&display_ctx);

    printf("Done!\n");

    /* Main loop (do nothing) */
    for (;;)
        tight_loop_contents();

    return 0;
}
