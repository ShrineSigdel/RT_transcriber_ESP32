/*
 * text_buffer.h
 * =============
 * Public interface for the scrolling line buffer.
 *
 * Responsibilities
 * ----------------
 *  1. Word-wrap an incoming string into lines of ≤ DISPLAY_CHARS_PER_LINE
 *  2. Append those lines to a circular buffer
 *  3. Expose the last N lines so display.cpp can render them
 *
 * Buffer model
 * ------------
 *   BUFFER_CAPACITY lines are kept in memory (more than fit on screen).
 *   When the buffer is full, the oldest line is dropped (shift-left).
 *   display_render() always receives the last DISPLAY_LINES_ON_SCREEN
 *   lines, so the screen always shows the most recent text.
 *
 *   Example with capacity=8, screen=4:
 *     after 10 pushes the buffer holds lines [3..10],
 *     screen shows [7, 8, 9, 10]  ← newest at bottom
 */

#pragma once
#include <Arduino.h>
#include "display.h"   // for DISPLAY_CHARS_PER_LINE, DISPLAY_LINES_ON_SCREEN

// How many lines to keep in RAM.
// Must be ≥ DISPLAY_LINES_ON_SCREEN.  8 is a comfortable margin.
static const int BUFFER_CAPACITY = 8;

/*
 * tbuf_init()
 * Clear the buffer.  Call once in setup().
 */
void tbuf_init();

/*
 * tbuf_push_text(text)
 * Word-wrap `text` into lines and append each to the buffer.
 * Triggers a display redraw via display_render() automatically.
 */
void tbuf_push_text(const String& text);

/*
 * tbuf_get_visible(out[], outCount)
 * Fill `out[]` with the last DISPLAY_LINES_ON_SCREEN lines.
 * `outCount` is set to the number of lines written (may be less if
 * the buffer has fewer lines than DISPLAY_LINES_ON_SCREEN).
 * Caller must ensure out[] has DISPLAY_LINES_ON_SCREEN slots.
 */
void tbuf_get_visible(String out[], int& outCount);
