# X-Clock für ESP32

WiFi-Uhr basierend auf IKEA OBEGRÄNSAD mit ESP32 (ELEGOO Board)

## Features

- WiFi Manager mit Captive Portal für einfache WLAN-Einrichtung
- Webinterface zur Konfiguration über Browser
- NTP Zeit-Synchronisation mit automatischer Aktualisierung
- Mehrere Anzeigemodi: Uhrzeit, Sekunden, Datum
- Helligkeitsregelung über Webinterface
- Button-Steuerung für Modus-Wechsel
- Persistente Einstellungen in EEPROM

## Hardware

- ELEGOO ESP32 Entwicklungsboard (USB-C)
- IKEA OBEGRÄNSAD LED Panel (16x16)
- Verbindungskabel

## Pin-Belegung

\`\`\`
ESP32 Pin  →  Funktion
─────────────────────
GPIO 18    →  Enable (P_EN)
GPIO 19    →  Data In (P_DI / MOSI)
GPIO 21    →  Clock (P_CLK / SCK)
GPIO 22    →  Latch (P_CLA)
GPIO 23    →  Button (P_KEY)
\`\`\`

## Installation

### 1. PlatformIO installieren

- Visual Studio Code installieren
- PlatformIO Extension installieren

### 2. Projekt kompilieren

\`\`\`bash
cd obegraensad-x
pio run
pio run --target upload
pio device monitor
\`\`\`

### 3. Erste Einrichtung

1. ESP32 mit Strom versorgen
2. Captive Portal öffnet sich automatisch
   - SSID: `OBEGRÄNSAD-X`
3. WLAN-Zugangsdaten eingeben
4. Fertig! Die Uhr verbindet sich und zeigt die Zeit an

## Webinterface

Nach der WLAN-Einrichtung ist das Webinterface erreichbar unter:

\`\`\`
http://[IP-ADRESSE]/
\`\`\`

Die IP-Adresse wird im Serial Monitor angezeigt.

### Funktionen

- Helligkeit einstellen (10-255)
- Anzeigemodus wählen (Uhrzeit, Sekunden, Datum)
- Gerät neu starten
- Verbindungsinformationen anzeigen

## Button-Steuerung

Kurzer Druck → Wechselt zwischen den Anzeigemodi

## Anzeigemodi

- **Modus 1:** Uhrzeit (HH:MM)
- **Modus 2:** Sekunden
- **Modus 3:** Datum (TT.MM)
- **Modus 4:** Wetter
- **Modus 5:** Display aus


## Zeitzone anpassen

In `config.h`:

// Für Deutschland (MEZ/MESZ)
#define TIMEZONE "CET-1CEST,M3.5.0/02,M10.5.0/03"

## Ort anpassen

In `config.h`:

`CITY` anpassen

## Displayrotation

In `config.h`:

`#define ROTATE_DISPLAY` hinzufügen

## Troubleshooting

### Display bleibt dunkel
- Stromversorgung prüfen (5V erforderlich)
- Pin-Verbindungen überprüfen
- Serial Monitor für Fehlermeldungen checken

### WiFi verbindet nicht
- WLAN-Zugangsdaten prüfen
- Router-Kompatibilität (2.4 GHz erforderlich)
- Captive Portal neu starten

### Zeit wird nicht angezeigt
- Internet-Verbindung prüfen
- NTP-Server erreichbar?
- Zeitzone korrekt konfiguriert?

## Dokumentation

Für detaillierte technische Spezifikationen siehe:

- [Product Requirements Document (PRD)](docs/PRD.md)
- [Software-Architektur](docs/ARCHITECTURE.md)
- [REST API Dokumentation](docs/API.md)

## Credits

Basierend auf dem X-Clock Projekt von Dr. Armin Zink  
Original: http://blog.digital-image.de/2023/05/31/x-clock/

Angepasst für ESP32 mit modernem Webinterface und WiFiManager.

## Lizenz

Open Source - Frei verwendbar für private und kommerzielle Projekte.
