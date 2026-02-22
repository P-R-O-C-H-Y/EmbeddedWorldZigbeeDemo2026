#ifndef _DISPLAY_EPD_W21_SPI_
#define _DISPLAY_EPD_W21_SPI_
#include "Arduino.h"

#define EPD_BUSY_PIN 5
#define EPD_RST_PIN  6
#define EPD_DC_PIN   7
#define EPD_CS_PIN   8
#define EPD_SCK_PIN  9
#define EPD_MOSI_PIN 3
#define EPD_MISO_PIN 26
#define EPD_PWR_PIN  -1

// Uncomment if panel uses BUSY LOW=busy (default HIGH=busy)
//#define EPD_BUSY_ACTIVE_LOW
#ifdef EPD_BUSY_ACTIVE_LOW
#define isEPD_W21_BUSY (digitalRead(EPD_BUSY_PIN) == 0)
#else
#define isEPD_W21_BUSY digitalRead(EPD_BUSY_PIN)
#endif
#define EPD_W21_RST_0  digitalWrite(EPD_RST_PIN, LOW)
#define EPD_W21_RST_1  digitalWrite(EPD_RST_PIN, HIGH)
#define EPD_W21_DC_0   digitalWrite(EPD_DC_PIN, LOW)
#define EPD_W21_DC_1   digitalWrite(EPD_DC_PIN, HIGH)
#define EPD_W21_CS_0   digitalWrite(EPD_CS_PIN, LOW)
#define EPD_W21_CS_1   digitalWrite(EPD_CS_PIN, HIGH)

void SPI_Write(unsigned char value);
void EPD_W21_WriteDATA(unsigned char datas);
void EPD_W21_WriteCMD(unsigned char command);

#endif
