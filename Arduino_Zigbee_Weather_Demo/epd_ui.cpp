/**
 * EPD UI – draw indoor/outdoor temp, humidity, and weather icon (partial update).
 * Bitmap fonts from the fonts headers (GFX format); sizes per LAYOUT_IMPLEMENTATION_PLAN.md.
 */

#include "epd_ui.h"
#include "Display_EPD_W21.h"
#include "weather_icons/weather_icons_4g.h"
#include "fonts/gfxfont.h"
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
#include <pgmspace.h>  /* ESP32: pgm_read_ptr reads 32-bit pointers from flash */
#endif
#include "fonts/InterBold72.h"
#include "fonts/InterBold48.h"
#include "fonts/InterRegular32.h"
#include "fonts/SourceSans22.h"
#include "fonts/Aktinson24.h"
#include "fonts/InterRegular28.h"
#include "fonts/InterBold14.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

/* Read byte/word/dword from PROGMEM when not provided by Arduino. */
#ifndef pgm_read_byte
  #define pgm_read_byte(addr)  (*(const unsigned char *)(addr))
#endif
#ifndef pgm_read_word
  #define pgm_read_word(addr)  (*(const uint16_t *)(addr))
#endif
#ifndef pgm_read_dword
  #define pgm_read_dword(addr) (*(const uint32_t *)(addr))
#endif
/* On 32-bit (e.g. ESP32) font pointers in PROGMEM are 32-bit; 16-bit read causes load fault. */
#ifndef pgm_read_ptr
  #if (defined(__SIZEOF_POINTER__) && __SIZEOF_POINTER__ >= 4) || (UINTPTR_MAX > 0xFFFFu)
    #define pgm_read_ptr(addr) ((const void *)(uintptr_t)pgm_read_dword(addr))
  #else
    #define pgm_read_ptr(addr) ((const void *)(uintptr_t)pgm_read_word(addr))
  #endif
#endif

/* 6x8 font: 6 bytes per char (column-major), LSB = top pixel. Chars: 0-9, . - ° C % space */
static const unsigned char font_6x8[][6] = {
  { 0x3E, 0x51, 0x49, 0x45, 0x3E, 0x00 }, /* 0 */
  { 0x00, 0x42, 0x7F, 0x40, 0x00, 0x00 }, /* 1 */
  { 0x62, 0x51, 0x49, 0x49, 0x46, 0x00 }, /* 2 */
  { 0x22, 0x49, 0x49, 0x49, 0x36, 0x00 }, /* 3 */
  { 0x18, 0x14, 0x12, 0x7F, 0x10, 0x00 }, /* 4 */
  { 0x27, 0x45, 0x45, 0x45, 0x39, 0x00 }, /* 5 */
  { 0x3C, 0x4A, 0x49, 0x49, 0x30, 0x00 }, /* 6 */
  { 0x01, 0x71, 0x09, 0x05, 0x03, 0x00 }, /* 7 */
  { 0x36, 0x49, 0x49, 0x49, 0x36, 0x00 }, /* 8 */
  { 0x06, 0x49, 0x49, 0x29, 0x1E, 0x00 }, /* 9 */
  { 0x00, 0x00, 0x30, 0x30, 0x00, 0x00 }, /* . */
  { 0x08, 0x08, 0x08, 0x08, 0x08, 0x00 }, /* - */
  { 0x06, 0x09, 0x09, 0x06, 0x00, 0x00 }, /* ° */
  { 0x1E, 0x21, 0x21, 0x21, 0x12, 0x00 }, /* C */
  { 0x23, 0x13, 0x08, 0x64, 0x62, 0x00 }, /* % */
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, /* space - 15 */
  { 0x00, 0x00, 0x7F, 0x00, 0x00, 0x00 }, /* I - 16 */
  { 0x7C, 0x0A, 0x09, 0x09, 0x06, 0x00 }, /* n - 17 */
  { 0x7F, 0x42, 0x42, 0x42, 0x3C, 0x00 }, /* d - 18 */
  { 0x3E, 0x41, 0x41, 0x41, 0x3E, 0x00 }, /* o - 19 */
  { 0x7F, 0x09, 0x09, 0x09, 0x06, 0x00 }, /* r - 20 */
  { 0x3C, 0x42, 0x42, 0x42, 0x4C, 0x00 }, /* u - 21 */
  { 0x01, 0x01, 0x7F, 0x01, 0x01, 0x00 }, /* t - 22 */
  { 0x7C, 0x12, 0x11, 0x12, 0x7C, 0x00 }, /* a - 23 */
  { 0x46, 0x49, 0x49, 0x49, 0x31, 0x00 }, /* e - 24 */
  { 0x00, 0x36, 0x36, 0x00, 0x00, 0x00 }, /* : - 25 */
  { 0x7F, 0x08, 0x08, 0x08, 0x7F, 0x00 }, /* H - 26 */
  { 0x00, 0x00, 0x7F, 0x40, 0x40, 0x00 }, /* l - 27 */
  { 0x47, 0x49, 0x49, 0x49, 0x31, 0x00 }, /* S - 28 */
  { 0x3C, 0x42, 0x42, 0x42, 0x4C, 0x00 }, /* w - 29 (reuse u shape) */
  { 0x7F, 0x40, 0x40, 0x40, 0x40, 0x7E }, /* U - 30 (clear U: verticals + flat bottom) */
  { 0x7F, 0x08, 0x14, 0x22, 0x41, 0x00 }, /* p - 31 */
  { 0x7F, 0x01, 0x02, 0x04, 0x08, 0x7F }, /* N - 32 (diagonal top-left to bottom-right) */
  { 0x7F, 0x04, 0x08, 0x10, 0x08, 0x7F }, /* M - 33 */
  { 0x7F, 0x09, 0x09, 0x09, 0x01, 0x00 }, /* F - 34 */
  { 0x41, 0x22, 0x14, 0x08, 0x08, 0x08 }, /* Y - 35 */
  { 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 }, /* / - 36 (forward slash) */
  { 0x7C, 0x08, 0x04, 0x08, 0x7C, 0x00 }, /* m - 37 (lowercase) */
  { 0x02, 0x25, 0x25, 0x25, 0x19, 0x00 }, /* s - 38 (lowercase) */
};

#define FONT_W  6
#define FONT_H  8

static int char_to_index(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c == '.') return 10;
  if (c == '-') return 11;
  if ((unsigned char)c == 0xB0) return 12; /* ° */
  if (c == 'C' || c == 'c') return 13;
  if (c == '%') return 14;
  if (c == ' ') return 15;
  if (c == 'I') return 16;
  if (c == 'n') return 17;
  if (c == 'd') return 18;
  if (c == 'o' || c == 'O') return 19;
  if (c == 'r') return 20;
  if (c == 'u') return 21;
  if (c == 't') return 22;
  if (c == 'a') return 23;
  if (c == 'e') return 24;
  if (c == ':') return 25;
  if (c == 'H') return 26;
  if (c == 'l') return 27;
  if (c == 'S') return 28;
  if (c == 'w') return 29;
  if (c == 'U') return 30;
  if (c == 'p') return 31;
  if (c == 'N') return 32;
  if (c == 'T') return 22;  /* same glyph as t */
  if (c == 'M') return 33;
  if (c == 'W') return 29;  /* same as w */
  if (c == 'L') return 27;  /* same as l */
  if (c == 'A') return 23;  /* same as a */
  if (c == 'F') return 34;
  if (c == 'D') return 18;  /* same as d */
  if (c == 'Y') return 35;
  if (c == '/') return 36;
  if (c == 'm') return 37;  /* lowercase m */
  if (c == 's') return 38;  /* lowercase s */
  return 15;
}

static void set_pixel(unsigned char *buf, unsigned int buf_w, unsigned int buf_h,
                      unsigned int x, unsigned int y) {
  if (x >= buf_w || y >= buf_h) return;
  unsigned int row_stride = (buf_w + 7u) / 8u;
  unsigned int byte_ix = y * row_stride + (x / 8u);
  unsigned int bit_ix  = 7 - (x % 8u);
  buf[byte_ix] |= (unsigned char)(1 << bit_ix);
}

static void draw_char(unsigned char *buf, unsigned int buf_w, unsigned int buf_h,
                      unsigned int px, unsigned int py, char c) {
  int idx = char_to_index(c);
  const unsigned char *col = font_6x8[idx];
  for (unsigned int cx = 0; cx < FONT_W; cx++) {
    for (unsigned int ry = 0; ry < FONT_H; ry++) {
      if ((col[cx] >> ry) & 1)
        set_pixel(buf, buf_w, buf_h, px + cx, py + ry);
    }
  }
}

static void draw_string(unsigned char *buf, unsigned int buf_w, unsigned int buf_h,
                        unsigned int px, unsigned int py, const char *str) {
  while (*str) {
    draw_char(buf, buf_w, buf_h, px, py, *str++);
    px += FONT_W + 1;
  }
}

#define TEMP_BUF_SIZE    ((EPD_UI_TEMP_REGION_W * EPD_UI_TEMP_REGION_H) / 8)
#define HUMID_BUF_SIZE   ((EPD_UI_HUMID_REGION_W * EPD_UI_HUMID_REGION_H) / 8)
#define ICON_ROW_STRIDE  (((EPD_UI_ICON_REGION_W) + 7u) / 8u)
#define ICON_BUF_SIZE    (ICON_ROW_STRIDE * (EPD_UI_ICON_REGION_H))
#define LARGE_ICON_ROW_STRIDE (((EPD_UI_LARGE_ICON_W) + 7u) / 8u)
#define LARGE_ICON_BUF_SIZE (LARGE_ICON_ROW_STRIDE * (EPD_UI_LARGE_ICON_H))
#define BATTERY_BUF_SIZE  ((EPD_UI_BATTERY_REGION_W * EPD_UI_BATTERY_REGION_H) / 8)
#define STATUS_BUF_SIZE   ((EPD_UI_STATUS_REGION_W * EPD_UI_STATUS_REGION_H) / 8)

/* Full-screen 4G buffer: 96000 bytes. Panel may expect column-major (X=height, Y=width). */
#define EPD_4G_BYTES_PER_COL  (200u)  /* 800/8*2 */
static unsigned char epd_4g_buffer[EPD_UI_4G_BUFFER_SIZE];

/* Invert buffer so panel shows white background, black content. */
#define EPD_UI_4G_INVERT     1

/* 1 = column-major (each column 800px then next column); 0 = row-major. Fixes 5x repeat. */
#define EPD_UI_4G_COLUMN_MAJOR  1

/* When set (only during build_demo_4g), flip Y only so orientation matches HELLO but text reads L→R (not mirrored). */
static uint8_t epd_ui_4g_flip_y = 0;

static void set_pixel_4g(unsigned int x, unsigned int y) {
  if (epd_ui_4g_flip_y) {
    y = (EPD_HEIGHT - 1u) - y;
  }
  if (x >= EPD_WIDTH || y >= EPD_HEIGHT) return;
  unsigned int byte_ix;
  unsigned int nibble_shift;
#if EPD_UI_4G_COLUMN_MAJOR
  /* Column x, row y. 2 bytes per 8 pixels = 4 pixels (4 nibbles) per byte. */
  unsigned int col_byte = (y / 8u) * 2u + ((y % 8u) / 4u);
  byte_ix = x * EPD_4G_BYTES_PER_COL + col_byte;
  /* Each byte has 4 nibbles (pixels): y%4 = 0,1,2,3 -> nibbles 0..3. */
  nibble_shift = (y % 4u);
#else
  unsigned int group = y * 60u + (x / 8u);
  byte_ix = group * 2u + ((x % 8u) / 4u);
  nibble_shift = (x % 4u);
#endif
  epd_4g_buffer[byte_ix] |= (unsigned char)(3 << (6 - nibble_shift * 2));
}

/* Set one pixel in 4G buffer to 2-bit value (0=white .. 3=black). */
static void set_pixel_4g_value(unsigned int x, unsigned int y, unsigned int value) {
  if (value > 3u) return;
  if (epd_ui_4g_flip_y) {
    y = (EPD_HEIGHT - 1u) - y;
  }
  if (x >= EPD_WIDTH || y >= EPD_HEIGHT) return;
  unsigned int byte_ix;
  unsigned int nibble_shift;
#if EPD_UI_4G_COLUMN_MAJOR
  unsigned int col_byte = (y / 8u) * 2u + ((y % 8u) / 4u);
  byte_ix = x * EPD_4G_BYTES_PER_COL + col_byte;
  nibble_shift = (y % 4u);
#endif
  epd_4g_buffer[byte_ix] &= (unsigned char)(~(3u << (6 - nibble_shift * 2)));
  epd_4g_buffer[byte_ix] |= (unsigned char)((value & 3u) << (6 - nibble_shift * 2));
}

/* Fill rectangle (all pixels black). */
static void fill_rect_4g(unsigned int bx, unsigned int by, unsigned int w, unsigned int h) {
  for (unsigned int dy = 0; dy < h; dy++)
    for (unsigned int dx = 0; dx < w; dx++)
      set_pixel_4g(bx + dx, by + dy);
}

/* Fill rectangle with 4-gray value (0=white .. 3=black). */
static void fill_rect_4g_value(unsigned int bx, unsigned int by, unsigned int w, unsigned int h, unsigned int value) {
  if (value > 3u) return;
  for (unsigned int dy = 0; dy < h; dy++)
    for (unsigned int dx = 0; dx < w; dx++)
      set_pixel_4g_value(bx + dx, by + dy, value);
}

/* 1-pixel rectangle outline. */
static void draw_rect_outline_4g(unsigned int bx, unsigned int by, unsigned int w, unsigned int h) {
  for (unsigned int dx = 0; dx < w; dx++) {
    set_pixel_4g(bx + dx, by);
    set_pixel_4g(bx + dx, by + h - 1u);
  }
  for (unsigned int dy = 0; dy < h; dy++) {
    set_pixel_4g(bx, by + dy);
    set_pixel_4g(bx + w - 1u, by + dy);
  }
}

/* Horizontal line from x0 to x1 inclusive at y. */
static void draw_hline_4g(unsigned int x0, unsigned int x1, unsigned int y) {
  for (; x0 <= x1; x0++)
    set_pixel_4g(x0, y);
}

/* Degree symbol in InterRegular32 is at 0x2A (reused from *). */
static const char degree_c[] = "\x2A";

/* Draw a GFX font string into 4G buffer at baseline (x, y). gray_value 0=white, 3=black, 1/2=gray. */
static void draw_gfxfont_string_4g(int x_baseline, int y_baseline, const char *str,
                                   const GFXfont *font, unsigned int gray_value) {
  if (!str || !font || gray_value > 3u) return;
  const uint8_t *bitmap = (const uint8_t *)pgm_read_ptr(&font->bitmap);
  const GFXglyph *glyph_base = (const GFXglyph *)pgm_read_ptr(&font->glyph);
  uint8_t first = pgm_read_byte(&font->first);
  uint8_t last = pgm_read_byte(&font->last);
  int x = x_baseline;
  while (*str) {
    unsigned char c = (unsigned char)*str++;
    if (c < first || c > last) continue;
    const GFXglyph *glyph = glyph_base + (c - first);
    uint16_t bitmapOffset = pgm_read_word(&glyph->bitmapOffset);
    uint8_t w = pgm_read_byte(&glyph->width);
    uint8_t h = pgm_read_byte(&glyph->height);
    int8_t xOff = (int8_t)pgm_read_byte(&glyph->xOffset);
    int8_t yOff = (int8_t)pgm_read_byte(&glyph->yOffset);
    uint8_t xAdv = pgm_read_byte(&glyph->xAdvance);
    if (w == 0 || h == 0) {
      x += (int)xAdv;
      continue;
    }
    /* Adafruit GFX format: row-major bits, 8 pixels per byte, MSB = left. bitIndex = x + width*y */
    unsigned int bytes_per_row = (unsigned int)((w + 7) / 8);
    int base_x = x + (int)xOff;
    int base_y = y_baseline + (int)yOff;
    for (unsigned int gy = 0; gy < (unsigned int)h; gy++) {
      for (unsigned int gx = 0; gx < (unsigned int)w; gx++) {
        unsigned int bit_index = gx + (unsigned int)w * gy;
        unsigned int byte_ix = bitmapOffset + (bit_index >> 3u);
        unsigned char byte_val = pgm_read_byte(bitmap + byte_ix);
        if (byte_val & (0x80u >> (bit_index & 7u))) {
          int px = base_x + (int)gx;
          int py = base_y + (int)gy;
          if (px >= 0 && py >= 0 && (unsigned int)px < EPD_WIDTH && (unsigned int)py < EPD_HEIGHT)
            set_pixel_4g_value((unsigned int)px, (unsigned int)py, gray_value);
        }
      }
    }
    x += (int)xAdv;
  }
}

/* Return total xAdvance of string in pixels for a GFX font (for right-align). */
static unsigned int gfxfont_string_width(const char *str, const GFXfont *font) {
  if (!str || !font) return 0u;
  const GFXglyph *glyph_base = (const GFXglyph *)pgm_read_ptr(&font->glyph);
  uint8_t first = pgm_read_byte(&font->first);
  uint8_t last = pgm_read_byte(&font->last);
  unsigned int w = 0u;
  while (*str) {
    unsigned char c = (unsigned char)*str++;
    if (c >= first && c <= last)
      w += (unsigned int)pgm_read_byte(&glyph_base[c - first].xAdvance);
  }
  return w;
}

/* Draw string vertically (one character per line, stacked). Uses 8x scale. */
static void draw_string_4g_vertical_8x(unsigned int base_x, unsigned int base_y, const char *str) {
  if (!str) return;
  unsigned int py = base_y;
  while (*str) {
    int idx = char_to_index(*str++);
    const unsigned char *col = font_6x8[idx];
    for (unsigned int cx = 0; cx < FONT_W; cx++) {
      for (unsigned int ry = 0; ry < FONT_H; ry++) {
        if ((col[cx] >> ry) & 1) {
          unsigned int bx = base_x + cx * 8u, by = py + ry * 8u;
          for (unsigned int dy = 0; dy < 8u; dy++)
            for (unsigned int dx = 0; dx < 8u; dx++)
              set_pixel_4g(bx + dx, by + dy);
        }
      }
    }
    py += (unsigned int)(FONT_H * 8u) + 8u;
  }
}

/* Battery: 54x32, 2px outline, nub, number centered. Left side. 4-gray value 2 (dark gray). */
#define EPD_UI_BATTERY_GRAY  2u   /* 0=white, 1=light gray, 2=dark gray, 3=black */
static void draw_battery_icon_4g(unsigned int bx, unsigned int by, float percent) {
  const unsigned int w = EPD_UI_BATTERY_ICON_W;
  const unsigned int h = EPD_UI_BATTERY_ICON_H;
  const unsigned int t = 2u;  /* outline thickness */
  const unsigned int v = EPD_UI_BATTERY_GRAY;
  /* 2px outline: top, bottom, left, right */
  fill_rect_4g_value(bx, by, w, t, v);
  fill_rect_4g_value(bx, by + h - t, w, t, v);
  fill_rect_4g_value(bx, by, t, h, v);
  fill_rect_4g_value(bx + w - t, by, t, h, v);
  /* Nub on right, 2px wide */
  for (unsigned int dx = 0; dx < 2u; dx++)
    for (unsigned int dy = 6u; dy < h - 6u; dy++)
      set_pixel_4g_value(bx + w + dx, by + dy, v);
  /* Number centered in inner area */
  char str[8];
  int p = (percent <= 0) ? 0 : (percent >= 100) ? 100 : (int)(percent + 0.5f);
  (void)snprintf(str, sizeof(str), "%d", p);
  unsigned int str_w = gfxfont_string_width(str, &InterTempSemiBold14pt7b);
  unsigned int inner_w = w - 2u * t;
  int tx = (int)bx + (int)t + (int)((inner_w > str_w) ? (inner_w - str_w) / 2u : 0u);
  int ty = (int)by + 26;
  draw_gfxfont_string_4g(tx, ty, str, &InterTempSemiBold14pt7b, v);
}

static void draw_string_4g(unsigned int base_x, unsigned int base_y, const char *str) {
  if (!str) return;
  unsigned int px = base_x, py = base_y;
  while (*str) {
    int idx = char_to_index(*str++);
    const unsigned char *col = font_6x8[idx];
    for (unsigned int cx = 0; cx < FONT_W; cx++) {
      for (unsigned int ry = 0; ry < FONT_H; ry++) {
        if ((col[cx] >> ry) & 1)
          set_pixel_4g(px + cx, py + ry);
      }
    }
    px += FONT_W + 1;
  }
}

/* Draw string at 2x scale (black on white). */
static void draw_string_4g_scaled(unsigned int base_x, unsigned int base_y, const char *str) {
  if (!str) return;
  unsigned int px = base_x, py = base_y;
  while (*str) {
    int idx = char_to_index(*str++);
    const unsigned char *col = font_6x8[idx];
    for (unsigned int cx = 0; cx < FONT_W; cx++) {
      for (unsigned int ry = 0; ry < FONT_H; ry++) {
        if ((col[cx] >> ry) & 1) {
          set_pixel_4g(px + cx * 2u,     py + ry * 2u);
          set_pixel_4g(px + cx * 2u + 1u, py + ry * 2u);
          set_pixel_4g(px + cx * 2u,     py + ry * 2u + 1u);
          set_pixel_4g(px + cx * 2u + 1u, py + ry * 2u + 1u);
        }
      }
    }
    px += (FONT_W + 1) * 2u;
  }
}

/* Draw string at 4x scale: each font pixel -> 4x4 block (height 32px to match battery). */
static void draw_string_4g_scaled_4x(unsigned int base_x, unsigned int base_y, const char *str) {
  if (!str) return;
  unsigned int px = base_x, py = base_y;
  while (*str) {
    int idx = char_to_index(*str++);
    const unsigned char *col = font_6x8[idx];
    for (unsigned int cx = 0; cx < FONT_W; cx++) {
      for (unsigned int ry = 0; ry < FONT_H; ry++) {
        if ((col[cx] >> ry) & 1) {
          unsigned int bx = px + cx * 4u, by = py + ry * 4u;
          for (unsigned int dy = 0; dy < 4u; dy++)
            for (unsigned int dx = 0; dx < 4u; dx++)
              set_pixel_4g(bx + dx, by + dy);
        }
      }
    }
    px += (FONT_W + 1u) * 4u;
  }
}

/* Draw string at 3x scale: each font pixel -> 3x3 block (bigger, solid). */
#define UI_3X_CHAR_W  ((FONT_W + 1u) * 3u)   /* 21 px per character */
static void draw_string_4g_scaled_3x(unsigned int base_x, unsigned int base_y, const char *str) {
  if (!str) return;
  unsigned int px = base_x, py = base_y;
  while (*str) {
    int idx = char_to_index(*str++);
    const unsigned char *col = font_6x8[idx];
    for (unsigned int cx = 0; cx < FONT_W; cx++) {
      for (unsigned int ry = 0; ry < FONT_H; ry++) {
        if ((col[cx] >> ry) & 1) {
          unsigned int bx = px + cx * 3u, by = py + ry * 3u;
          for (unsigned int dy = 0; dy < 3u; dy++)
            for (unsigned int dx = 0; dx < 3u; dx++)
              set_pixel_4g(bx + dx, by + dy);
        }
      }
    }
    px += UI_3X_CHAR_W;
  }
}

/* Draw string at 6x scale, tight: for big numbers (temp/hum/wind), slightly smaller than 8x. */
#define UI_6X_CHAR_W_TIGHT (FONT_W * 6u)     /* 36px per char */
static void draw_string_4g_scaled_6x_tight(unsigned int base_x, unsigned int base_y, const char *str) {
  if (!str) return;
  unsigned int px = base_x, py = base_y;
  while (*str) {
    int idx = char_to_index(*str++);
    const unsigned char *col = font_6x8[idx];
    for (unsigned int cx = 0; cx < FONT_W; cx++) {
      for (unsigned int ry = 0; ry < FONT_H; ry++) {
        if ((col[cx] >> ry) & 1) {
          unsigned int bx = px + cx * 6u, by = py + ry * 6u;
          for (unsigned int dy = 0; dy < 6u; dy++)
            for (unsigned int dx = 0; dx < 6u; dx++)
              set_pixel_4g(bx + dx, by + dy);
        }
      }
    }
    px += UI_6X_CHAR_W_TIGHT;
  }
}

/* Draw string at 8x scale: each font pixel -> 8x8 block (same style as HELLO, solid). */
#define UI_8X_CHAR_W   ((FONT_W + 1u) * 8u)   /* 56px per char (with gap) */
#define UI_8X_CHAR_W_TIGHT (FONT_W * 8u)     /* 48px per char, no gap */
#define UI_8X_CHAR_H   (FONT_H * 8u)
static void draw_string_4g_scaled_8x(unsigned int base_x, unsigned int base_y, const char *str) {
  if (!str) return;
  unsigned int px = base_x, py = base_y;
  while (*str) {
    int idx = char_to_index(*str++);
    const unsigned char *col = font_6x8[idx];
    for (unsigned int cx = 0; cx < FONT_W; cx++) {
      for (unsigned int ry = 0; ry < FONT_H; ry++) {
        if ((col[cx] >> ry) & 1) {
          unsigned int bx = px + cx * 8u, by = py + ry * 8u;
          for (unsigned int dy = 0; dy < 8u; dy++)
            for (unsigned int dx = 0; dx < 8u; dx++)
              set_pixel_4g(bx + dx, by + dy);
        }
      }
    }
    px += UI_8X_CHAR_W;
  }
}
/* Same as above but 48px per character (tighter, for numbers). */
static void draw_string_4g_scaled_8x_tight(unsigned int base_x, unsigned int base_y, const char *str) {
  if (!str) return;
  unsigned int px = base_x, py = base_y;
  while (*str) {
    int idx = char_to_index(*str++);
    const unsigned char *col = font_6x8[idx];
    for (unsigned int cx = 0; cx < FONT_W; cx++) {
      for (unsigned int ry = 0; ry < FONT_H; ry++) {
        if ((col[cx] >> ry) & 1) {
          unsigned int bx = px + cx * 8u, by = py + ry * 8u;
          for (unsigned int dy = 0; dy < 8u; dy++)
            for (unsigned int dx = 0; dx < 8u; dx++)
              set_pixel_4g(bx + dx, by + dy);
        }
      }
    }
    px += UI_8X_CHAR_W_TIGHT;
  }
}

/* Blit 1-bit image (w x h) into 4G buffer at (base_x, base_y). Row stride = (w+7)/8 bytes. */
static void blit_1bit_to_4g(const unsigned char *bitmap, unsigned int w, unsigned int h,
                            unsigned int base_x, unsigned int base_y) {
  const unsigned int row_stride = (w + 7u) / 8u;
  for (unsigned int y = 0; y < h; y++) {
    for (unsigned int x = 0; x < w; x++) {
      unsigned int byte_ix = y * row_stride + (x / 8u);
      unsigned int bit_ix  = 7 - (x % 8u);
      if ((bitmap[byte_ix] >> bit_ix) & 1)
        set_pixel_4g(base_x + x, base_y + y);
    }
  }
}

/* 4G icon: column-major, (icon_w x icon_h). Blit into 4G buffer at (base_x, base_y). */
static void blit_4g_icon_to_4g(const unsigned char *icon_4g, unsigned int base_x, unsigned int base_y,
                               unsigned int icon_w, unsigned int icon_h) {
  unsigned int bytes_per_col = ((icon_h - 1u) / 8u) * 2u + ((icon_h - 1u) % 8u) / 4u + 1u;
  for (unsigned int ix = 0; ix < icon_w; ix++) {
    for (unsigned int col_byte = 0; col_byte < bytes_per_col; col_byte++) {
#ifdef __AVR__
      unsigned char b = (unsigned char)pgm_read_byte(icon_4g + ix * bytes_per_col + col_byte);
#else
      unsigned char b = icon_4g[ix * bytes_per_col + col_byte];
#endif
      for (unsigned int k = 0; k < 4u; k++) {
        unsigned int iy = col_byte * 4u + k;
        if (iy >= icon_h) break;
        unsigned int value = (b >> (6 - k * 2)) & 3u;
        set_pixel_4g_value(base_x + ix, base_y + iy, value);
      }
    }
  }
}

/* Read one pixel (0..3) from 4G icon, column-major. */
static unsigned int get_4g_icon_pixel(const unsigned char *icon_4g, unsigned int icon_w, unsigned int icon_h,
                                      unsigned int px, unsigned int py) {
  if (px >= icon_w || py >= icon_h) return 0u;
  unsigned int bytes_per_col = ((icon_h - 1u) / 8u) * 2u + ((icon_h - 1u) % 8u) / 4u + 1u;
  unsigned int byte_ix = px * bytes_per_col + (py / 4u);
  unsigned int shift = (unsigned int)(6 - (py % 4u) * 2);
#ifdef __AVR__
  unsigned char b = (unsigned char)pgm_read_byte(icon_4g + byte_ix);
#else
  unsigned char b = icon_4g[byte_ix];
#endif
  return (unsigned int)(b >> shift) & 3u;
}

/* Blit 4G icon at half size (nearest-neighbor downscale) into 4G buffer. */
static void blit_4g_icon_to_4g_half(const unsigned char *icon_4g, unsigned int base_x, unsigned int base_y,
                                   unsigned int icon_w, unsigned int icon_h) {
  unsigned int dw = icon_w / 2u, dh = icon_h / 2u;
  for (unsigned int dy = 0; dy < dh; dy++) {
    for (unsigned int dx = 0; dx < dw; dx++) {
      unsigned int v = get_4g_icon_pixel(icon_4g, icon_w, icon_h, dx * 2u, dy * 2u);
      set_pixel_4g_value(base_x + dx, base_y + dy, v);
    }
  }
}

/* Blit 4G icon to fit inside box_size x box_size preserving aspect ratio, centered. */
static void blit_4g_icon_to_4g_fit(const unsigned char *icon_4g, unsigned int base_x, unsigned int base_y,
                                   unsigned int icon_w, unsigned int icon_h, unsigned int box_size) {
  unsigned int max_dim = (icon_w >= icon_h) ? icon_w : icon_h;
  if (max_dim == 0u) return;
  unsigned int dest_w = (icon_w * box_size) / max_dim;
  unsigned int dest_h = (icon_h * box_size) / max_dim;
  if (dest_w == 0u || dest_h == 0u) return;
  unsigned int ox = (box_size - dest_w) / 2u;
  unsigned int oy = (box_size - dest_h) / 2u;
  for (unsigned int dy = 0; dy < dest_h; dy++) {
    for (unsigned int dx = 0; dx < dest_w; dx++) {
      unsigned int sx = (dx * icon_w) / dest_w;
      unsigned int sy = (dy * icon_h) / dest_h;
      unsigned int v = get_4g_icon_pixel(icon_4g, icon_w, icon_h, sx, sy);
      set_pixel_4g_value(base_x + ox + dx, base_y + oy + dy, v);
    }
  }
}

/* Blit 1-bit image at half size (nearest-neighbor) into 4G buffer. */
static void blit_1bit_to_4g_half(const unsigned char *bitmap, unsigned int w, unsigned int h,
                                 unsigned int base_x, unsigned int base_y) {
  unsigned int dw = w / 2u, dh = h / 2u;
  const unsigned int row_stride = (w + 7u) / 8u;
  for (unsigned int dy = 0; dy < dh; dy++) {
    for (unsigned int dx = 0; dx < dw; dx++) {
      unsigned int sx = dx * 2u, sy = dy * 2u;
      unsigned int byte_ix = sy * row_stride + (sx / 8u);
      unsigned int bit_ix  = 7 - (sx % 8u);
      if ((bitmap[byte_ix] >> bit_ix) & 1)
        set_pixel_4g(base_x + dx, base_y + dy);
    }
  }
}

/* Blit 1-bit image to fit inside box_size x box_size preserving aspect ratio, centered. */
static void blit_1bit_to_4g_fit(const unsigned char *bitmap, unsigned int w, unsigned int h,
                                unsigned int base_x, unsigned int base_y, unsigned int box_size) {
  unsigned int max_dim = (w >= h) ? w : h;
  if (max_dim == 0u) return;
  unsigned int dest_w = (w * box_size) / max_dim;
  unsigned int dest_h = (h * box_size) / max_dim;
  if (dest_w == 0u || dest_h == 0u) return;
  unsigned int ox = (box_size - dest_w) / 2u;
  unsigned int oy = (box_size - dest_h) / 2u;
  const unsigned int row_stride = (w + 7u) / 8u;
  for (unsigned int dy = 0; dy < dest_h; dy++) {
    for (unsigned int dx = 0; dx < dest_w; dx++) {
      unsigned int sx = (dx * w) / dest_w;
      unsigned int sy = (dy * h) / dest_h;
      unsigned int byte_ix = sy * row_stride + (sx / 8u);
      unsigned int bit_ix  = 7 - (sx % 8u);
      if ((bitmap[byte_ix] >> bit_ix) & 1)
        set_pixel_4g(base_x + ox + dx, base_y + oy + dy);
    }
  }
}

/* Copy 1-bit image (src_w x src_h) into buffer (dst_w x dst_h) at offset (ox, oy). */
static void copy_1bit_into_buf(unsigned char *dst, unsigned int dst_w, unsigned int dst_h,
                              const unsigned char *src, unsigned int src_w, unsigned int src_h,
                              unsigned int ox, unsigned int oy) {
  unsigned int src_stride = (src_w + 7u) / 8u;
  unsigned int dst_stride = (dst_w + 7u) / 8u;
  for (unsigned int y = 0; y < src_h; y++) {
    for (unsigned int x = 0; x < src_w; x++) {
      unsigned int src_byte_ix = y * src_stride + (x / 8u);
      unsigned int src_bit = 7 - (x % 8u);
      unsigned char byte_val;
#ifdef __AVR__
      byte_val = (unsigned char)pgm_read_byte(src + src_byte_ix);
#else
      byte_val = src[src_byte_ix];
#endif
      int on = (byte_val >> src_bit) & 1;
      if (!on) continue;
      unsigned int dx = ox + x, dy = oy + y;
      if (dx >= dst_w || dy >= dst_h) continue;
      unsigned int dst_byte_ix = dy * dst_stride + (dx / 8u);
      unsigned int dst_bit = 7 - (dx % 8u);
      dst[dst_byte_ix] |= (unsigned char)(1 << dst_bit);
    }
  }
}

static void format_temp(char *out, size_t out_size, float temp_c) {
  if (temp_c < -99.9f || temp_c > 99.9f)
    snprintf(out, out_size, "---");
  else
    snprintf(out, out_size, "%.1f%cC", (double)temp_c, (char)0xB0);  /* °C */
}

/* Number only for temp (draw °C separately in smaller font). */
static void format_temp_number(char *out, size_t out_size, float temp_c) {
  if (temp_c < -99.9f || temp_c > 99.9f)
    snprintf(out, out_size, "---");
  else
    snprintf(out, out_size, "%.1f", (double)temp_c);
}

static void format_humidity(char *out, size_t out_size, float humidity_percent) {
  if (humidity_percent < 0 || humidity_percent > 100)
    snprintf(out, out_size, "--%%");
  else
    snprintf(out, out_size, "%.0f%%", (double)humidity_percent);
}

/* Number only for humidity (draw % separately in smaller font). */
static void format_humidity_number(char *out, size_t out_size, float humidity_percent) {
  if (humidity_percent < 0 || humidity_percent > 100)
    snprintf(out, out_size, "---");
  else
    snprintf(out, out_size, "%.0f", (double)humidity_percent);
}

static void format_temp_range(char *out, size_t out_size, float tmin_c, float tmax_c) {
  char a[16], b[16];
  format_temp(a, sizeof(a), tmin_c);
  format_temp(b, sizeof(b), tmax_c);
  snprintf(out, out_size, "%s-%s", a, b);
}

/* Forecast temps as integers: "min°C-max°C" (e.g. "-2°C-3°C"). */
static void format_temp_range_int(char *out, size_t out_size, int tmin_c, int tmax_c) {
  if (tmin_c < -99 || tmin_c > 99 || tmax_c < -99 || tmax_c > 99)
    snprintf(out, out_size, "---");
  else
    snprintf(out, out_size, "%d%cC/%d%cC", tmin_c, (char)0xB0, tmax_c, (char)0xB0);
}

/* Single temp for forecast: "−2°" or "3°" (degree only, no C). */
/* Forecast min/max: number only (no ° symbol). */
static void format_temp_int_degree(char *out, size_t out_size, int temp_c) {
  if (temp_c < -99 || temp_c > 99)
    snprintf(out, out_size, "---");
  else
    snprintf(out, out_size, "%d", temp_c);
}

/* Draw a circle outline (midpoint). cx,cy center; r radius. */
static void draw_circle(unsigned char *buf, unsigned int w, unsigned int h,
                        int cx, int cy, int r) {
  int x = r, y = 0, err = 1 - r;
  while (x >= y) {
    set_pixel(buf, w, h, (unsigned int)(cx + x), (unsigned int)(cy + y));
    set_pixel(buf, w, h, (unsigned int)(cx - x), (unsigned int)(cy + y));
    set_pixel(buf, w, h, (unsigned int)(cx + x), (unsigned int)(cy - y));
    set_pixel(buf, w, h, (unsigned int)(cx - x), (unsigned int)(cy - y));
    set_pixel(buf, w, h, (unsigned int)(cx + y), (unsigned int)(cy + x));
    set_pixel(buf, w, h, (unsigned int)(cx - y), (unsigned int)(cy + x));
    set_pixel(buf, w, h, (unsigned int)(cx + y), (unsigned int)(cy - x));
    set_pixel(buf, w, h, (unsigned int)(cx - y), (unsigned int)(cy - x));
    y++;
    if (err <= 0) { err += 2 * y + 1; }
    else { x--; err += 2 * (y - x) + 1; }
  }
}

/* Draw filled circle. */
static void fill_circle(unsigned char *buf, unsigned int w, unsigned int h,
                        int cx, int cy, int r) {
  for (int dy = -r; dy <= r; dy++) {
    int q = r * r - dy * dy;
    int dx = (q <= 0) ? 0 : (int)(sqrtf((float)q) + 0.5f);
    for (int x = cx - dx; x <= cx + dx; x++)
      set_pixel(buf, w, h, (unsigned int)x, (unsigned int)(cy + dy));
  }
}

/* Horizontal line. */
static void hline(unsigned char *buf, unsigned int w, unsigned int h,
                  unsigned int x0, unsigned int x1, unsigned int y) {
  for (; x0 <= x1; x0++) set_pixel(buf, w, h, x0, y);
}

/* Vertical line. */
static void vline(unsigned char *buf, unsigned int w, unsigned int h,
                  unsigned int x, unsigned int y0, unsigned int y1) {
  for (; y0 <= y1; y0++) set_pixel(buf, w, h, x, y0);
}

static void draw_icon_simple_rays(unsigned char *buf, unsigned int w, unsigned int h) {
  int cx = (int)w / 2, cy = (int)h / 2, r = (int)h / 4;
  draw_circle(buf, w, h, cx, cy, r);
  hline(buf, w, h, 0, (unsigned int)(w - 1), (unsigned int)cy);
  vline(buf, w, h, (unsigned int)cx, 0, (unsigned int)(h - 1));
  hline(buf, w, h, (unsigned int)(cx - r), (unsigned int)(cx + r), (unsigned int)(cy - r / 2));
  hline(buf, w, h, (unsigned int)(cx - r), (unsigned int)(cx + r), (unsigned int)(cy + r / 2));
  vline(buf, w, h, (unsigned int)(cx - r / 2), (unsigned int)(cy - r), (unsigned int)(cy + r));
  vline(buf, w, h, (unsigned int)(cx + r / 2), (unsigned int)(cy - r), (unsigned int)(cy + r));
}

static void draw_icon_cloud(unsigned char *buf, unsigned int w, unsigned int h) {
  fill_circle(buf, w, h, (int)w/2 - 8, (int)h/2 + 4, 12);
  fill_circle(buf, w, h, (int)w/2 + 8, (int)h/2 + 4, 12);
  fill_circle(buf, w, h, (int)w/2, (int)h/2 - 4, 14);
  hline(buf, w, h, 4, (unsigned int)(w - 5), (unsigned int)(h/2 + 14));
}

static void draw_icon_rain(unsigned char *buf, unsigned int w, unsigned int h) {
  draw_icon_cloud(buf, w, h);
  for (int i = 0; i < 5; i++) {
    unsigned int x = 12 + (unsigned int)i * 14;
    vline(buf, w, h, x, (unsigned int)(h/2 + 18), (unsigned int)(h - 4));
    vline(buf, w, h, x + 1, (unsigned int)(h/2 + 18), (unsigned int)(h - 4));
  }
}

static void draw_icon_snow(unsigned char *buf, unsigned int w, unsigned int h) {
  draw_icon_cloud(buf, w, h);
  int cx = (int)w / 2, cy = (int)h / 2 + 20;
  for (int i = 0; i < 3; i++) {
    int dx = 8 * (i - 1);
    vline(buf, w, h, (unsigned int)(cx + dx), (unsigned int)(cy - 8), (unsigned int)(cy + 8));
    hline(buf, w, h, (unsigned int)(cx + dx - 4), (unsigned int)(cx + dx + 4), (unsigned int)cy);
    int x = cx + dx, y = cy;
    set_pixel(buf, w, h, (unsigned int)(x - 3), (unsigned int)(y - 3));
    set_pixel(buf, w, h, (unsigned int)(x + 3), (unsigned int)(y - 3));
    set_pixel(buf, w, h, (unsigned int)(x - 3), (unsigned int)(y + 3));
    set_pixel(buf, w, h, (unsigned int)(x + 3), (unsigned int)(y + 3));
  }
}

static void draw_icon_fog(unsigned char *buf, unsigned int w, unsigned int h) {
  draw_icon_cloud(buf, w, h);
  for (int i = 0; i < 4; i++) {
    unsigned int y = (unsigned int)(h/2 + 20 + i * 10);
    hline(buf, w, h, 8, (unsigned int)(w - 9), y);
  }
}

static void draw_icon_thunder(unsigned char *buf, unsigned int w, unsigned int h) {
  draw_icon_cloud(buf, w, h);
  int cx = (int)w / 2;
  /* zigzag lightning */
  vline(buf, w, h, (unsigned int)(cx + 4), (unsigned int)(h/2 + 10), (unsigned int)(h/2 + 28));
  for (int y = h/2 + 28; y <= h/2 + 36; y++) set_pixel(buf, w, h, (unsigned int)(cx + 4 - (y - (h/2+28))), (unsigned int)y);
  vline(buf, w, h, (unsigned int)(cx - 4), (unsigned int)(h/2 + 36), (unsigned int)(h - 8));
}

static void draw_weather_icon(unsigned char *buf, unsigned int w, unsigned int h,
                              epd_ui_weather_icon_t icon) {
  memset(buf, 0, (w * h) / 8);
  switch (icon) {
    case EPD_UI_ICON_CLEAR:
      draw_icon_simple_rays(buf, w, h);
      break;
    case EPD_UI_ICON_PARTLY_CLOUDY:
      draw_icon_simple_rays(buf, w, h);
      draw_icon_cloud(buf, w, h);
      break;
    case EPD_UI_ICON_CLOUDY:
      draw_icon_cloud(buf, w, h);
      break;
    case EPD_UI_ICON_FOG:
      draw_icon_fog(buf, w, h);
      break;
    case EPD_UI_ICON_RAIN:
      draw_icon_rain(buf, w, h);
      break;
    case EPD_UI_ICON_SNOW:
      draw_icon_snow(buf, w, h);
      break;
    case EPD_UI_ICON_THUNDERSTORM:
      draw_icon_thunder(buf, w, h);
      break;
    default:
      draw_icon_cloud(buf, w, h);
      break;
  }
}

epd_ui_weather_icon_t epd_ui_weather_code_to_icon(int wmo_code) {
  if (wmo_code == 0 || wmo_code == 1) return EPD_UI_ICON_CLEAR;       /* Clear, Mainly clear */
  if (wmo_code == 2) return EPD_UI_ICON_PARTLY_CLOUDY;                  /* Partly cloudy */
  if (wmo_code == 3) return EPD_UI_ICON_CLOUDY;                        /* Cloudy */
  if (wmo_code == 45 || wmo_code == 48) return EPD_UI_ICON_FOG;        /* Fog, Rime fog */
  if (wmo_code >= 51 && wmo_code <= 67) return EPD_UI_ICON_RAIN;       /* Drizzle, rain, freezing rain */
  if (wmo_code >= 71 && wmo_code <= 77) return EPD_UI_ICON_SNOW;       /* Snow, snow grains */
  if (wmo_code >= 95 && wmo_code <= 99) return EPD_UI_ICON_THUNDERSTORM; /* Thunderstorm */
  if (wmo_code >= 85 && wmo_code <= 86) return EPD_UI_ICON_SNOW;       /* Snow showers */
  if (wmo_code >= 80 && wmo_code <= 82) return EPD_UI_ICON_RAIN;      /* Showers */
  return EPD_UI_ICON_CLOUDY;
}

void epd_ui_draw_indoor_temp(float temp_c) {
  static unsigned char buf[TEMP_BUF_SIZE];
  memset(buf, 0, sizeof(buf));
  char str[16];
  format_temp(str, sizeof(str), temp_c);
  draw_string(buf, EPD_UI_TEMP_REGION_W, EPD_UI_TEMP_REGION_H, 0, 8, str);
  EPD_Dis_Part(EPD_UI_INDOOR_TEMP_X, EPD_UI_INDOOR_TEMP_Y, buf, EPD_UI_TEMP_REGION_H, EPD_UI_TEMP_REGION_W);
}

void epd_ui_draw_outdoor_temp(float temp_c) {
  static unsigned char buf[TEMP_BUF_SIZE];
  memset(buf, 0, sizeof(buf));
  char str[16];
  format_temp(str, sizeof(str), temp_c);
  draw_string(buf, EPD_UI_TEMP_REGION_W, EPD_UI_TEMP_REGION_H, 0, 8, str);
  EPD_Dis_Part(EPD_UI_OUTDOOR_TEMP_X, EPD_UI_OUTDOOR_TEMP_Y, buf, EPD_UI_TEMP_REGION_H, EPD_UI_TEMP_REGION_W);
}

void epd_ui_draw_indoor_humidity(float humidity_percent) {
  static unsigned char buf[HUMID_BUF_SIZE];
  memset(buf, 0, sizeof(buf));
  char str[8];
  format_humidity(str, sizeof(str), humidity_percent);
  draw_string(buf, EPD_UI_HUMID_REGION_W, EPD_UI_HUMID_REGION_H, 0, 8, str);
  EPD_Dis_Part(EPD_UI_INDOOR_HUMID_X, EPD_UI_INDOOR_HUMID_Y, buf, EPD_UI_HUMID_REGION_H, EPD_UI_HUMID_REGION_W);
}

void epd_ui_draw_outdoor_humidity(float humidity_percent) {
  static unsigned char buf[HUMID_BUF_SIZE];
  memset(buf, 0, sizeof(buf));
  char str[8];
  format_humidity(str, sizeof(str), humidity_percent);
  draw_string(buf, EPD_UI_HUMID_REGION_W, EPD_UI_HUMID_REGION_H, 0, 8, str);
  EPD_Dis_Part(EPD_UI_OUTDOOR_HUMID_X, EPD_UI_OUTDOOR_HUMID_Y, buf, EPD_UI_HUMID_REGION_H, EPD_UI_HUMID_REGION_W);
}

/* Representative WMO code per icon type (each has a bitmap). */
static int icon_to_wmo(epd_ui_weather_icon_t icon) {
  switch (icon) {
    case EPD_UI_ICON_CLEAR:         return 0;
    case EPD_UI_ICON_PARTLY_CLOUDY: return 2;
    case EPD_UI_ICON_CLOUDY:       return 3;
    case EPD_UI_ICON_FOG:          return 45;
    case EPD_UI_ICON_RAIN:         return 61;
    case EPD_UI_ICON_SNOW:         return 71;
    case EPD_UI_ICON_THUNDERSTORM: return 95;
    default:                       return 3;
  }
}

/* Fill frame-sized 1-bit buffer: center icon (by WMO) or fallback to drawn icon. */
static void fill_icon_buf_64x64(unsigned char *buf, int wmo_code, epd_ui_weather_icon_t fallback_icon) {
  const unsigned char *bitmap = weather_icon_bitmap_by_wmo(wmo_code);
  if (bitmap) {
    unsigned int icon_w = weather_icon_1bit_width_by_wmo(wmo_code);
    unsigned int icon_h = weather_icon_1bit_height_by_wmo(wmo_code);
    unsigned int ox = (EPD_UI_ICON_REGION_W - icon_w) / 2u;
    unsigned int oy = (EPD_UI_ICON_REGION_H - icon_h) / 2u;
    memset(buf, 0, (size_t)ICON_BUF_SIZE);
    copy_1bit_into_buf(buf, EPD_UI_ICON_REGION_W, EPD_UI_ICON_REGION_H,
                      bitmap, icon_w, icon_h, ox, oy);
  } else {
    draw_weather_icon(buf, EPD_UI_ICON_REGION_W, EPD_UI_ICON_REGION_H, fallback_icon);
  }
}

void epd_ui_draw_outdoor_icon(epd_ui_weather_icon_t icon) {
  static unsigned char buf[ICON_BUF_SIZE];
  int wmo = icon_to_wmo(icon);
  fill_icon_buf_64x64(buf, wmo, icon);
  EPD_Dis_Part(EPD_UI_OUTDOOR_ICON_X, EPD_UI_OUTDOOR_ICON_Y, buf, EPD_UI_ICON_REGION_H, EPD_UI_ICON_PART_LINE);
}

/* Scale icon 2x (e.g. 106x106 -> 212x212); source/dest use row-stride layout. */
static void scale_icon_2x(const unsigned char *src, unsigned char *dst) {
  const unsigned int sw = EPD_UI_ICON_REGION_W, sh = EPD_UI_ICON_REGION_H;
  const unsigned int dw = EPD_UI_LARGE_ICON_W, dh = EPD_UI_LARGE_ICON_H;
  const unsigned int src_stride = ICON_ROW_STRIDE, dst_stride = LARGE_ICON_ROW_STRIDE;
  memset(dst, 0, (size_t)LARGE_ICON_BUF_SIZE);
  for (unsigned int sy = 0; sy < sh; sy++) {
    for (unsigned int sx = 0; sx < sw; sx++) {
      unsigned int byte_ix = sy * src_stride + (sx / 8u);
      unsigned int bit_ix  = 7 - (sx % 8u);
      int on = (src[byte_ix] >> bit_ix) & 1;
      if (!on) continue;
      unsigned int dx = sx * 2u, dy = sy * 2u;
      for (unsigned int oy = 0; oy < 2u; oy++) {
        for (unsigned int ox = 0; ox < 2u; ox++) {
          unsigned int byte_o = (dy + oy) * dst_stride + ((dx + ox) / 8u);
          unsigned int bit_o  = 7 - ((dx + ox) % 8u);
          dst[byte_o] |= (unsigned char)(1 << bit_o);
        }
      }
    }
  }
}

void epd_ui_draw_large_weather_icon(epd_ui_weather_icon_t icon) {
  static unsigned char small_buf[ICON_BUF_SIZE];
  static unsigned char large_buf[LARGE_ICON_BUF_SIZE];
  int wmo = icon_to_wmo(icon);
  fill_icon_buf_64x64(small_buf, wmo, icon);
  scale_icon_2x(small_buf, large_buf);
  EPD_Dis_Part(EPD_UI_LARGE_ICON_X, EPD_UI_LARGE_ICON_Y, large_buf, EPD_UI_LARGE_ICON_H, EPD_UI_LARGE_ICON_PART_LINE);
}

void epd_ui_draw_battery(float percent) {
  static unsigned char buf[BATTERY_BUF_SIZE];
  memset(buf, 0, sizeof(buf));
  char str[8];
  if (percent < 0 || percent > 100)
    snprintf(str, sizeof(str), "--%%");
  else
    snprintf(str, sizeof(str), "%.0f%%", (double)percent);
  draw_string(buf, EPD_UI_BATTERY_REGION_W, EPD_UI_BATTERY_REGION_H, 0, 8, str);
  EPD_Dis_Part(EPD_UI_BATTERY_X, EPD_UI_BATTERY_Y, buf, EPD_UI_BATTERY_REGION_H, EPD_UI_BATTERY_REGION_W);
}

void epd_ui_draw_status(const char *str) {
  static unsigned char buf[STATUS_BUF_SIZE];
  memset(buf, 0, sizeof(buf));
  if (str) draw_string(buf, EPD_UI_STATUS_REGION_W, EPD_UI_STATUS_REGION_H, 0, 8, str);
  EPD_Dis_Part(EPD_UI_STATUS_X, EPD_UI_STATUS_Y, buf, EPD_UI_STATUS_REGION_H, EPD_UI_STATUS_REGION_W);
}

void epd_ui_draw_status2(const char *str) {
  static unsigned char buf[STATUS_BUF_SIZE];
  memset(buf, 0, sizeof(buf));
  if (str) draw_string(buf, EPD_UI_STATUS_REGION_W, EPD_UI_STATUS_REGION_H, 0, 8, str);
  EPD_Dis_Part(EPD_UI_STATUS2_X, EPD_UI_STATUS2_Y, buf, EPD_UI_STATUS_REGION_H, EPD_UI_STATUS_REGION_W);
}

/* -------- New layout-aligned partial redraw helpers -------- */

/* Convert one 4G pixel to 2-bit grayscale value (0..3). */
static unsigned int get_pixel_4g_value(unsigned int x, unsigned int y) {
  if (epd_ui_4g_flip_y) {
    y = (EPD_HEIGHT - 1u) - y;
  }
  if (x >= EPD_WIDTH || y >= EPD_HEIGHT) return 0u;
  unsigned int byte_ix;
  unsigned int nibble_shift;
#if EPD_UI_4G_COLUMN_MAJOR
  unsigned int col_byte = (y / 8u) * 2u + ((y % 8u) / 4u);
  byte_ix = x * EPD_4G_BYTES_PER_COL + col_byte;
  nibble_shift = (y % 4u);
#else
  unsigned int group = y * 60u + (x / 8u);
  byte_ix = group * 2u + ((x % 8u) / 4u);
  nibble_shift = (x % 4u);
#endif
  return (unsigned int)((epd_4g_buffer[byte_ix] >> (6u - nibble_shift * 2u)) & 0x3u);
}

/* Pack rectangular region from 4G buffer into 1-bit buffer and push with EPD_Dis_Part.
 * NOTE: In this panel driver, partial-update addressing uses swapped axes:
 *   - PART_LINE   maps to panel X (0..799)  -> logical Y
 *   - PART_COLUMN maps to panel Y (0..479)  -> logical X
 * So we transpose logical coordinates before issuing EPD_Dis_Part.
 */
static void push_4g_region_as_1bit(unsigned int x, unsigned int y, unsigned int w, unsigned int h) {
  static unsigned char part_buf[EPD_ARRAY];  /* max full-screen 1-bit buffer */
  static unsigned char clear_buf[EPD_ARRAY]; /* same shape, forced white */
  /* Driver aligns panel-X to 8px; panel-X corresponds to logical Y. */
  unsigned int y_aligned = y - (y % 8u);
  unsigned int y_pad = y - y_aligned;            /* top blank pixels in logical region */
  unsigned int line_aligned = ((h + y_pad + 7u) / 8u) * 8u;  /* panel-X span */
  unsigned int row_stride = line_aligned / 8u;                /* bytes per panel row */
  unsigned int total_bytes = row_stride * w;                  /* rows == logical width */
  if (total_bytes > EPD_ARRAY) return;
  /* Partial 1-bit polarity: 1 = white, 0 = black. */
  memset(clear_buf, 0xFF, total_bytes);
  memset(part_buf, 0xFF, total_bytes);

  /* Row-major in panel space: row=panel-Y(logical X), col=panel-X(logical Y). */
  for (unsigned int row = 0; row < w; row++) {
    for (unsigned int col = 0; col < line_aligned; col++) {
      int on = 0;
      if (col >= y_pad && col < y_pad + h) {
        /* Threshold gray: dark gray/black -> black in 1-bit partial update. */
        unsigned int logical_x = x + row;
        unsigned int logical_y = y + (col - y_pad);
        unsigned int v = get_pixel_4g_value(logical_x, logical_y);
        on = (v >= 2u);
      }
      if (on) {
        unsigned int byte_ix = row * row_stride + (col / 8u);
        unsigned int bit_ix = 7u - (col % 8u);
        part_buf[byte_ix] &= (unsigned char)(~(1u << bit_ix));
      }
    }
  }

  /* panel_x_start=logical_y, panel_y_start=logical_x */
  EPD_Dis_Part(y_aligned, x, clear_buf, w, line_aligned); /* clear region first */
  EPD_Dis_Part(y_aligned, x, part_buf, w, line_aligned);
}

/* Draw only time in full-screen 4G buffer, then push header region. */
void epd_ui_draw_time_header(const char *time_str) {
  memset(epd_4g_buffer, 0, sizeof(epd_4g_buffer));
  epd_ui_4g_flip_y = 1;
  if (time_str && time_str[0])
    draw_gfxfont_string_4g((int)EPD_UI_TIME_X, (int)EPD_UI_TIME_Y + 46, time_str,
                           &InterTempRegular32pt7b, 2u);
  push_4g_region_as_1bit(0u, 0u, EPD_WIDTH, EPD_UI_IN_TEMP_Y);
}

/* Draw only battery icon in full-screen 4G buffer, then push battery region. */
void epd_ui_draw_battery_header(float percent) {
  memset(epd_4g_buffer, 0, sizeof(epd_4g_buffer));
  epd_ui_4g_flip_y = 1;
  draw_battery_icon_4g(EPD_UI_BATTERY_ICON_X, EPD_UI_BATTERY_ICON_Y, percent);
  push_4g_region_as_1bit(EPD_UI_BATTERY_ICON_X, EPD_UI_BATTERY_ICON_Y,
                         EPD_UI_BATTERY_ICON_W + 2u, EPD_UI_BATTERY_ICON_H);
}

void epd_ui_draw_indoor_block(float indoor_temp_c, float indoor_humidity) {
  char str[48];
  memset(epd_4g_buffer, 0, sizeof(epd_4g_buffer));
  epd_ui_4g_flip_y = 1;

  format_temp_number(str, sizeof(str), indoor_temp_c);
  {
    unsigned int tw_num = gfxfont_string_width(str, &InterTempSemiBold72pt7b);
    unsigned int tw_deg = gfxfont_string_width(degree_c, &InterTempRegular32pt7b);
    unsigned int tw_unit = tw_deg + gfxfont_string_width("C", &InterTempRegular32pt7b);
    unsigned int total = tw_num + 2u + tw_unit;
    int tx = (int)((EPD_UI_RIGHT_EDGE > total) ? EPD_UI_RIGHT_EDGE - total : EPD_UI_MARGIN);
    int by = (int)EPD_UI_IN_TEMP_Y + 103;
    draw_gfxfont_string_4g(tx, by, str, &InterTempSemiBold72pt7b, 3u);
    draw_gfxfont_string_4g(tx + (int)tw_num + 2, by, degree_c, &InterTempRegular32pt7b, 3u);
    draw_gfxfont_string_4g(tx + (int)tw_num + 2 + (int)tw_deg, by, "C", &InterTempRegular32pt7b, 3u);
  }

  format_humidity_number(str, sizeof(str), indoor_humidity);
  {
    unsigned int tw_num = gfxfont_string_width(str, &InterTempSemiBold48pt7b);
    unsigned int tw_unit = gfxfont_string_width("%", &InterTempRegular32pt7b);
    unsigned int total = tw_num + 2u + tw_unit;
    int tx = (int)((EPD_UI_RIGHT_EDGE > total) ? EPD_UI_RIGHT_EDGE - total : EPD_UI_MARGIN);
    int by = (int)EPD_UI_IN_HUMID_Y + 68;
    draw_gfxfont_string_4g(tx, by, str, &InterTempSemiBold48pt7b, 2u);
    draw_gfxfont_string_4g(tx + (int)tw_num + 2, by, "%", &InterTempRegular32pt7b, 2u);
  }

  draw_gfxfont_string_4g((int)EPD_UI_IN_LABEL_X, (int)EPD_UI_IN_LABEL_Y + 28, "IN",
                         &SourceSansLabel22pt7b, 1u);
  draw_hline_4g(EPD_UI_MARGIN, 480u - EPD_UI_MARGIN - 1u, EPD_UI_SEPARATOR_Y);
  push_4g_region_as_1bit(0u, EPD_UI_IN_TEMP_Y, EPD_WIDTH, EPD_UI_SEPARATOR_Y - EPD_UI_IN_TEMP_Y + 1u);
}

void epd_ui_draw_outdoor_block(float outdoor_temp_c, float outdoor_humidity, int wmo_weather_code) {
  char str[48];
  memset(epd_4g_buffer, 0, sizeof(epd_4g_buffer));
  epd_ui_4g_flip_y = 1;

  draw_gfxfont_string_4g((int)EPD_UI_OUT_LABEL_X, (int)EPD_UI_OUT_LABEL_Y + 28, "OUT",
                         &SourceSansLabel22pt7b, 1u);

  const unsigned char *icon_4g = weather_icon_4g_by_wmo(wmo_weather_code);
  if (icon_4g) {
    unsigned int iw = weather_icon_4g_width_by_wmo(wmo_weather_code);
    unsigned int ih = weather_icon_4g_height_by_wmo(wmo_weather_code);
    unsigned int ox = (WEATHER_ICON_FRAME_W - iw) / 2u;
    unsigned int oy = (WEATHER_ICON_FRAME_H - ih) / 2u;
    blit_4g_icon_to_4g(icon_4g, EPD_UI_OUT_ICON_X + ox, EPD_UI_OUT_ICON_Y + oy, iw, ih);
  } else {
    static unsigned char icon_buf[ICON_BUF_SIZE];
    epd_ui_weather_icon_t fallback_icon = epd_ui_weather_code_to_icon(wmo_weather_code);
    fill_icon_buf_64x64(icon_buf, wmo_weather_code, fallback_icon);
    blit_1bit_to_4g(icon_buf, EPD_UI_ICON_REGION_W, EPD_UI_ICON_REGION_H, EPD_UI_OUT_ICON_X, EPD_UI_OUT_ICON_Y);
  }

  format_temp_number(str, sizeof(str), outdoor_temp_c);
  {
    unsigned int tw_num = gfxfont_string_width(str, &InterTempSemiBold72pt7b);
    unsigned int tw_deg = gfxfont_string_width(degree_c, &InterTempRegular32pt7b);
    unsigned int tw_unit = tw_deg + gfxfont_string_width("C", &InterTempRegular32pt7b);
    unsigned int total = tw_num + 2u + tw_unit;
    int tx = (int)((EPD_UI_RIGHT_EDGE > total) ? EPD_UI_RIGHT_EDGE - (int)total : (int)EPD_UI_OUT_DATA_X);
    int by = (int)EPD_UI_OUT_TEMP_Y + 103;
    draw_gfxfont_string_4g(tx, by, str, &InterTempSemiBold72pt7b, 3u);
    draw_gfxfont_string_4g(tx + (int)tw_num + 2, by, degree_c, &InterTempRegular32pt7b, 3u);
    draw_gfxfont_string_4g(tx + (int)tw_num + 2 + (int)tw_deg, by, "C", &InterTempRegular32pt7b, 3u);
  }

  format_humidity_number(str, sizeof(str), outdoor_humidity);
  {
    unsigned int tw_num = gfxfont_string_width(str, &InterTempSemiBold48pt7b);
    unsigned int tw_unit = gfxfont_string_width("%", &InterTempRegular32pt7b);
    unsigned int total = tw_num + 2u + tw_unit;
    int tx = (int)((EPD_UI_RIGHT_EDGE > total) ? EPD_UI_RIGHT_EDGE - (int)total : (int)EPD_UI_OUT_DATA_X);
    int by = (int)EPD_UI_OUT_HUMID_Y + 68;
    draw_gfxfont_string_4g(tx, by, str, &InterTempSemiBold48pt7b, 2u);
    draw_gfxfont_string_4g(tx + (int)tw_num + 2, by, "%", &InterTempRegular32pt7b, 2u);
  }

  unsigned int y0 = EPD_UI_OUT_LABEL_Y;
  unsigned int y1 = EPD_UI_OUT_ICON_Y + EPD_UI_OUT_ICON_H;
  push_4g_region_as_1bit(0u, y0, EPD_WIDTH, y1 - y0 + 1u);
}

void epd_ui_draw_forecast_block(const epd_ui_forecast_day_t *forecast) {
  char str[48];
  memset(epd_4g_buffer, 0, sizeof(epd_4g_buffer));
  epd_ui_4g_flip_y = 1;

  static unsigned char icon_buf[ICON_BUF_SIZE];
  unsigned int cy = EPD_UI_FORECAST_CARDS_Y;
  unsigned int cx = EPD_UI_FORECAST_SIDE_MARGIN;
  for (int i = 0; i < 3; i++) {
    const char *date_str = (forecast && forecast[i].date && forecast[i].date[0]) ? forecast[i].date : "---";
    int wmo = (forecast && i < 3) ? forecast[i].wmo_code : 0;
    int tmin = (forecast && i < 3) ? forecast[i].temp_min_c : 0;
    int tmax = (forecast && i < 3) ? forecast[i].temp_max_c : 0;

    draw_rect_outline_4g(cx, cy, EPD_UI_FORECAST_CARD_W, EPD_UI_FORECAST_CARD_H);

    {
      unsigned int date_w = gfxfont_string_width(date_str, &AtkinsonForecast24pt7b);
      int tx = (int)cx + (int)(EPD_UI_FORECAST_CARD_W > date_w ? (EPD_UI_FORECAST_CARD_W - date_w) / 2u : 0u);
      draw_gfxfont_string_4g(tx, (int)cy + (int)EPD_UI_FORECAST_DATE_Y, date_str, &AtkinsonForecast24pt7b, 3u);
    }

    unsigned int icon_x = cx + EPD_UI_FORECAST_ICON_OFFSET_X;
    unsigned int icon_y = cy + EPD_UI_FORECAST_ICON_Y;
    const unsigned char *icon_4g = weather_icon_4g_by_wmo(wmo);
    if (icon_4g) {
      unsigned int iw = weather_icon_4g_width_by_wmo(wmo);
      unsigned int ih = weather_icon_4g_height_by_wmo(wmo);
      blit_4g_icon_to_4g_fit(icon_4g, icon_x, icon_y, iw, ih, EPD_UI_FORECAST_ICON_W);
    } else {
      epd_ui_weather_icon_t fallback = epd_ui_weather_code_to_icon(wmo);
      fill_icon_buf_64x64(icon_buf, wmo, fallback);
      blit_1bit_to_4g_fit(icon_buf, EPD_UI_ICON_REGION_W, EPD_UI_ICON_REGION_H, icon_x, icon_y, EPD_UI_FORECAST_ICON_W);
    }

    format_temp_int_degree(str, sizeof(str), tmax);
    {
      unsigned int w = gfxfont_string_width(str, &InterTempRegular28pt7b);
      int tx = (int)cx + (int)(EPD_UI_FORECAST_CARD_W > w ? (EPD_UI_FORECAST_CARD_W - w) / 2u : 0u);
      draw_gfxfont_string_4g(tx, (int)cy + (int)EPD_UI_FORECAST_TEMP_Y + 33, str, &InterTempRegular28pt7b, 3u);
    }
    draw_hline_4g(cx + (EPD_UI_FORECAST_CARD_W - EPD_UI_FORECAST_TEMP_LINE_LEN) / 2u,
                  cx + (EPD_UI_FORECAST_CARD_W - EPD_UI_FORECAST_TEMP_LINE_LEN) / 2u + EPD_UI_FORECAST_TEMP_LINE_LEN - 1u,
                  cy + EPD_UI_FORECAST_TEMP_LINE_Y);
    format_temp_int_degree(str, sizeof(str), tmin);
    {
      unsigned int w = gfxfont_string_width(str, &InterTempRegular28pt7b);
      int tx = (int)cx + (int)(EPD_UI_FORECAST_CARD_W > w ? (EPD_UI_FORECAST_CARD_W - w) / 2u : 0u);
      draw_gfxfont_string_4g(tx, (int)cy + (int)EPD_UI_FORECAST_TEMP_MAX_Y + 33, str, &InterTempRegular28pt7b, 3u);
    }

    cx += EPD_UI_FORECAST_CARD_W + EPD_UI_FORECAST_GAP;
  }

  unsigned int w = 3u * EPD_UI_FORECAST_CARD_W + 2u * EPD_UI_FORECAST_GAP;
  push_4g_region_as_1bit(EPD_UI_FORECAST_SIDE_MARGIN, EPD_UI_FORECAST_CARDS_Y, w, EPD_UI_FORECAST_CARD_H);
}

const unsigned char *epd_ui_build_demo_4g(float indoor_temp_c, float indoor_humidity,
  float outdoor_temp_c, float outdoor_humidity, int wmo_weather_code, float battery_percent,
  const char *status1, float wind_speed_m_s, const epd_ui_forecast_day_t *forecast) {
  memset(epd_4g_buffer, 0, sizeof(epd_4g_buffer));  /* white background */
  epd_ui_4g_flip_y = 1;  /* flip Y only: orientation matches HELLO, text L→R */

  char str[48];

  /* Header: time Inter Regular 32px Dark Gray (font includes colon) */
  if (status1 && status1[0])
    draw_gfxfont_string_4g((int)EPD_UI_TIME_X, (int)EPD_UI_TIME_Y + 46, status1,
                           &InterTempRegular32pt7b, 2u);
  draw_battery_icon_4g(EPD_UI_BATTERY_ICON_X, EPD_UI_BATTERY_ICON_Y, battery_percent);

  /* IN section: number in 72px/48px, °C in Inter Regular 32px (° at 0x2A); IN label Source Sans 22px */
  format_temp_number(str, sizeof(str), indoor_temp_c);
  { unsigned int tw_num = gfxfont_string_width(str, &InterTempSemiBold72pt7b);
    unsigned int tw_deg = gfxfont_string_width(degree_c, &InterTempRegular32pt7b);
    unsigned int tw_unit = tw_deg + gfxfont_string_width("C", &InterTempRegular32pt7b);
    unsigned int total = tw_num + 2u + tw_unit;
    int tx = (int)((EPD_UI_RIGHT_EDGE > total) ? EPD_UI_RIGHT_EDGE - total : EPD_UI_MARGIN);
    int by = (int)EPD_UI_IN_TEMP_Y + 103;
    draw_gfxfont_string_4g(tx, by, str, &InterTempSemiBold72pt7b, 3u);
    draw_gfxfont_string_4g(tx + (int)tw_num + 2, by, degree_c, &InterTempRegular32pt7b, 3u);
    draw_gfxfont_string_4g(tx + (int)tw_num + 2 + (int)tw_deg, by, "C", &InterTempRegular32pt7b, 3u); }
  format_humidity_number(str, sizeof(str), indoor_humidity);
  { unsigned int tw_num = gfxfont_string_width(str, &InterTempSemiBold48pt7b);
    unsigned int tw_unit = gfxfont_string_width("%", &InterTempRegular32pt7b);
    unsigned int total = tw_num + 2u + tw_unit;
    int tx = (int)((EPD_UI_RIGHT_EDGE > total) ? EPD_UI_RIGHT_EDGE - total : EPD_UI_MARGIN);
    int by = (int)EPD_UI_IN_HUMID_Y + 68;
    draw_gfxfont_string_4g(tx, by, str, &InterTempSemiBold48pt7b, 2u);
    draw_gfxfont_string_4g(tx + (int)tw_num + 2, by, "%", &InterTempRegular32pt7b, 2u); }
  draw_gfxfont_string_4g((int)EPD_UI_IN_LABEL_X, (int)EPD_UI_IN_LABEL_Y + 28, "IN",
                         &SourceSansLabel22pt7b, 1u);

  /* Separator */
  draw_hline_4g(EPD_UI_MARGIN, 480u - EPD_UI_MARGIN - 1u, EPD_UI_SEPARATOR_Y);

  /* OUT section: OUT label Source Sans 22px, icon, -3.2°C Inter SemiBold 72px, humidity 48px Dark Gray */
  draw_gfxfont_string_4g((int)EPD_UI_OUT_LABEL_X, (int)EPD_UI_OUT_LABEL_Y + 28, "OUT",
                         &SourceSansLabel22pt7b, 1u);
  const unsigned char *icon_4g = weather_icon_4g_by_wmo(wmo_weather_code);
  if (icon_4g) {
    unsigned int iw = weather_icon_4g_width_by_wmo(wmo_weather_code);
    unsigned int ih = weather_icon_4g_height_by_wmo(wmo_weather_code);
    unsigned int ox = (WEATHER_ICON_FRAME_W - iw) / 2u;
    unsigned int oy = (WEATHER_ICON_FRAME_H - ih) / 2u;
    blit_4g_icon_to_4g(icon_4g,
                       EPD_UI_OUT_ICON_X + ox, EPD_UI_OUT_ICON_Y + oy, iw, ih);
  } else {
    static unsigned char icon_buf[ICON_BUF_SIZE];
    epd_ui_weather_icon_t fallback_icon = epd_ui_weather_code_to_icon(wmo_weather_code);
    fill_icon_buf_64x64(icon_buf, wmo_weather_code, fallback_icon);
    blit_1bit_to_4g(icon_buf, EPD_UI_ICON_REGION_W, EPD_UI_ICON_REGION_H,
                    EPD_UI_OUT_ICON_X, EPD_UI_OUT_ICON_Y);
  }
  format_temp_number(str, sizeof(str), outdoor_temp_c);
  { unsigned int tw_num = gfxfont_string_width(str, &InterTempSemiBold72pt7b);
    unsigned int tw_deg = gfxfont_string_width(degree_c, &InterTempRegular32pt7b);
    unsigned int tw_unit = tw_deg + gfxfont_string_width("C", &InterTempRegular32pt7b);
    unsigned int total = tw_num + 2u + tw_unit;
    int tx = (int)((EPD_UI_RIGHT_EDGE > total) ? EPD_UI_RIGHT_EDGE - (int)total : (int)EPD_UI_OUT_DATA_X);
    int by = (int)EPD_UI_OUT_TEMP_Y + 103;
    draw_gfxfont_string_4g(tx, by, str, &InterTempSemiBold72pt7b, 3u);
    draw_gfxfont_string_4g(tx + (int)tw_num + 2, by, degree_c, &InterTempRegular32pt7b, 3u);
    draw_gfxfont_string_4g(tx + (int)tw_num + 2 + (int)tw_deg, by, "C", &InterTempRegular32pt7b, 3u); }
  format_humidity_number(str, sizeof(str), outdoor_humidity);
  { unsigned int tw_num = gfxfont_string_width(str, &InterTempSemiBold48pt7b);
    unsigned int tw_unit = gfxfont_string_width("%", &InterTempRegular32pt7b);
    unsigned int total = tw_num + 2u + tw_unit;
    int tx = (int)((EPD_UI_RIGHT_EDGE > total) ? EPD_UI_RIGHT_EDGE - (int)total : (int)EPD_UI_OUT_DATA_X);
    int by = (int)EPD_UI_OUT_HUMID_Y + 68;
    draw_gfxfont_string_4g(tx, by, str, &InterTempSemiBold48pt7b, 2u);
    draw_gfxfont_string_4g(tx + (int)tw_num + 2, by, "%", &InterTempRegular32pt7b, 2u); }

  /* FORECAST: 3 cards (no title); date Atkinson 24px, temps Inter 28px */
  {
    static unsigned char icon_buf[ICON_BUF_SIZE];
    unsigned int cy = EPD_UI_FORECAST_CARDS_Y;
    unsigned int cx = EPD_UI_FORECAST_SIDE_MARGIN;
    for (int i = 0; i < 3; i++) {
      const char *date_str = (forecast && forecast[i].date && forecast[i].date[0])
          ? forecast[i].date : "---";
      int wmo = (forecast && i < 3) ? forecast[i].wmo_code : 0;
      int tmin = (forecast && i < 3) ? forecast[i].temp_min_c : 0;
      int tmax = (forecast && i < 3) ? forecast[i].temp_max_c : 0;

      draw_rect_outline_4g(cx, cy, EPD_UI_FORECAST_CARD_W, EPD_UI_FORECAST_CARD_H);

      /* Date Atkinson 24px Black, centered in card */
      { unsigned int date_w = gfxfont_string_width(date_str, &AtkinsonForecast24pt7b);
        int tx = (int)cx + (int)(EPD_UI_FORECAST_CARD_W > date_w ? (EPD_UI_FORECAST_CARD_W - date_w) / 2u : 0u);
        draw_gfxfont_string_4g(tx, (int)cy + (int)EPD_UI_FORECAST_DATE_Y, date_str, &AtkinsonForecast24pt7b, 3u); }

      /* 70x70 box: icon scaled to fit inside preserving aspect ratio, centered */
      unsigned int icon_x = cx + EPD_UI_FORECAST_ICON_OFFSET_X;
      unsigned int icon_y = cy + EPD_UI_FORECAST_ICON_Y;
      const unsigned char *icon_4g = weather_icon_4g_by_wmo(wmo);
      if (icon_4g) {
        unsigned int iw = weather_icon_4g_width_by_wmo(wmo);
        unsigned int ih = weather_icon_4g_height_by_wmo(wmo);
        blit_4g_icon_to_4g_fit(icon_4g, icon_x, icon_y, iw, ih, EPD_UI_FORECAST_ICON_W);
      } else {
        epd_ui_weather_icon_t fallback = epd_ui_weather_code_to_icon(wmo);
        fill_icon_buf_64x64(icon_buf, wmo, fallback);
        blit_1bit_to_4g_fit(icon_buf, EPD_UI_ICON_REGION_W, EPD_UI_ICON_REGION_H, icon_x, icon_y, EPD_UI_FORECAST_ICON_W);
      }

      /* MAX on top, line, min below; Inter 28px Black, centered */
      format_temp_int_degree(str, sizeof(str), tmax);
      { unsigned int w = gfxfont_string_width(str, &InterTempRegular28pt7b);
        int tx = (int)cx + (int)(EPD_UI_FORECAST_CARD_W > w ? (EPD_UI_FORECAST_CARD_W - w) / 2u : 0u);
        draw_gfxfont_string_4g(tx, (int)cy + (int)EPD_UI_FORECAST_TEMP_Y + 33, str, &InterTempRegular28pt7b, 3u); }
      draw_hline_4g(cx + (EPD_UI_FORECAST_CARD_W - EPD_UI_FORECAST_TEMP_LINE_LEN) / 2u,
                    cx + (EPD_UI_FORECAST_CARD_W - EPD_UI_FORECAST_TEMP_LINE_LEN) / 2u + EPD_UI_FORECAST_TEMP_LINE_LEN - 1u,
                    cy + EPD_UI_FORECAST_TEMP_LINE_Y);
      format_temp_int_degree(str, sizeof(str), tmin);
      { unsigned int w = gfxfont_string_width(str, &InterTempRegular28pt7b);
        int tx = (int)cx + (int)(EPD_UI_FORECAST_CARD_W > w ? (EPD_UI_FORECAST_CARD_W - w) / 2u : 0u);
        draw_gfxfont_string_4g(tx, (int)cy + (int)EPD_UI_FORECAST_TEMP_MAX_Y + 33, str, &InterTempRegular28pt7b, 3u); }

      cx += EPD_UI_FORECAST_CARD_W + EPD_UI_FORECAST_GAP;
    }
  }

  /* No separator line at bottom; forecast has same margin below as on sides */

  epd_ui_4g_flip_y = 0;

#if EPD_UI_4G_INVERT
  /* Panel shows our 0 as black; invert so we get white background, black content. */
  for (unsigned int i = 0; i < EPD_UI_4G_BUFFER_SIZE; i++)
    epd_4g_buffer[i] = (unsigned char)(~epd_4g_buffer[i]);
#endif

  return epd_4g_buffer;
}
