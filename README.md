# Abfahrtstafel

ÖV-Abfahrtstafel im SBB-Stil für ESP32 + 2.0" ST7789-Display, Daten von transport.opendata.ch.

## Updates

Jeder Push auf `main` baut über GitHub Actions eine neue Firmware und veröffentlicht sie als Release (`v<Nummer>`).
Die Tafel prüft beim Start und alle 6 Stunden, ob es ein neueres Release gibt, und installiert es selbst.

## Verkabelung

| Display | ESP32 |
|---|---|
| GND | GND |
| VCC | 3V3 |
| SCL | 18 |
| SDA | 23 |
| RST | 4 |
| DC | 16 |
| CS | 17 |

## Bedienung

- Erster Start: QR-Code scannen, WLAN wählen.
- BOOT kurz drücken: QR-Code zur Einstellungsseite (nachts: Tafel 2 Minuten einschalten).
- BOOT 5 s halten: WLAN zurücksetzen.
