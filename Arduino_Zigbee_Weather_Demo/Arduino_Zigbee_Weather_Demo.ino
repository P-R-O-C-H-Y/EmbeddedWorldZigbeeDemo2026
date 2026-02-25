/*
 * Arduino Zigbee Weather Demo
 * ESP32-C5, SHT40 (I2C), E-ink, Zigbee -> Home Assistant.
 *
 * Flow per wake (every 5 min or on touch):
 * 1. Read indoor (SHT40)
 * 2. Report to Zigbee (triggers HA automation)
 * 3. Wait for HA to send OUT + Forecast
 * 4. Draw display once
 * 5. Deep sleep 5 min; wake also on touch panel INT (GPIO 4) for immediate update
 */

#ifndef ZIGBEE_MODE_ED
#error "Zigbee end device mode is not selected in Tools->Zigbee mode"
#endif

#include <Wire.h>
#include <SPI.h>
#include <esp_sleep.h>
#include <Preferences.h>
#include <Adafruit_SHT4x.h>
#include "Zigbee.h"
#include "Display_EPD_W21_spi.h"
#include "Display_EPD_W21.h"
#include "epd_ui.h"

#define I2C_SCL_PIN 1
#define I2C_SDA_PIN 2
#define TOUCH_INT_PIN 4   /* Touch panel INT; wake from deep sleep on touch (ext1) */

#define WAIT_FOR_HA_MS 3000u    /* Wait for HA to send OUT/forecast after report (~1.5s typical, 3s buffer) */
#define ZIGBEE_FIRST_FORM_DELAY_MS 10000u  /* Extra delay after first Zigbee join for proper config */
#define SLEEP_SECONDS 300u      /* 5 min deep sleep between updates */
#define ZIGBEE_CONNECT_TIMEOUT_MS 10000u  /* If Zigbee fails to connect within 10s, continue without it */

/* Sentinel values: HA did not send data; UI shows "---" for these. */
#define OUT_TEMP_NO_DATA  999.0f   /* format shows --- when |temp| > 99.9 */
#define OUT_HUM_NO_DATA   -1.0f    /* format shows --- when hum < 0 or > 100 */
#define OUT_WMO_NO_DATA   -1       /* no valid icon; fallback used */
#define FC_TEMP_NO_DATA   100      /* format shows --- when |temp| > 99 */

/* Set to 1 to render time text. */
#define UI_SHOW_TIME 0

// Zigbee settings
#define ZIGBEE_IN_ENDPOINT 1 // temp, humidity
#define ZIGBEE_OUT_ENDPOINT 2 // Analog (out temp, hum and weather code)
#define ZIGBEE_FORECAST1_ENDPOINT 3   // Analog (day 1 wmo, tmin, tmax - fits in float)
#define ZIGBEE_FORECAST2_ENDPOINT 4   // Analog (day 2 wmo, tmin, tmax)
#define ZIGBEE_FORECAST3_ENDPOINT 5   // Analog (day 3 wmo, tmin, tmax)
#define ZIGBEE_FORECAST_DATES_ENDPOINT 6  // Analog (FC1/2/3 dates: idx|(date<<2), sent 3x)
#define ZIGBEE_LAST_UPDATE_TIME_ENDPOINT 7  // Analog: hour*60+minute (0-1439), HA sends after OUT

// Preferences settings
#define PREFS_NS "weather"

static Adafruit_SHT4x sht4 = Adafruit_SHT4x();
static bool sht4_ready = false;

static float current_in_temp_c = 21.5f;
static float current_in_humidity = 45.0f;
/* OUT/Forecast: "no data" until HA sends; UI shows "---" for these. */
static float current_out_temp_c = OUT_TEMP_NO_DATA;
static float current_out_humidity = OUT_HUM_NO_DATA;
static int current_out_wmo = OUT_WMO_NO_DATA;
static int current_last_update_hour = -1;   /* -1 = not received; else 0-23 */
static int current_last_update_minute = -1; /* 0-59 */
static char current_last_update_str[16] = "---";  /* "HH:MM" or "---" for display */
static char current_fc_date[3][12] = { "---", "---", "---" };
static epd_ui_forecast_day_t current_forecast[3] = {
  { current_fc_date[0], OUT_WMO_NO_DATA, FC_TEMP_NO_DATA, FC_TEMP_NO_DATA },
  { current_fc_date[1], OUT_WMO_NO_DATA, FC_TEMP_NO_DATA, FC_TEMP_NO_DATA },
  { current_fc_date[2], OUT_WMO_NO_DATA, FC_TEMP_NO_DATA, FC_TEMP_NO_DATA },
};

static ZigbeeTempSensor zbTempIn = ZigbeeTempSensor(ZIGBEE_IN_ENDPOINT);
static ZigbeeAnalog zbTempOut = ZigbeeAnalog(ZIGBEE_OUT_ENDPOINT);
static ZigbeeAnalog zbForecast1 = ZigbeeAnalog(ZIGBEE_FORECAST1_ENDPOINT);
static ZigbeeAnalog zbForecast2 = ZigbeeAnalog(ZIGBEE_FORECAST2_ENDPOINT);
static ZigbeeAnalog zbForecast3 = ZigbeeAnalog(ZIGBEE_FORECAST3_ENDPOINT);
static ZigbeeAnalog zbForecastDates = ZigbeeAnalog(ZIGBEE_FORECAST_DATES_ENDPOINT);
static ZigbeeAnalog zbLastUpdateTime = ZigbeeAnalog(ZIGBEE_LAST_UPDATE_TIME_ENDPOINT);

static Preferences prefs;

/* Load saved OUT and forecast from NVS; used when HA does not send data this wake. */
static void prefs_load_weather(void) {
  if (!prefs.begin(PREFS_NS, true)) return;  /* read-only for load */
  current_out_temp_c = prefs.getFloat("out_temp", OUT_TEMP_NO_DATA);
  current_out_humidity = prefs.getFloat("out_hum", OUT_HUM_NO_DATA);
  current_out_wmo = prefs.getInt("out_wmo", OUT_WMO_NO_DATA);
  current_last_update_hour = prefs.getInt("upd_hr", -1);
  current_last_update_minute = prefs.getInt("upd_min", -1);
  if (current_last_update_hour >= 0 && current_last_update_hour <= 23 && current_last_update_minute >= 0 && current_last_update_minute <= 59)
    snprintf(current_last_update_str, sizeof(current_last_update_str), "%d:%02d", current_last_update_hour, current_last_update_minute);
  else
    snprintf(current_last_update_str, sizeof(current_last_update_str), "---");
  for (int i = 0; i < 3; i++) {
    char key[8];
    snprintf(key, sizeof(key), "fc%d_m", i);
    int m = prefs.getInt(key, 0);
    snprintf(key, sizeof(key), "fc%d_d", i);
    int d = prefs.getInt(key, 0);
    snprintf(key, sizeof(key), "fc%d_wmo", i);
    current_forecast[i].wmo_code = prefs.getInt(key, OUT_WMO_NO_DATA);
    snprintf(key, sizeof(key), "fc%d_tmin", i);
    current_forecast[i].temp_min_c = prefs.getInt(key, FC_TEMP_NO_DATA);
    snprintf(key, sizeof(key), "fc%d_tmax", i);
    current_forecast[i].temp_max_c = prefs.getInt(key, FC_TEMP_NO_DATA);
    if (m > 0 && d > 0) {
      snprintf(current_fc_date[i], sizeof(current_fc_date[i]), "%d.%d.", d, m);
    } else {
      snprintf(current_fc_date[i], sizeof(current_fc_date[i]), "---");
    }
    current_forecast[i].date = current_fc_date[i];
  }
  prefs.end();
}

static void prefs_save_out(void) {
  if (!prefs.begin(PREFS_NS, false)) return;
  prefs.putFloat("out_temp", current_out_temp_c);
  prefs.putFloat("out_hum", current_out_humidity);
  prefs.putInt("out_wmo", current_out_wmo);
  prefs.putInt("upd_hr", current_last_update_hour);
  prefs.putInt("upd_min", current_last_update_minute);
  prefs.end();
}

static bool prefs_get_zigbee_formed(void) {
  if (!prefs.begin(PREFS_NS, true)) return true;  /* assume formed if can't read */
  bool v = prefs.getBool("zb_formed", false);
  prefs.end();
  return v;
}

static void prefs_set_zigbee_formed(void) {
  if (!prefs.begin(PREFS_NS, false)) return;
  prefs.putBool("zb_formed", true);
  prefs.end();
}

static void prefs_save_forecast(int idx, int month, int day) {
  if (idx < 0 || idx >= 3) return;
  if (!prefs.begin(PREFS_NS, false)) return;
  char key[8];
  if (month > 0 && day > 0) {
    snprintf(key, sizeof(key), "fc%d_m", idx);
    prefs.putInt(key, month);
    snprintf(key, sizeof(key), "fc%d_d", idx);
    prefs.putInt(key, day);
  }
  snprintf(key, sizeof(key), "fc%d_wmo", idx);
  prefs.putInt(key, current_forecast[idx].wmo_code);
  snprintf(key, sizeof(key), "fc%d_tmin", idx);
  prefs.putInt(key, current_forecast[idx].temp_min_c);
  snprintf(key, sizeof(key), "fc%d_tmax", idx);
  prefs.putInt(key, current_forecast[idx].temp_max_c);
  prefs.end();
}

/* HA packing: (temp*10+500)<<14 | Hum<<7 | Code. Temp -50.0..+50.0°C, 1 decimal. */
static void decode_current_packed(uint32_t packed, float *out_temp_c, float *out_hum, int *out_wmo) {
  if (out_wmo) *out_wmo = (int)(packed & 0x7Fu);
  if (out_hum) *out_hum = (float)((packed >> 7u) & 0x7Fu);
  if (out_temp_c) *out_temp_c = (float)(((int)((packed >> 14u) & 0x3FFu) - 500) / 10.0f);
}

/* FC packing (fits in float): WMO | (Tmin+35)<<7 | (Tmax+35)<<14. Temps -35..50 C. */
static void decode_forecast_packed(uint32_t packed, int *wmo, int *tmin_c, int *tmax_c) {
  if (wmo) *wmo = (int)(packed & 0x7Fu);
  if (tmin_c) *tmin_c = (int)((packed >> 7u) & 0x7Fu) - 35;
  if (tmax_c) *tmax_c = (int)((packed >> 14u) & 0x7Fu) - 35;
}

/* FC dates packing: idx | (date<<2), date=(month-1)*31+(day-1). HA sends 3x (idx 0,1,2). */
static void decode_forecast_date_packed(uint32_t packed, int *idx, int *month, int *day) {
  int i = (int)(packed & 0x3u);
  uint32_t date = (packed >> 2u) & 0x3FFu;  /* 0-371 */
  if (idx) *idx = i;
  if (month) *month = (int)(date / 31u) + 1;
  if (day) *day = (int)(date % 31u) + 1;
}

static void onInOutPackedCurrent(uint32_t packed) {
  decode_current_packed(packed, &current_out_temp_c, &current_out_humidity, &current_out_wmo);
  prefs_save_out();
  Serial.printf("OUT_TEMP received: %.1fC %.0f%% wmo=%d\n", current_out_temp_c, current_out_humidity, current_out_wmo);
}

static void onForecastPackedData(int idx, uint32_t packed) {
  if (idx < 0 || idx >= 3) return;
  decode_forecast_packed(packed, &current_forecast[idx].wmo_code, &current_forecast[idx].temp_min_c, &current_forecast[idx].temp_max_c);
  prefs_save_forecast(idx, 0, 0);  /* date saved separately */
}

static void onForecastDatePacked(uint32_t packed) {
  int idx = 0;
  int m = 0, d = 0;
  decode_forecast_date_packed(packed, &idx, &m, &d);
  if (idx < 0 || idx >= 3) return;
  snprintf(current_fc_date[idx], sizeof(current_fc_date[idx]), "%d.%d.", d, m);
  current_forecast[idx].date = current_fc_date[idx];
  prefs_save_forecast(idx, m, d);
  Serial.printf("FC date received: FC%d = %d.%d.\n", idx + 1, d, m);
}

/* Last update time: value = hour*60 + minute (0-1439). */
static void onLastUpdateTimeReceived(uint32_t value) {
  int total = (int)value;
  if (total < 0 || total > 1439) return;
  current_last_update_hour = total / 60;
  current_last_update_minute = total % 60;
  snprintf(current_last_update_str, sizeof(current_last_update_str), "%d:%02d", current_last_update_hour, current_last_update_minute);
  prefs_save_out();
  Serial.printf("Last update time received: %s\n", current_last_update_str);
}

/* ZigbeeAnalog callbacks (float present value) */
static void onTempOutPacked(float analog) {
  if (analog < 0.0f) return;
  onInOutPackedCurrent((uint32_t)lroundf(analog));
}

static void onForecastPacked(float analog, int idx) {
  if (analog < 0.0f) return;
  onForecastPackedData(idx, (uint32_t)lroundf(analog));
  Serial.printf("FC%d received: %s wmo=%d %d/%dC\n", idx + 1, current_fc_date[idx],
    current_forecast[idx].wmo_code, current_forecast[idx].temp_min_c, current_forecast[idx].temp_max_c);
}

static void onForecast1Packed(float analog) { onForecastPacked(analog, 0); }
static void onForecast2Packed(float analog) { onForecastPacked(analog, 1); }
static void onForecast3Packed(float analog) { onForecastPacked(analog, 2); }

static void onForecastDatesPacked(float analog) {
  if (analog < 0.0f) return;
  onForecastDatePacked((uint32_t)lroundf(analog));
}

static void onLastUpdateTimePacked(float analog) {
  if (analog < 0.0f) return;
  onLastUpdateTimeReceived((uint32_t)lroundf(analog));
}

static bool read_indoor_sensor(float *temp_c, float *hum_percent) {
  if (!sht4_ready) return false;
  sensors_event_t humidity, temp;
  if (!sht4.getEvent(&humidity, &temp)) return false;
  *temp_c = temp.temperature;
  *hum_percent = humidity.relative_humidity;
  return true;
}

static const char *ui_time_or_blank(const char *time_str) {
#if UI_SHOW_TIME
  return time_str;
#else
  (void)time_str;
  return "";
#endif
}

/** Returns true if Zigbee started and connected; false otherwise (continue with display using last known data). */
static bool zigbee_init_receiver(void) {
  zbTempIn.setManufacturerAndModel("Espressif", "ZigbeeWeatherStationDemo");
  zbTempIn.setMinMaxValue(-40, 85);
  zbTempIn.setDefaultValue(21.5);
  zbTempIn.setTolerance(0.1);
  zbTempIn.addHumiditySensor(0, 100, 1, 45);

  zbTempOut.addAnalogOutput();
  zbTempOut.setAnalogOutputDescription("OUT packed");

  zbForecast1.addAnalogOutput();
  zbForecast1.setAnalogOutputDescription("FC1 packed");
  zbForecast2.addAnalogOutput();
  zbForecast2.setAnalogOutputDescription("FC2 packed");
  zbForecast3.addAnalogOutput();
  zbForecast3.setAnalogOutputDescription("FC3 packed");
  zbForecastDates.addAnalogOutput();
  zbForecastDates.setAnalogOutputDescription("FC dates packed");
  zbLastUpdateTime.addAnalogOutput();
  zbLastUpdateTime.setAnalogOutputDescription("Last update time (hour*60+min)");

  zbTempOut.onAnalogOutputChange(onTempOutPacked);
  zbForecast1.onAnalogOutputChange(onForecast1Packed);
  zbForecast2.onAnalogOutputChange(onForecast2Packed);
  zbForecast3.onAnalogOutputChange(onForecast3Packed);
  zbForecastDates.onAnalogOutputChange(onForecastDatesPacked);
  zbLastUpdateTime.onAnalogOutputChange(onLastUpdateTimePacked);

  Zigbee.addEndpoint(&zbTempIn);
  Zigbee.addEndpoint(&zbTempOut);
  Zigbee.addEndpoint(&zbForecast1);
  Zigbee.addEndpoint(&zbForecast2);
  Zigbee.addEndpoint(&zbForecast3);
  Zigbee.addEndpoint(&zbForecastDates);
  Zigbee.addEndpoint(&zbLastUpdateTime);

  esp_zb_cfg_t zigbeeConfig = ZIGBEE_DEFAULT_ED_CONFIG();
  zigbeeConfig.nwk_cfg.zed_cfg.keep_alive = 10000;
  Zigbee.setTimeout(10000);

  if (!Zigbee.begin(&zigbeeConfig, false)) {
    Serial.println("Zigbee failed to start; continuing with last known data.");
    return false;
  }
  Serial.println("Connecting to Zigbee network...");
  unsigned long deadline = millis() + ZIGBEE_CONNECT_TIMEOUT_MS;
  while (!Zigbee.connected()) {
    if (millis() >= deadline) {
      Serial.println();
      Serial.println("Zigbee connect timeout; continuing with last known data.");
      return false;
    }
    Serial.print(".");
    delay(100);
  }
  Serial.println();
  Serial.println("Zigbee connected.");
  if (!prefs_get_zigbee_formed()) {
    Serial.println("First Zigbee join: waiting 5s for proper configuration...");
    delay(ZIGBEE_FIRST_FORM_DELAY_MS);
    prefs_set_zigbee_formed();
  }
  return true;
}

void setup() {
  Serial.begin(115200);

  pinMode(EPD_BUSY_PIN, INPUT);
  pinMode(EPD_RST_PIN, OUTPUT);
  pinMode(EPD_DC_PIN, OUTPUT);
  pinMode(EPD_CS_PIN, OUTPUT);
  digitalWrite(EPD_RST_PIN, HIGH);

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, 400000);
  sht4_ready = sht4.begin();
  if (sht4_ready) {
    sht4.setPrecision(SHT4X_HIGH_PRECISION);
    sht4.setHeater(SHT4X_NO_HEATER);
  } else {
    Serial.println("SHT40 init failed; using cached indoor values.");
  }

  prefs_load_weather();  /* restore last OUT/forecast so we can draw them if HA doesn't send this wake */

  bool zigbee_ok = zigbee_init_receiver();

  /* 1. Read indoor */
  float in_temp = current_in_temp_c;
  float in_hum = current_in_humidity;
  if (read_indoor_sensor(&in_temp, &in_hum)) {
    current_in_temp_c = in_temp;
    current_in_humidity = in_hum;
  }

  if (zigbee_ok) {
    zbTempIn.setTemperature(current_in_temp_c);
    zbTempIn.setHumidity(current_in_humidity);
    zbTempIn.report();
    /* 2. Wait for HA to send OUT + Forecast (Zigbee callbacks update current_*) */
    delay(WAIT_FOR_HA_MS);
  }

  /* 3. Draw display once */
  SPI.end();
  SPI.begin(EPD_SCK_PIN, EPD_MISO_PIN, EPD_MOSI_PIN);
  SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE0));
  EPD_HW_Init_4G();
  const unsigned char *img = epd_ui_build_demo_4g(
    current_in_temp_c, current_in_humidity,
    current_out_temp_c, current_out_humidity, current_out_wmo, current_last_update_str,
    ui_time_or_blank(""), 0.0f, current_forecast);
  EPD_WhiteScreen_ALL_4G(img);

  /* Small delay to allow display to update and print debug information */
  delay(100);

  /* 4. Deep sleep: wake on 5 min timer OR touch (INT on GPIO 4). On wake, setup() runs again. */
  pinMode(TOUCH_INT_PIN, INPUT_PULLUP);   /* Idle high; touch pulls INT low -> wake */
  esp_sleep_enable_timer_wakeup((uint64_t)SLEEP_SECONDS * 1000000u);
  esp_sleep_enable_ext1_wakeup_io((1ULL << TOUCH_INT_PIN), ESP_EXT1_WAKEUP_ANY_LOW);
  esp_deep_sleep_start();
}

void loop() {
  /* Never reached: esp_deep_sleep_start() does not return. */
  delay(1000);
}
