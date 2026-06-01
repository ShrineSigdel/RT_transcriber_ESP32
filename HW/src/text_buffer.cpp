/*
 * text_buffer.cpp
 * ===============
 * Implements text_buffer.h.
 *
 * Word-wrap algorithm
 * -------------------
 * Given a string "Hello world this is a long sentence":
 *
 *  1. If len(remaining) ≤ DISPLAY_CHARS_PER_LINE → push whole string, done.
 *  2. Scan backwards from index DISPLAY_CHARS_PER_LINE to 0 for a space.
 *  3. If found at index `b`:
 *       push remaining[0 .. b-1]
 *       remaining = remaining[b+1 ..]   (skip the space itself)
 *  4. If not found (a single word longer than the line):
 *       push remaining[0 .. CHARS_PER_LINE-1]   (hard break)
 *       remaining = remaining[CHARS_PER_LINE ..]
 *  5. Repeat from step 1.
 *
 * Circular buffer (shift model)
 * ------------------------------
 * We use a simple shift rather than a true ring buffer.
 * When the buffer is full and a new line arrives:
 *   lineBuffer[0] = lineBuffer[1]   (oldest dropped)
 *   lineBuffer[1] = lineBuffer[2]
 *   ...
 *   lineBuffer[BUFFER_CAPACITY-1] = newLine
 *
 * This keeps the array in chronological order at all times, which makes
 * tbuf_get_visible() trivial — just take the last N elements.
 * For BUFFER_CAPACITY=8 the shift is 7 assignments, negligible on ESP32.
 */

#include "text_buffer.h"
#include "display.h"

// ── Internal state ────────────────────────────────────────────────────────────
static String _buf[BUFFER_CAPACITY];
static int    _count = 0;   // lines currently in buffer (0 .. BUFFER_CAPACITY)


// ─────────────────────────────────────────────────────────────────────────────
void tbuf_init() {
    for (int i = 0; i < BUFFER_CAPACITY; i++) _buf[i] = "";
    _count = 0;
}


// ─────────────────────────────────────────────────────────────────────────────
static void _push_line(const String& line) {
    /*
     * Append one wrapped line to the buffer.
     * If the buffer is at capacity, drop the oldest entry.
     */
    if (_count < BUFFER_CAPACITY) {
        _buf[_count++] = line;
    } else {
        // Shift left — index 0 (oldest) is overwritten
        for (int i = 0; i < BUFFER_CAPACITY - 1; i++) {
            _buf[i] = _buf[i + 1];
        }
        _buf[BUFFER_CAPACITY - 1] = line;
        // _count stays at BUFFER_CAPACITY
    }
}


// ─────────────────────────────────────────────────────────────────────────────
void tbuf_push_text(const String& text) {
    /*
     * Word-wrap `text` into screen-width lines, push each, then redraw.
     */
    String remaining = text;
    remaining.trim();

    while (remaining.length() > 0) {

        // Case 1: fits on one line
        if ((int)remaining.length() <= DISPLAY_CHARS_PER_LINE) {
            _push_line(remaining);
            break;
        }

        // Case 2: too long — find last space within the first CHARS_PER_LINE chars
        int breakAt = -1;
        for (int i = DISPLAY_CHARS_PER_LINE; i >= 1; i--) {
            if (remaining.charAt(i) == ' ') {
                breakAt = i;
                break;
            }
        }

        if (breakAt > 0) {
            // Break at the word boundary
            _push_line(remaining.substring(0, breakAt));
            remaining = remaining.substring(breakAt + 1);  // skip space
        } else {
            // No space found: hard-break (rare — very long single token)
            _push_line(remaining.substring(0, DISPLAY_CHARS_PER_LINE));
            remaining = remaining.substring(DISPLAY_CHARS_PER_LINE);
        }

        remaining.trim();
    }

    // Redraw after all lines for this text segment are pushed
    String visible[DISPLAY_LINES_ON_SCREEN];
    int    visCount = 0;
    tbuf_get_visible(visible, visCount);
    display_render(visible, visCount);
}


// ─────────────────────────────────────────────────────────────────────────────
void tbuf_get_visible(String out[], int& outCount) {
    /*
     * Return the last DISPLAY_LINES_ON_SCREEN lines of the buffer.
     * If the buffer has fewer lines, return all of them.
     */
    int start = (_count > DISPLAY_LINES_ON_SCREEN)
                ? _count - DISPLAY_LINES_ON_SCREEN
                : 0;

    outCount = 0;
    for (int i = start; i < _count; i++) {
        out[outCount++] = _buf[i];
    }
}
