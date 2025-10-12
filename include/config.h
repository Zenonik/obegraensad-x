#ifndef CONFIG_H
#define CONFIG_H

#define P_EN   5    // OE / Enable
#define P_DI   23   // Data
#define P_CLK  18   // Clock
#define P_CLA  19   // Latch
#define P_KEY  22   // Button

#define DISPLAY_WIDTH 16
#define DISPLAY_HEIGHT 16

#pragma once

#define ROTATE_DISPLAY

#define NTP_SERVER "pool.ntp.org"
#define TIMEZONE "CET-1CEST,M3.5.0/02,M10.5.0/03"

#define AP_NAME "OBEGRÄNSAD-X"
#define AP_PASSWORD ""

#define WEB_SERVER_PORT 80

// ---------------------------------------------------
// EEPROM-Konfiguration
// ---------------------------------------------------
#define EEPROM_SIZE 512           // Gesamtgröße
#define EEPROM_MAGIC 0xABCD1234   // Magic für SettingsManager
// (Feste Adressen sind NICHT nötig – SettingsManager verwaltet alles selbst)

#endif
