/**
 * Weather icons for E-ink. Each icon has its own W/H (from PNG); icons are centered in frame.
 * Frame size is 106x106. Use blit_1bit_to_4g / blit_4g_icon_to_4g with icon dimensions.
 */
#ifndef WEATHER_ICONS_4G_H
#define WEATHER_ICONS_4G_H

#include <stdint.h>

/* Frame size: icon is centered in this region on the E-ink. */
#define WEATHER_ICON_FRAME_W  106
#define WEATHER_ICON_FRAME_H  106
/* Max buffer size for 1-bit frame (used for centering smaller 1-bit icons). */
#define WEATHER_ICON_BYTES    (((WEATHER_ICON_FRAME_W + 7) / 8) * WEATHER_ICON_FRAME_H)

/* Icons: 0 and 45 are 1-bit only (SUN, FOG); all others use 4G. */
#include "weather_icon_0.h"   /* 1-bit */
#include "weather_icon_45.h"  /* 1-bit */
#include "weather_icon_2_4g.h"
#include "weather_icon_3_4g.h"
#include "weather_icon_51_4g.h"
#include "weather_icon_61_4g.h"
#include "weather_icon_71_4g.h"
#include "weather_icon_95_4g.h"

/** True for WMO codes that have a 4G icon (only 0 and 45 use 1-bit). */
inline int weather_icon_use_4g(int wmo_code) {
  return (wmo_code == 2 || wmo_code == 3 || wmo_code == 51 || wmo_code == 61 || wmo_code == 71 || wmo_code == 95);
}

/** Return 4G icon width for given WMO (actual PNG width). */
inline unsigned int weather_icon_4g_width_by_wmo(int wmo_code) {
  switch (wmo_code) {
    case 2:   return WEATHER_ICON_WEATHER_2_4G_W;
    case 3:   return WEATHER_ICON_WEATHER_3_4G_W;
    case 51: case 52: case 53: case 54: case 55: case 56: case 57: case 58: case 59:
    case 80: case 81: case 82: return WEATHER_ICON_WEATHER_51_4G_W;
    case 60: case 61: case 62: case 63: case 64: case 65: case 66: case 67: return WEATHER_ICON_WEATHER_61_4G_W;
    case 71: case 73: case 75: case 77: case 85: case 86: return WEATHER_ICON_WEATHER_71_4G_W;
    case 95: case 96: case 99: return WEATHER_ICON_WEATHER_95_4G_W;
    default:  return WEATHER_ICON_FRAME_W;
  }
}

/** Return 4G icon height for given WMO (actual PNG height). */
inline unsigned int weather_icon_4g_height_by_wmo(int wmo_code) {
  switch (wmo_code) {
    case 2:   return WEATHER_ICON_WEATHER_2_4G_H;
    case 3:   return WEATHER_ICON_WEATHER_3_4G_H;
    case 51: case 52: case 53: case 54: case 55: case 56: case 57: case 58: case 59:
    case 80: case 81: case 82: return WEATHER_ICON_WEATHER_51_4G_H;
    case 60: case 61: case 62: case 63: case 64: case 65: case 66: case 67: return WEATHER_ICON_WEATHER_61_4G_H;
    case 71: case 73: case 75: case 77: case 85: case 86: return WEATHER_ICON_WEATHER_71_4G_H;
    case 95: case 96: case 99: return WEATHER_ICON_WEATHER_95_4G_H;
    default:  return WEATHER_ICON_FRAME_H;
  }
}

/** Return 1-bit icon width for given WMO (only 0 and 45 have 1-bit; others use 4G). */
inline unsigned int weather_icon_1bit_width_by_wmo(int wmo_code) {
  switch (wmo_code) {
    case 0: case 1: return WEATHER_ICON_WEATHER_0_W;
    case 45: case 48: return WEATHER_ICON_WEATHER_45_W;
    default: return WEATHER_ICON_FRAME_W;
  }
}

/** Return 1-bit icon height for given WMO (only 0 and 45 have 1-bit; others use 4G). */
inline unsigned int weather_icon_1bit_height_by_wmo(int wmo_code) {
  switch (wmo_code) {
    case 0: case 1: return WEATHER_ICON_WEATHER_0_H;
    case 45: case 48: return WEATHER_ICON_WEATHER_45_H;
    default: return WEATHER_ICON_FRAME_H;
  }
}

/**
 * Return 4-gray (2bpp) column-major icon for given WMO, or nullptr if 1-bit only.
 * PROGMEM; use blit_4g_icon_to_4g() which reads via pgm_read_byte on AVR.
 */
inline const unsigned char* weather_icon_4g_by_wmo(int wmo_code) {
  switch (wmo_code) {
    case 2:   return weather_2_4g;
    case 3:   return weather_3_4g;
    case 51: case 52: case 53: case 54: case 55: case 56: case 57: case 58: case 59:
    case 80: case 81: case 82:  return weather_51_4g;
    case 60: case 61: case 62: case 63: case 64: case 65: case 66: case 67: return weather_61_4g;
    case 71:
    case 73:
    case 75:
    case 77:
    case 85:
    case 86:  return weather_71_4g;
    case 95:
    case 96:
    case 99:  return weather_95_4g;
    default:  return nullptr;
  }
}

/**
 * Return 1-bit bitmap for given WMO code, or nullptr if unknown.
 * Data is PROGMEM; use memcpy_P to copy to RAM if needed, or pass to blit that reads PROGMEM.
 */
inline const unsigned char* weather_icon_bitmap_by_wmo(int wmo_code) {
  switch (wmo_code) {
    case 0: case 1:   return weather_0;
    case 45: case 48: return weather_45;
    default:          return nullptr;
  }
}

#endif /* WEATHER_ICONS_4G_H */
