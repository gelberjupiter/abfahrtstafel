# Abfahrtstafel

ÖV-Abfahrtstafel im SBB-Stil für ESP32 + 2.0" ST7789-Display.
Daten von search.ch, bei Ausfall automatisch von transport.opendata.ch.

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

- Erster Start: QR-Code scannen (oder WLAN "Abfahrtstafel" antippen). Das Einrichtungsfenster öffnet sich
  von selbst, WLAN wählen, Passwort eingeben. Die Tafel prüft das Passwort sofort.
- Einstellungen: http://abfahrtstafel.local (Handy im selben WLAN).
- BOOT kurz drücken: QR-Code zur Einstellungsseite (nachts: Tafel 2 Minuten einschalten).
- BOOT 5 s halten: WLAN zurücksetzen.

## Fehlerbehandlung

- Fällt ein Fahrplan-Server aus, wird automatisch der andere verwendet; die Tafel zeigt den Grund an.
- WLAN weg: neuer Versuch alle 30 s; nach 10 min Einrichtungsmodus, der das alte WLAN jede Minute weiter versucht.
- Hängt etwas länger (kein Abruf, 1 h kein Server), startet die Tafel still neu (nicht nachts).

## Schriften

Source Sans 3 und Oswald (Fallblatt), beide unter der SIL Open Font License 1.1 (siehe `fonts/`).
