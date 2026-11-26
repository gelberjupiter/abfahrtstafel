# Abfahrtstafel

ÖV-Abfahrtstafel im SBB-Stil für ESP32 + 2.0" ST7789-Display (320x240) oder ESP32-S3 + 1.8" ST7735-Display (160x128).
Daten von search.ch, bei Ausfall automatisch von transport.opendata.ch.

## Updates

Jeder Push auf `main` baut über GitHub Actions eine neue Firmware und veröffentlicht sie als Release (`v<Nummer>`).
Das Release enthält beide Varianten: `firmware.bin` (ESP32) und `firmware-esp32s3.bin` (ESP32-S3); jede Tafel lädt nur ihre eigene.
Die Tafel prüft beim Start und alle 30 Minuten, ob es ein neueres Release gibt, und installiert es selbst.
Während des Updates zeigt sie Fortschritt in Prozent und die geschätzte Restzeit.

## Verkabelung

| Display | ESP32 (2.0" ST7789) | ESP32-S3 (1.8" ST7735) |
|---|---|---|
| GND | GND | GND |
| VCC | 3V3 | 3V3 |
| SCL | 18 | 12 |
| SDA | 23 | 11 |
| RST | 4 | 8 |
| DC | 16 | 9 |
| CS | 17 | 10 |
| BLK | – | 13 (nachts aus) |

Auf dem S3 hat die Tafel ein eigenes Layout für 160x128: 5 Zeilen, Source Sans 15/13 px (Fallblatt: Oswald 17 px).

Flashen per USB: `pio run -e esp32dev -t upload` bzw. `pio run -e esp32s3 -t upload --upload-port <Port>`.

## Bedienung

- Erster Start: QR-Code scannen (oder WLAN "Abfahrtstafel" antippen). Das Einrichtungsfenster öffnet sich
  von selbst, WLAN wählen, Passwort eingeben. Die Tafel prüft das Passwort sofort.
- Einstellungen: http://abfahrtstafel.local (Handy im selben WLAN). Der QR-Code dazu erscheint beim Start
  nach der WLAN-Einrichtung (oder ohne Haltestelle) für 30 s; wird er nicht gescannt, startet die Tafel
  mit den Standard-Einstellungen.
- BOOT kurz drücken: QR-Code zur Einstellungsseite (nachts: Tafel 2 Minuten einschalten).
- BOOT 5 s halten: WLAN zurücksetzen.

## Fehlerbehandlung

- Fällt ein Fahrplan-Server aus, wird automatisch der andere verwendet; die Tafel zeigt den Grund an.
- Zeitserver (NTP) gesperrt, z. B. in Gäste-WLANs: Die Uhrzeit kommt dann aus der Antwort eines Webservers
  und wird alle 6 Stunden nachgestellt.
- WLAN weg: neuer Versuch alle 30 s; nach 10 min Einrichtungsmodus, der das alte WLAN jede Minute weiter versucht.
- Hängt etwas länger (kein Abruf, 1 h kein Server), startet die Tafel still neu (nicht nachts).

## Schriften

Source Sans 3 und Oswald (Fallblatt), beide unter der SIL Open Font License 1.1 (siehe `fonts/`).
