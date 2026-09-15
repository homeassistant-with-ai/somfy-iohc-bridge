# Changelog

All notable changes to this project are documented in this file.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [1.0.0] — 2026-09-15

Initial public release. See the [v1.0.0 release notes](https://github.com/homeassistant-with-ai/somfy-iohc-bridge/releases/tag/v1.0.0) for the full feature list.

### Added
- Somfy io-homecontrol pairing (0x39 + 0x30) over 868.95 MHz FSK, verified against real captured frames and cross-referenced reference implementations
- OPEN/CLOSE/STOP button transmission
- Experimental RX-sniffing to track state changes made via the original Situo remote
- Home Assistant MQTT Discovery (`cover` entity)
- Persistent pairing identity in ESP32 NVS flash
- OLED status screen (Heltec WiFi LoRa32 V3)
- Host-native (g++) test suite for the protocol layer, run in CI
- Bilingual documentation (Dutch/English)
