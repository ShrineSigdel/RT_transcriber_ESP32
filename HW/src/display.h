/*
 * display.h
 * =========
 * Public interface for the OLED display module.
 *
 * Other modules only include this header — they never touch U8g2 directly.
 * All U8g2 types and calls are hidden inside display.cpp.
 *
 * Hardware assumed
 * ----------------
 *   SH1106 1.3" OLED, 128×64 px, I2C
 *   SDA → GPIO 21   (ESP32 default I2C SDA)
 *   SCL → GPIO 22   (ESP32 default I2C SCL)
 *   VCC → 3.3V
 *   GND → GND
 *
 * Font & layout maths
 * -------------------
 *   Font: u8g2_font_6x10_tf
 *     glyph width  = 6 px  →  floor(128/6) = 21 chars per line
 *     glyph height = 10 px
 *     line gap     = 3 px  →  line stride = 13 px
 *     lines on screen: floor(64/13) = 4
 *
 *   Line Y baselines (top-of-glyph, setFontPosTop):
 *     line 0 → y =  2
 *     line 1 → y = 15
 *     line 2 → y = 28
 *     line 3 → y = 41
 */

#pragma once
#include <Arduino.h>

// ── Geometry constants (used by text_buffer.h for wrap width) ─────────────────
static const int DISPLAY_CHARS_PER_LINE  = 21;
static const int DISPLAY_LINES_ON_SCREEN = 4;

/*
 * display_init()
 * Initialise I2C and the SH1106 driver.
 * Must be called once in setup() before any other display_* function.
 */
void display_init();

/*
 * display_status(line1, line2)
 * Clear the screen and show 1–2 short status strings.
 * Used during boot, WiFi connect, WS reconnect.
 * line2 may be nullptr or "" to show only one line.
 */
void display_status(const char* line1, const char* line2 = "");

/*
 * display_render(lines[], count)
 * Redraw the screen using up to DISPLAY_LINES_ON_SCREEN strings.
 * Caller passes a pointer to the tail of the line buffer and the
 * number of lines to draw (clamped to DISPLAY_LINES_ON_SCREEN).
 * The last line is drawn bold (pixel-shifted) to highlight new text.
 */
void display_render(const String lines[], int count);
