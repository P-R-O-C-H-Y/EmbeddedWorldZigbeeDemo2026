/**
 * Minimal GFX font types for bitmap font headers (Adafruit GFX compatible).
 * Use with fonts/*.h; draw via epd_ui.cpp draw_gfxfont_string_4g().
 */
#ifndef GFXFONT_H
#define GFXFONT_H

#include <stdint.h>

/* Allow font headers to use PROGMEM; define as empty when not provided by Arduino. */
#ifndef PROGMEM
#define PROGMEM
#endif

typedef struct {
  uint16_t bitmapOffset;
  uint8_t  width;
  uint8_t  height;
  uint8_t  xAdvance;
  int8_t   xOffset;
  int8_t   yOffset;
} GFXglyph;

typedef struct {
  const uint8_t  *bitmap;
  const GFXglyph  *glyph;
  uint8_t   first;
  uint8_t   last;
  uint16_t  yAdvance;  /* line height; uint16_t for large fonts (e.g. 120pt = 284) */
} GFXfont;

#endif /* GFXFONT_H */
