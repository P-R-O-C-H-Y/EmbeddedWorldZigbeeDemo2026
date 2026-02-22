/**
 * EPD UI – partial regions for indoor/outdoor temp, humidity, weather icon, battery, status.
 * Portrait 480x800: content spread over full height. Call EPD_HW_Init_Fast() once before drawing.
 */

#ifndef EPD_UI_H
#define EPD_UI_H

#include <stdint.h>

/* Region sizes (pixels). */
#define EPD_UI_TEMP_REGION_W      80u
#define EPD_UI_TEMP_REGION_H      24u
#define EPD_UI_HUMID_REGION_W     48u
#define EPD_UI_HUMID_REGION_H     24u
#define EPD_UI_ICON_REGION_W      106u
#define EPD_UI_ICON_REGION_H      106u
/* Byte-aligned width for EPD_Dis_Part (driver expects width multiple of 8) */
#define EPD_UI_ICON_PART_LINE     (((EPD_UI_ICON_REGION_W + 7u) / 8u) * 8u)   /* 112 */
#define EPD_UI_LARGE_ICON_W       212u
#define EPD_UI_LARGE_ICON_H       212u
#define EPD_UI_LARGE_ICON_PART_LINE (((EPD_UI_LARGE_ICON_W + 7u) / 8u) * 8u)  /* 216 */
#define EPD_UI_BATTERY_REGION_W   48u
#define EPD_UI_BATTERY_REGION_H   24u
#define EPD_UI_STATUS_REGION_W    432u   /* full width minus margins */
#define EPD_UI_STATUS_REGION_H    24u

/* Layout per ASCII: header | TEMP+HUM (IN data) | IN label | sep | OUT label | icon+TEMP+HUM | WIND | FORECAST 3 DAYS | sep | footer
 * Temp rows use 72px font (glyph ~105px, may extend a few px into gap); humidity/wind 48px. */
#define EPD_UI_LINE_H_8X      72u
#define EPD_UI_BOX_H          72u
#define EPD_UI_TEMP_ROW_H     96u    /* room for 72px font; glyph ~105px extends slightly into gap */
#define EPD_UI_TEMP_HUM_GAP   24u    /* pixels between TEMP row and HUM row */
#define EPD_UI_BOX_GAP        4u
#define EPD_UI_MARGIN         16u
#define EPD_UI_HEADER_H       36u
#define EPD_UI_LABEL_2X_H     (20u + 8u)   /* Source Sans 22px line + gap */

/* Header: battery left, time to right of battery; battery uses fixed offset, rest uses LAYOUT_SHIFT_Y */
#define EPD_UI_LAYOUT_SHIFT_Y 24u   /* everything except Forecast/battery moved up by this */
#define EPD_UI_BATTERY_SHIFT_Y 12u  /* battery Y offset (independent of LAYOUT_SHIFT_Y) */
#define EPD_UI_TIME_X         (EPD_UI_BATTERY_ICON_X + EPD_UI_BATTERY_ICON_W + 2u + 8u)   /* right of battery */
#define EPD_UI_TIME_Y         ((EPD_UI_MARGIN - 10u) - EPD_UI_LAYOUT_SHIFT_Y)
#define EPD_UI_RIGHT_EDGE     (480u - EPD_UI_MARGIN)
#define EPD_UI_BATTERY_ICON_W 54u   /* 64 - 10 */
#define EPD_UI_BATTERY_ICON_H 32u
#define EPD_UI_BATTERY_ICON_X EPD_UI_MARGIN   /* left side, margin from edge */
#define EPD_UI_BATTERY_ICON_Y ((EPD_UI_MARGIN) - EPD_UI_BATTERY_SHIFT_Y + 4u)

/* IN section: TEMP (72px) then HUM (48px) then "IN" label (22px) */
#define EPD_UI_INOUT_SHIFT_Y  8u   /* IN, separator, OUT (temp/hum/icon) moved up by this */
#define EPD_UI_DATA_SHIFT_LEFT 10u /* IN/OUT temp and humidity shifted left by this */
#define EPD_UI_IN_DATA_X      (EPD_UI_MARGIN - EPD_UI_DATA_SHIFT_LEFT)
#define EPD_UI_IN_BOX_W       (480u - EPD_UI_IN_DATA_X - EPD_UI_MARGIN)
#define EPD_UI_IN_TEMP_X      EPD_UI_IN_DATA_X
#define EPD_UI_IN_TEMP_Y      ((EPD_UI_MARGIN + EPD_UI_HEADER_H + 20u) - EPD_UI_LAYOUT_SHIFT_Y - EPD_UI_INOUT_SHIFT_Y)
#define EPD_UI_IN_HUMID_X     EPD_UI_IN_DATA_X
#define EPD_UI_IN_HUMID_Y     (EPD_UI_IN_TEMP_Y + EPD_UI_TEMP_ROW_H + EPD_UI_TEMP_HUM_GAP)
#define EPD_UI_IN_LABEL_X     EPD_UI_MARGIN
#define EPD_UI_IN_LABEL_Y     (EPD_UI_IN_HUMID_Y + EPD_UI_BOX_H + 4u)

#define EPD_UI_SEPARATOR_Y    (EPD_UI_IN_LABEL_Y + EPD_UI_LABEL_2X_H + 8u)

/* OUT section: "OUT" label, icon, TEMP (72px), HUM (48px) */
#define EPD_UI_OUT_LABEL_X    EPD_UI_MARGIN
#define EPD_UI_OUT_LABEL_Y    (EPD_UI_SEPARATOR_Y + 8u)
#define EPD_UI_OUT_TEMP_Y     (EPD_UI_OUT_LABEL_Y + EPD_UI_LABEL_2X_H)
#define EPD_UI_OUT_HUMID_Y    (EPD_UI_OUT_TEMP_Y + EPD_UI_TEMP_ROW_H + EPD_UI_TEMP_HUM_GAP)
#define EPD_UI_OUT_ICON_W     EPD_UI_ICON_REGION_W
#define EPD_UI_OUT_ICON_H     EPD_UI_ICON_REGION_H
#define EPD_UI_OUT_ICON_X     EPD_UI_MARGIN
#define EPD_UI_OUT_ICON_Y     (EPD_UI_OUT_HUMID_Y - 20u)
#define EPD_UI_OUT_DATA_X     (EPD_UI_OUT_ICON_X + EPD_UI_OUT_ICON_W + 12u - EPD_UI_DATA_SHIFT_LEFT)
#define EPD_UI_OUT_BOX_W      (480u - EPD_UI_OUT_DATA_X - EPD_UI_MARGIN)
#define EPD_UI_OUT_TEMP_X     EPD_UI_OUT_DATA_X
#define EPD_UI_OUT_HUMID_X    EPD_UI_OUT_DATA_X

/* FORECAST: side margin same as bottom (MARGIN); single smaller gap between cards */
#define EPD_UI_FORECAST_SIDE_MARGIN  EPD_UI_MARGIN   /* 24u, same as bottom gap */
#define EPD_UI_FORECAST_GAP_BETWEEN 12u              /* single gap between frames */
#define EPD_UI_FORECAST_ICON_W    70u
#define EPD_UI_FORECAST_ICON_H    70u
#define EPD_UI_FORECAST_CARD_W    ((480u - 2u * EPD_UI_FORECAST_SIDE_MARGIN - 2u * EPD_UI_FORECAST_GAP_BETWEEN) / 3u)  /* 136 */
#define EPD_UI_FORECAST_CARD_H    234u   /* 230 + 4 (top padding between border and date) */
#define EPD_UI_FORECAST_GAP       EPD_UI_FORECAST_GAP_BETWEEN
#define EPD_UI_FORECAST_TOP_PADDING  4u   /* gap between top box border and date */
#define EPD_UI_FORECAST_DATE_H    24u
#define EPD_UI_FORECAST_DATE_Y    (36u + EPD_UI_FORECAST_TOP_PADDING)   /* date baseline from card top (was 36, +4px gap) */
#define EPD_UI_FORECAST_ICON_OFFSET_X  ((EPD_UI_FORECAST_CARD_W - EPD_UI_FORECAST_ICON_W) / 2u)
#define EPD_UI_FORECAST_GAP_DATE_ICON  20u   /* gap below date (was 16, +4 from card height increase) */
#define EPD_UI_FORECAST_ICON_Y    (EPD_UI_FORECAST_TOP_PADDING + EPD_UI_FORECAST_DATE_H + EPD_UI_FORECAST_GAP_DATE_ICON)
#define EPD_UI_FORECAST_GAP_ICON_TEMP  17u   /* gap below icon (was 12, +5) */
#define EPD_UI_FORECAST_TEMP_Y    (EPD_UI_FORECAST_ICON_Y + EPD_UI_FORECAST_ICON_H + EPD_UI_FORECAST_GAP_ICON_TEMP)
#define EPD_UI_FORECAST_TEMP_LINE_LEN  64u
#define EPD_UI_FORECAST_GAP_TEMP_LINE  11u   /* gap below min temp (was 6, +5) */
#define EPD_UI_FORECAST_TEMP_LINE_Y    (EPD_UI_FORECAST_TEMP_Y + 32u + EPD_UI_FORECAST_GAP_TEMP_LINE)
#define EPD_UI_FORECAST_GAP_LINE_MAX   12u   /* gap below temp line (was 8, +4) */
#define EPD_UI_FORECAST_TEMP_MAX_Y     (EPD_UI_FORECAST_TEMP_LINE_Y + 1u + EPD_UI_FORECAST_GAP_LINE_MAX)
/* Anchor forecast at bottom: explicit bottom margin so cards don't touch display edge.
 * Some panels have non-visible area at bottom or driver offset: add EPD_UI_FORECAST_BOTTOM_INSET
 * if the margin does not appear (e.g. try 24 or 32). */
#define EPD_UI_FORECAST_BOTTOM_INSET   24u   /* extra offset: add if margin not visible on your panel */
#define EPD_UI_FORECAST_CARDS_Y        (800u - EPD_UI_FORECAST_BOTTOM_INSET - EPD_UI_FORECAST_CARD_H)

#define EPD_UI_SEPARATOR2_Y   (800u - EPD_UI_MARGIN)  /* unused; no line drawn */
#define EPD_UI_FOOTER_Y       (800u - EPD_UI_MARGIN)

/* Legacy / compatibility */
#define EPD_UI_BATTERY_X      (480u - EPD_UI_MARGIN - 4u * 56u)
#define EPD_UI_BATTERY_Y      EPD_UI_MARGIN
#define EPD_UI_IN_VAL_X       EPD_UI_IN_TEMP_X
#define EPD_UI_IN_VAL_Y       EPD_UI_IN_TEMP_Y
#define EPD_UI_OUT_VAL_X      EPD_UI_OUT_TEMP_X
#define EPD_UI_OUT_VAL_Y      EPD_UI_OUT_TEMP_Y
#define EPD_UI_LARGE_ICON_X   EPD_UI_OUT_ICON_X
#define EPD_UI_LARGE_ICON_Y   EPD_UI_OUT_ICON_Y

/* Legacy / partial-update names (map to IN/OUT layout). */
#define EPD_UI_INDOOR_LABEL_X     EPD_UI_IN_LABEL_X
#define EPD_UI_INDOOR_LABEL_Y     EPD_UI_IN_LABEL_Y
#define EPD_UI_INDOOR_VAL_X       EPD_UI_IN_VAL_X
#define EPD_UI_INDOOR_VAL_Y       EPD_UI_IN_VAL_Y
#define EPD_UI_OUTDOOR_LABEL_X    EPD_UI_OUT_LABEL_X
#define EPD_UI_OUTDOOR_LABEL_Y    EPD_UI_OUT_LABEL_Y
#define EPD_UI_OUTDOOR_VAL_X      EPD_UI_OUT_VAL_X
#define EPD_UI_OUTDOOR_VAL_Y      EPD_UI_OUT_VAL_Y
#define EPD_UI_OUTDOOR_ICON_X    EPD_UI_OUT_ICON_X
#define EPD_UI_OUTDOOR_ICON_Y    EPD_UI_OUT_ICON_Y
#define EPD_UI_STATUS_X          EPD_UI_MARGIN
#define EPD_UI_STATUS_Y          EPD_UI_FOOTER_Y
#define EPD_UI_STATUS2_X         EPD_UI_MARGIN
#define EPD_UI_STATUS2_Y         (EPD_UI_FOOTER_Y + EPD_UI_LINE_H_8X)

/* Legacy names for partial-update API */
#define EPD_UI_INDOOR_TEMP_X      EPD_UI_INDOOR_LABEL_X
#define EPD_UI_INDOOR_TEMP_Y      EPD_UI_INDOOR_LABEL_Y
#define EPD_UI_INDOOR_HUMID_X     EPD_UI_INDOOR_VAL_X
#define EPD_UI_INDOOR_HUMID_Y     EPD_UI_INDOOR_VAL_Y
#define EPD_UI_OUTDOOR_TEMP_X     EPD_UI_OUTDOOR_LABEL_X
#define EPD_UI_OUTDOOR_TEMP_Y     EPD_UI_OUTDOOR_LABEL_Y
#define EPD_UI_OUTDOOR_HUMID_X    EPD_UI_OUTDOOR_VAL_X
#define EPD_UI_OUTDOOR_HUMID_Y    EPD_UI_OUTDOOR_VAL_Y

/** Weather icon / condition (maps from Open-Meteo WMO or HA). */
typedef enum {
  EPD_UI_ICON_CLEAR = 0,
  EPD_UI_ICON_PARTLY_CLOUDY,
  EPD_UI_ICON_CLOUDY,
  EPD_UI_ICON_FOG,
  EPD_UI_ICON_RAIN,
  EPD_UI_ICON_SNOW,
  EPD_UI_ICON_THUNDERSTORM,
  EPD_UI_ICON_COUNT
} epd_ui_weather_icon_t;

/**
 * Map Open-Meteo WMO weather_code to EPD_UI icon.
 * https://open-meteo.com/en/docs#api_form
 */
epd_ui_weather_icon_t epd_ui_weather_code_to_icon(int wmo_code);

/** Draw indoor temperature (e.g. "21.5°C"). Partial update only this region. */
void epd_ui_draw_indoor_temp(float temp_c);

/** Draw outdoor temperature. */
void epd_ui_draw_outdoor_temp(float temp_c);

/** Draw indoor humidity (e.g. "45%"). */
void epd_ui_draw_indoor_humidity(float humidity_percent);

/** Draw outdoor humidity. */
void epd_ui_draw_outdoor_humidity(float humidity_percent);

/** Draw outdoor weather icon 64x64 (clear, rain, snow, etc.). */
void epd_ui_draw_outdoor_icon(epd_ui_weather_icon_t icon);

/** Draw large centered weather icon 128x128 (2x scaled). */
void epd_ui_draw_large_weather_icon(epd_ui_weather_icon_t icon);

/** Draw battery percentage (0–100). Partial update only this region. */
void epd_ui_draw_battery(float percent);

/** Draw status line (e.g. "Updated 12:34"). Max ~50 chars. */
void epd_ui_draw_status(const char *str);

/** Draw second status line. */
void epd_ui_draw_status2(const char *str);

/** Partial redraw: time in header (always refreshed on wake). */
void epd_ui_draw_time_header(const char *time_str);

/** Partial redraw: battery icon in header. */
void epd_ui_draw_battery_header(float percent);

/** Partial redraw: full IN section (temp, humidity, label). */
void epd_ui_draw_indoor_block(float indoor_temp_c, float indoor_humidity);

/** Partial redraw: full OUT section (label, icon, temp, humidity). */
void epd_ui_draw_outdoor_block(float outdoor_temp_c, float outdoor_humidity, int wmo_weather_code);

/** One day of forecast: date (e.g. "18.2."), WMO code for icon, temp min/max °C (integer). */
typedef struct {
  const char *date;
  int wmo_code;
  int temp_min_c;
  int temp_max_c;
} epd_ui_forecast_day_t;

/** Partial redraw: forecast cards area. */
void epd_ui_draw_forecast_block(const epd_ui_forecast_day_t *forecast);

/** Build full-screen 4G image buffer (96000 bytes) with demo layout (per ASCII art).
 *  status1: time (top-left). wind_speed_m_s: unused (kept for API compatibility).
 *  forecast: 3 days (date, icon, temp min-max); NULL = placeholders.
 */
const unsigned char *epd_ui_build_demo_4g(float indoor_temp_c, float indoor_humidity,
  float outdoor_temp_c, float outdoor_humidity, int wmo_weather_code, float battery_percent,
  const char *status1, float wind_speed_m_s, const epd_ui_forecast_day_t *forecast);

#define EPD_UI_4G_BUFFER_SIZE  (96000u)  /* EPD_ARRAY * 2 for 4-gray full screen */

#endif /* EPD_UI_H */
