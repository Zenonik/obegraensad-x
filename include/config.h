#pragma once

// === START Systemconfig - DONT TOUCH! ===

#define P_EN   5    // OE / Enable
#define P_DI   23   // Data
#define P_CLK  18   // Clock
#define P_CLA  19   // Latch
#define P_KEY  22   // Button

#define DISPLAYMODES 6

#define DISPLAY_WIDTH 16
#define DISPLAY_HEIGHT 16

// === END Systemconfig - DONT TOUCH! ===
// ========================================
// === Userconfig ===

#define ROTATE_DISPLAY

#define NTP_SERVER "pool.ntp.org"
#define TIMEZONE "CET-1CEST,M3.5.0/02,M10.5.0/03"

#define CITY "Detmold"  // für Wetter-API

#define AP_NAME "OBEGRÄNSAD-X"
#define AP_PASSWORD ""
#define WEB_SERVER_PORT 80
#define OTA_VERSION_URL "https://raw.githubusercontent.com/<user>/<repo>/main/version.txt"
#define OTA_FIRMWARE_URL "https://github.com/<user>/<repo>/releases/latest/download/firmware.bin"