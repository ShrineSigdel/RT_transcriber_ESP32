/*
 * display.cpp
 * ===========
 * Implements display.h using the U8g2 library.
 *
 * U8g2 driver used: U8G2_SH1106_128X64_NONAME_F_HW_I2C
 *   _F_     = full frame-buffer mode
 *             → all drawing goes to RAM first, then flushed to OLED in
 *               one call (sendBuffer).  No partial-update flicker.
 *   HW_I2C  = hardware I2C peripheral on GPIO 21 (SDA) / 22 (SCL)
 *   U8G2_R0 = no rotation
 *
 * Draw cycle (used in both display_status and display_render)
 * -----------------------------------------------------------
 *   clearBuffer()  — zero the 1 KB RAM buffer
 *   drawStr(...)   — write glyph data into RAM  (fast, no I2C traffic)
 *   sendBuffer()   — flush RAM → OLED via I2C   (the only slow step)
 *
 * Keeping all U8g2 calls in this file means the rest of the codebase
 * never needs to know which display chip or which font is in use.
 */

#include "display.h"
#include <U8g2lib.h>
#include <Wire.h>

// ── Driver instance (file-scope, not exposed in the header) ───────────────────
static U8G2_SH1106_128X64_NONAME_F_HW_I2C _u8g2(
    U8G2_R0,        // rotation: 0°
    U8X8_PIN_NONE   // reset: not connected
);

// ── Line Y positions (top-of-glyph baseline, setFontPosTop) ──────────────────
// Font height = 10 px, gap = 3 px → stride = 13 px
// First line at y=2 gives 2 px top margin.
static const int _LINE_Y[DISPLAY_LINES_ON_SCREEN] = { 2, 15, 28, 41 };


// ─────────────────────────────────────────────────────────────────────────────
void display_init() {
    _u8g2.begin();

    // Font: 6 px wide × 10 px tall, proportional, full ASCII + Latin-1
    _u8g2.setFont(u8g2_font_6x10_tf);

    // y coordinate = TOP of the glyph (easier mental model than baseline)
    _u8g2.setFontPosTop();

    // Draw color 1 = white pixels on black background
    _u8g2.setDrawColor(1);
}


// ─────────────────────────────────────────────────────────────────────────────
void display_status(const char* line1, const char* line2) {
    _u8g2.clearBuffer();

    if (line1 && strlen(line1) > 0) {
        _u8g2.drawStr(0, _LINE_Y[0], line1);
    }
    if (line2 && strlen(line2) > 0) {
        _u8g2.drawStr(0, _LINE_Y[1], line2);
    }

    _u8g2.sendBuffer();
}


// ─────────────────────────────────────────────────────────────────────────────
void display_render(const String lines[], int count) {
    /*
     * Draw up to DISPLAY_LINES_ON_SCREEN lines from the `lines[]` array.
     *
     * The caller (tbuf_push_text) always passes the tail of the buffer, so
     * lines[0] is the oldest visible line and lines[count-1] is the newest.
     *
     * The newest line is drawn twice (at x=0 and x=1) to make it appear
     * slightly bolder — a cheap way to highlight new transcript text without
     * needing a second font.
     */
    _u8g2.clearBuffer();

    int draw = min(count, DISPLAY_LINES_ON_SCREEN);

    for (int i = 0; i < draw; i++) {
        bool isNewest = (i == draw - 1);
        const char* cstr = lines[i].c_str();

        _u8g2.drawStr(0, _LINE_Y[i], cstr);

        if (isNewest) {
            // Second draw at x=1 shifts one pixel right → thicker appearance
            _u8g2.drawStr(1, _LINE_Y[i], cstr);
        }
    }

    _u8g2.sendBuffer();
}
