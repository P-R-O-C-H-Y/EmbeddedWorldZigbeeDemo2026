# Arduino Zigbee Weather Demo

A low-power weather station built on **ESP32-C5** (M5Stack Stamp-C5) that displays indoor and outdoor conditions plus a 3-day forecast on an E-ink panel. Indoor data comes from a local SHT40 sensor; outdoor data and forecast are fetched by Home Assistant via the Open-Meteo API and pushed to the device over Zigbee.

---

## Overview

The device wakes every **5 minutes**, reads indoor temperature and humidity from the SHT40 sensor, reports to Zigbee (which triggers a Home Assistant automation), waits for HA to push outdoor data and forecast, updates the E-ink display, then enters deep sleep. Outdoor and forecast data are stored in NVS so the display keeps showing the last received values when HA does not send new data (e.g., forecast updates only hourly).

### Features

- **Indoor**: Temperature and humidity from SHT40 (I2C)
- **Outdoor**: Current temperature, humidity, and weather icon from Open-Meteo (via HA)
- **Forecast**: 3-day forecast with date, weather icon, and min/max temperatures
- **Battery**: Percentage displayed in the header
- **Persistence**: Last outdoor data and forecast saved to NVS; used when HA does not send data this wake

---

## Hardware

| Component    | Description                         |
|-------------|-------------------------------------|
| MCU         | ESP32-C5 (Zigbee end device mode)   |
| Indoor      | SHT40 temperature/humidity (I2C)    |
| Display     | E-ink 480×800, 4-gray               |
| Connectivity| Zigbee 3.0                          |

**Pin configuration** (see `Arduino_Zigbee_Weather_Demo.ino`):

- I2C: SDA = 2, SCL = 1 (SHT40)
- EPD: Defined in `Display_EPD_W21_spi.h` / `Display_EPD_W21.h`

---

## Required Arduino Libraries

Install via **Sketch → Include Library → Manage Libraries** (or Arduino CLI):

| Library            | Purpose                    | Install as                    |
|--------------------|----------------------------|-------------------------------|
| **Adafruit SHT4x** | SHT40 temperature/humidity | `Adafruit SHT4x` by Adafruit  |

---

## Setup Steps

### 1. Espressif Arduino Core

- Install **ESP32 Arduino**
- In **Tools → Board**, select **ESP32C5 Dev Module**
- In **Tools → Zigbee mode**, select **End Device (ED)**.
- In **Tools → Partition Scheme**, select **Zigbee 4MB with spiffs**.
- Open `Arduino_Zigbee_Weather_Demo.ino` and flash the board.

### 2. Connect to Zigbee

- Pair the device with your Home Assistant Zigbee network (ZHA or Zigbee2MQTT).
- In HA, open the device details and note the **IEEE address** (e.g. `D0:CF:13:FE:FF:E1:9B:4C`).

### 3. Add REST command

Add the following to `configuration.yaml`. Adjust `latitude`, `longitude`, and `timezone` as needed:

```yaml
rest_command:
  fetch_openmeteo_forecast:
    url: "https://api.open-meteo.com/v1/forecast?latitude=49.149&longitude=17.0019&daily=temperature_2m_max,temperature_2m_min,weather_code&current=temperature_2m,relative_humidity_2m,weather_code&timezone=Europe%2FBerlin&forecast_days=3"
    method: GET
```

Restart Home Assistant.

### 4. Create helper for forecast sync

The automation uses an input datetime to track when the forecast was last synced.

1. Go to **Settings → Devices & Services → Helpers**.
2. Create a helper: **Date and/or time**.
3. Name it `Last weather forecast sync` (or keep default).
4. Ensure the entity ID is `input_datetime.last_weather_forecast_sync`.

Alternatively, create it via `configuration.yaml`:

```yaml
input_datetime:
  last_weather_forecast_sync:
    name: "Last weather forecast sync"
```

### 5. Import the automation

1. Go to **Settings → Automations**.
2. Click **Create automation** → **Edit in YAML**.
3. Paste the contents of `ha_automation_zigbee_station_smart_sync.yaml`.
4. Replace the placeholder IEEE address (`D0:CF:13:FE:FF:E1:9B:4C`) with your device’s IEEE address (in both `zha.set_zigbee_cluster_attribute` actions).
5. Save and enable the automation.

---

## How it works

1. The device reports indoor temperature, humidity, and battery over Zigbee.
2. A state change on the temperature sensor triggers the HA automation.
3. The automation calls the Open-Meteo REST API and receives current conditions and 3-day forecast.
4. HA encodes the data as packed 32-bit integers and writes them to Zigbee Analog Output clusters (endpoints 2–5).
5. The device receives the values, decodes them, stores them in NVS, and refreshes the E-ink display.
6. The device sleeps for 5 minutes and repeats the cycle.

**Data rates:**

- **Current outdoor**: Every 5 minutes (on each device wake).
- **Forecast**: About every hour (automation skips forecast when last sync was &lt; 55 minutes ago).

---

## Files

| File                                | Purpose                          |
|-------------------------------------|----------------------------------|
| `Arduino_Zigbee_Weather_Demo.ino`   | Main firmware                    |
| `epd_ui.cpp` / `epd_ui.h`           | E-ink layout and drawing         |
| `weather_icons/`                   | Weather icon assets (4G + 1-bit) |
| `ha_automation_zigbee_station_smart_sync.yaml` | HA automation definition  |

---

## Troubleshooting

- **Display shows "---" for outdoor / forecast**  
  Ensure the device is paired, the automation runs without errors, and the IEEE address in the automation matches your device.

- **Indoor data wrong or missing**  
  Check SHT40 wiring (SDA, SCL, VCC, GND) and that I2C pins in the sketch match your board.

- **Forecast not updating**  
  Confirm `input_datetime.last_weather_forecast_sync` exists and that the REST command returns valid JSON (check logs).
