# Somfy Bridge — Somfy io-homecontrol besturen via Home Assistant (ESP32, lokaal, zonder cloud)

*Ook beschikbaar in het [Engels / English](README.en.md).*

[![Licentie: MIT](https://img.shields.io/badge/licentie-MIT-green.svg)](LICENSE)
[![Platform: ESP32-S3](https://img.shields.io/badge/platform-ESP32--S3-blue.svg)](platformio.ini)
[![Home Assistant](https://img.shields.io/badge/Home%20Assistant-MQTT%20Discovery-41BDF5.svg)](#home-assistant-configuratie)
[![Geen cloud nodig](https://img.shields.io/badge/cloud-niet%20nodig-success.svg)](#credits)

Door [homeassistant-with-ai](https://github.com/homeassistant-with-ai) — vrij te gebruiken/aan te passen/te forken, zie [LICENSE](LICENSE).

**Goedkoop, open-source alternatief voor de Somfy TaHoma Switch en de Somfy
Connectivity Kit.** Deze DIY Somfy-bridge maakt een Somfy io-homecontrol
zonnescherm, rolluik of gordijnmotor (getest met een **Situo 1 io Pure II**
afstandsbediening) bestuurbaar vanuit **Home Assistant** via **MQTT** — 100%
lokaal, zonder Somfy TaHoma, Connexoon-app, Somfy-cloudaccount of enige
andere internetdienst. Draait op een goedkope **ESP32-S3 + SX1262**-radio
(Heltec WiFi LoRa32 V3), gebouwd met **RadioLib** en **PlatformIO**.

### Waarom deze Somfy-bridge in plaats van een TaHoma Switch of Connectivity Kit?

| | Somfy TaHoma Switch | Somfy Connectivity Kit | **Deze Somfy Bridge (DIY)** |
|---|---|---|---|
| Richtprijs\* | ±€130–150 | ±€100–130 | **±€20–25** (Heltec WiFi LoRa32 V3) |
| Cloud/account nodig | Ja (Somfy-cloud) | Ja (Somfy-cloud) | **Nee — volledig lokaal** |
| Open source | Nee | Nee | **Ja (MIT-licentie)** |
| Aanpasbaar/uitbreidbaar | Nee | Nee | **Ja — volledige broncode** |
| Werkt bij internetuitval | Nee (cloud-afhankelijk) | Nee (cloud-afhankelijk) | **Ja** |

\* Richtprijzen, kunnen per regio en moment verschillen — controleer actuele
prijzen zelf. Genoemd ter indicatie van de kostenbesparing, niet als
harde belofte.

**Trefwoorden:** Somfy io-homecontrol Home Assistant, Somfy zonder TaHoma,
Somfy zonder Connexoon, Somfy zonder cloud, ESP32 Somfy bridge, Somfy
MQTT, Heltec WiFi LoRa32 V3 Somfy, Somfy zonnescherm domotica, Somfy
Connectivity Kit alternatief, Somfy TaHoma Switch alternatief, DIY smart
home Somfy, RadioLib SX1262 Somfy.

De bridge:
- pairt als een **nieuwe, geautoriseerde afstandsbediening** bij je motor via de normale PROG-procedure (geen beveiliging omzeild, geen replay-aanval)
- stuurt Open/Close/Stop via 868,95 MHz FSK, protocolcompatibel met io-homecontrol 1W
- **luistert ook mee** met je originele Situo, zodat de getoonde stand synchroon blijft als je die afstandsbediening gebruikt (experimentele functie, zie verderop)
- registreert zichzelf in Home Assistant via MQTT Discovery als een normale `cover`-entiteit
- bewaart de koppeling (adres, sleutel, teller) in ESP32 NVS-flash, dus een reboot vereist geen nieuwe pairing

## Ondersteunde hardware

**Gebouwd en getest op:** Heltec WiFi LoRa32 **V3** (ESP32-S3 + SX1262).

De protocollaag (`lib/iohc/`) is hardware-onafhankelijk en host-testbaar;
de radio-specifieke code (`src/tx_control.cpp`, `src/rx_control.cpp`,
`platformio.ini`) is specifiek geschreven en geverifieerd voor de SX1262
op dit board.

## Benodigde libraries (PlatformIO regelt dit automatisch)

Zie `platformio.ini` — bij de eerste build download PlatformIO deze zelf:

| Library | Doel |
|---|---|
| `jgromes/RadioLib` | SX1262 radiobesturing (FSK TX/RX) |
| `thingpulse/ESP8266 and ESP32 OLED driver for SSD1306 displays` | OLED-statusscherm |
| `knolleary/PubSubClient` | MQTT-client |
| `bblanchon/ArduinoJson` | Home Assistant discovery-JSON opbouwen |

## Build

```bash
pio run
```

Vanuit de projectmap (`somfy-iohc-bridge/`). Eerste build duurt langer
(toolchain-download); volgende builds zijn snel.

## Flash

Sluit het board via USB-C aan en zoek de poortnaam:

```bash
ls /dev/cu.usbserial-*      # macOS, meestal iets als /dev/cu.usbserial-0001
```

```bash
pio run -t upload --upload-port /dev/cu.usbserial-0001
```

## Serial monitor

```bash
pio device monitor -p /dev/cu.usbserial-0001 -b 115200
```

Bij het opstarten draait de firmware eerst een reeks **self-tests**
(crypto, frame-opbouw) tegen bekende, geverifieerde testvectoren uit de
protocoldocumentatie. Als die falen, stopt de firmware met een duidelijke
foutmelding — er wordt dan nooit geprobeerd te zenden met mogelijk kapotte
logica.

## Configuratie: WiFi en MQTT (secrets.h)

Wifi- en MQTT-wachtwoorden staan **niet** in Git. Kopieer het voorbeeld en vul in:

```bash
cp include/secrets.h.example include/secrets.h
```

```cpp
#define WIFI_SSID     "jouw-wifi-naam"
#define WIFI_PASSWORD "jouw-wifi-wachtwoord"

#define MQTT_HOST     "homeassistant.local"   // of het IP van je HA-instantie
#define MQTT_PORT     1883
#define MQTT_USER     "mqtt-gebruikersnaam"   // apart, niet-admin HA-gebruikersaccount aanbevolen
#define MQTT_PASSWORD "mqtt-wachtwoord"
```

`include/secrets.h` staat in `.gitignore`. Zonder dit bestand compileert het
project niet (met opzet — dan kun je nooit per ongeluk zonder configuratie
flashen).

**MQTT-gebruiker aanmaken in Home Assistant:** Instellingen → Personen →
Gebruikers → Gebruiker toevoegen. Géén Administrator-rechten nodig.

## Somfy pairing

De bridge pairt zichzelf als een **nieuwe afstandsbediening** via de normale
PROG-procedure van je motor — niets wordt omzeild.

1. Flash de firmware en open de serial monitor.
2. Zet je Somfy-motor in **PROG/leermodus** (via de PROG-knop op je
   bestaande Situo, of op de motor/muurschakelaar zelf — afhankelijk van je
   installatie).
3. Houd binnen dat tijdvenster de **BOOT-knop** op het board **>1,5 seconde**
   ingedrukt.
4. De firmware genereert een nieuw willekeurig adres + 128-bit sleutel, en
   zendt het pairing-commando (0x39 + 0x30) naar het broadcast-adres.
5. Het OLED-scherm toont "Connected" zodra de zending is voltooid.

> **Belangrijk:** io-homecontrol 1W geeft **geen bevestiging** terug. "Connected"
> betekent dat de bridge succesvol heeft **gezonden**, niet dat de motor het
> zeker geaccepteerd heeft. Test daarna één commando (stap hieronder) en kijk
> of het scherm echt beweegt.

**Testen:** korte druk op de BOOT-knop cyclet door CLOSE → STOP → OPEN.
Vanuit Home Assistant werkt de `cover.zonnescherm`-entiteit (of hoe je 'm
hernoemd hebt) direct met de standaard Open/Stop/Close-bediening.

De koppeling (adres, sleutel, sequence-teller) wordt na een succesvolle
pairing **automatisch opgeslagen in NVS-flash** — een reboot, stroomuitval
of herflash met dezelfde firmware vereist dus **geen nieuwe pairing**.

**Je originele Situo blijft gewoon werken** — pairing voegt de bridge toe
als extra afstandsbediening, er wordt niets van de bestaande koppeling
gewist of overschreven.

## Home Assistant configuratie

Grotendeels automatisch via **MQTT Discovery**:

1. Zorg dat Home Assistant's ingebouwde **MQTT-integratie** actief is en
   verbonden is met dezelfde broker als in `secrets.h`
   (Instellingen → Apparaten & Services → Integratie toevoegen → MQTT,
   als die nog niet bestaat).
2. Zodra de bridge verbindt, verschijnt automatisch een apparaat **"Somfy
   Bridge"** met een `cover`-entiteit onder Instellingen → Apparaten & Services → MQTT.
3. Hernoem de entiteit eventueel naar smaak (Instellingen → Entiteiten →
   zoek "Zonnescherm" → potlood-icoon).

**MQTT-topics** (voor wie het los wil gebruiken, ook zonder Home Assistant):

| Topic | Richting | Inhoud |
|---|---|---|
| `somfy/awning/set` | naar bridge | `OPEN` / `CLOSE` / `STOP` |
| `somfy/awning/state` | van bridge | `open` / `closed` (retained) |
| `somfy/awning/availability` | van bridge | `online` / `offline` (Last Will) |

De gepubliceerde state is altijd een **aanname** op basis van het laatst
gegeven commando (via Home Assistant, de BOOT-knop, of — experimenteel —
gedetecteerde signalen van je originele Situo). io-homecontrol 1W geeft geen
echte positieterugkoppeling van de motor; een absolute percentagestand is
dus niet beschikbaar.

## RX-sniffing: meeluisteren met de originele Situo (experimenteel)

De bridge luistert ook passief mee op 868,95 MHz om te detecteren wanneer
je de **originele Situo** gebruikt, en werkt dan zelf de getoonde stand bij
(zowel OLED als Home Assistant) — zonder dat de bridge zelf iets zendt.

**Hoe het werkt:** de SX1262 ondersteunt geen ruwe ontvangstmodus, dus wordt
het hardware-syncwoord van de radio gezet op een deel van ons eigen
UART-verpakte bit-patroon (`0x7F 0xD9`) om de ontvangst te triggeren; de rest
wordt in software gedecodeerd en tegen de CRC gevalideerd.

- Er wordt alleen gereageerd op frames van het **specifieke adres van jouw
  eigen afstandsbediening**, ingesteld via `SOMFY_REMOTE_SRC` in
  `include/secrets.h` (niet in Git, dus elke fork/gebruiker vult zijn eigen
  adres in) — een naburige Somfy-afstandsbediening in de buurt beïnvloedt de
  status dus niet.
- Foutief-herkende ruis wordt genegeerd dankzij de CRC-check; in het slechtste
  geval toont het scherm een keer de verkeerde stand, maar er wordt nooit
  een commando naar de motor gestuurd op basis van wat ontvangen is.
- **Zo vind je jouw eigen adres:** flash de firmware met de placeholder-waarde
  uit `secrets.h.example`, open de serial monitor, druk op je eigen
  afstandsbediening, en lees het `src`-veld uit de `[RX]`-logregels. Vul dat
  in bij `SOMFY_REMOTE_SRC` in je eigen `secrets.h` en flash opnieuw.

## Projectstructuur

```
somfy-iohc-bridge/
├── platformio.ini          Board/library-configuratie
├── include/
│   ├── secrets.h.example   Sjabloon voor WiFi/MQTT-config
│   └── secrets.h           Jouw eigen config (niet in Git)
├── lib/
│   ├── iohc/                io-homecontrol protocollaag (hardware-onafhankelijk, host-testbaar)
│   │   ├── iohc_crc.h        CRC-16 (geverifieerd tegen echte capture)
│   │   ├── iohc_crypto.*     AES key-transfer, 1W-authenticatie-IV
│   │   ├── iohc_frame.*      Generieke frame-header opbouw/CRC
│   │   ├── iohc_commands.*   Somfy-specifieke pairing/knopframes
│   │   ├── iohc_phy.*        UART-bitframing (encode + decode)
│   │   └── iohc_constants.h  Geverifieerde protocolconstanten (met bronvermelding)
│   ├── storage/nvs_store.*  Persistente opslag van pairing-identiteit (ESP32 NVS)
│   └── ui/display.*         OLED-statusscherm
├── src/
│   ├── main.cpp             Opstartvolgorde, self-tests, BOOT-knop
│   ├── tx_control.*         Pairing- en knop-verzending (radio TX)
│   ├── rx_control.*         Passieve ontvangst/sniffing (radio RX)
│   └── net_control.*        WiFi, MQTT, Home Assistant discovery
└── test/                    Host-native tests (g++, geen hardware nodig)
```

**Host-tests draaien** (valideert protocollogica zonder ESP32):

```bash
cd test
g++ -std=c++17 -I ../lib/iohc -o /tmp/t test_crc.cpp && /tmp/t
g++ -std=c++17 -I ../lib/iohc -o /tmp/t test_crypto.cpp ../lib/iohc/iohc_crypto.cpp -lcrypto && /tmp/t
g++ -std=c++17 -I ../lib/iohc -o /tmp/t test_frame.cpp ../lib/iohc/iohc_frame.cpp && /tmp/t
g++ -std=c++17 -I ../lib/iohc -o /tmp/t test_commands.cpp ../lib/iohc/iohc_commands.cpp ../lib/iohc/iohc_frame.cpp ../lib/iohc/iohc_crypto.cpp -lcrypto && /tmp/t
g++ -std=c++17 -I ../lib/iohc -o /tmp/t test_phy.cpp ../lib/iohc/iohc_phy.cpp && /tmp/t
g++ -std=c++17 -I ../lib/iohc -o /tmp/t test_phy_decode.cpp ../lib/iohc/iohc_phy.cpp ../lib/iohc/iohc_commands.cpp ../lib/iohc/iohc_frame.cpp ../lib/iohc/iohc_crypto.cpp -lcrypto && /tmp/t
```

## Troubleshooting

**Build faalt met "secrets.h not found"**
→ `cp include/secrets.h.example include/secrets.h` en vul je gegevens in.

**Board niet gevonden op `/dev/cu.usbserial-*`**
→ Controleer de USB-kabel (sommige zijn alleen voor opladen). Op macOS kan
een CH9102/CP210x-driver nodig zijn — meestal automatisch geïnstalleerd.

**Radio-init faalt (`[RADIO] beginFSK(...) FAILED`)**
→ Controleer de pin-`build_flags` in `platformio.ini` tegen je exacte
boardrevisie. Een losse verbinding tussen ESP32 en SX1262 (zeldzaam, maar
mogelijk bij een defect board) geeft dezelfde foutmelding.

**Crypto- of frame-self-test faalt bij opstarten**
→ Dit zou nooit mogen gebeuren op ongewijzigde code; het duidt op
gecorrumpeerde flash of een verkeerd gecompileerde library-versie. Wis de
flash volledig (`pio run -t erase`) en flash opnieuw.

**Pairing lijkt te lukken, maar de motor beweegt niet op een test-commando**
→ Vaak: het PROG-tijdvenster was al verlopen toen de bridge zond, of de
motor stond niet echt in leermodus. Herhaal de pairing-procedure. Er is
geen risico in het opnieuw proberen.

**Home Assistant toont geen "Somfy Bridge"-apparaat**
→ Controleer of HA's **MQTT-integratie** (niet alleen de Mosquitto-broker
zelf) is toegevoegd en verbonden. Controleer de serial-log voor
`[MQTT] Verbonden` en `[MQTT] Discovery-config gepubliceerd`. Als de
publish faalt: waarschijnlijk een MQTT-bufferprobleem — zie
`MQTT_MAX_PACKET_SIZE` in `platformio.ini`.

**Home Assistant en de werkelijke stand lopen uit elkaar**
→ Verwacht gedrag als je de originele Situo gebruikt buiten het bereik van
de RX-sniffing-functie (bijv. als die net een pakket mist), of als het
zonnescherm ooit handmatig verplaatst is. Stuur eenmalig het juiste
commando (Open/Close) via Home Assistant of de BOOT-knop om te corrigeren.

## Recovery: pairing/teller kwijt

Als de opgeslagen identiteit ooit corrupt raakt of je wilt gewoon opnieuw
beginnen:

1. Wis de NVS-namespace: makkelijkste manier is de flash volledig wissen
   (`pio run -t erase`, gevolgd door opnieuw flashen), of voeg tijdelijk een
   aanroep van `store::clear()` toe in `setup()`, flash, en verwijder die
   regel weer.
2. Herhaal de normale pairing-procedure (zie hierboven) — je motor onthoudt
   zijn eigen sleutel-slots dus prima meerdere pairings van dezelfde bridge
   na elkaar; er ontstaat geen dubbele/verweesde koppeling zolang je niet
   telkens een nieuw willekeurig NodeID kiest zonder de oude te verwijderen
   (in de praktijk vult dit gewoon één slot van je motor, meestal geen probleem
   bij een enkel apparaat).

De **sequence-counter** wordt bij elke zending direct naar NVS geschreven,
dus een crash of stroomuitval halverwege een zending veroorzaakt geen
desynchronisatie met de motor — in het ergste geval wordt een sequence-
nummer overgeslagen, wat io-homecontrol prima toestaat (nummers hoeven niet
strikt opeenvolgend te zijn, alleen niet hergebruikt).

## Credits

Dit is **geen fork** — dit project deelt geen Git-geschiedenis en geen
gekopieerde broncode met andere projecten, en is van de grond af opnieuw
geschreven. De protocolwaarden in `lib/iohc/` zijn wel geverifieerd tegen
publieke bronnen, met dank aan:

- **[iown-home](https://github.com/rspaargaren/iown-home)** (rspaargaren) —
  protocoldocumentatie (`docs/radio.md`, `docs/linklayer.md`, `docs/commands.md`)
  met echte gesnifte voorbeeldframes, gebruikt als primaire referentie. Let
  op: hun eigen `iohome_constants.h`-broncode bevat minstens twee fouten
  t.o.v. hun eigen documentatie (zie bronvermelding in
  `lib/iohc/iohc_constants.h`) — daarom is er bewust NIET blind van
  overgenomen, alleen van de documentatie/voorbeeldframes.
- **[samr037/iohc-flipper](https://github.com/samr037/iohc-flipper)**
  (Apache-2.0) — hardware-gevalideerde Flipper Zero-implementatie, gebruikt
  om de knop-/pairingframe-opbouw en de vendor/manufacturer-ID's onafhankelijk
  te bevestigen. Eén constante (`IOHC_VENDOR_SOMFY`) is daarmee herleidbaar
  tot `rspaargaren/iown-homecontrol-esp32sx1276` (eveneens Apache-2.0) — zie
  de bronvermelding in `lib/iohc/iohc_constants.h` voor de precieze herkomst.

Beide bronnen worden per-constante/per-functie geciteerd in de
codecommentaren van `lib/iohc/`, zodat elke waarde herleidbaar is.

## Belangrijkste beperkingen (expliciet, niet verzwegen)

- **Geen echte positieterugkoppeling.** io-homecontrol 1W is eenrichtingsverkeer; alle standweergave is een aanname.
- **RX-sniffing is experimenteel** en specifiek aan dit ene Situo-adres gekoppeld — bij vervanging van de afstandsbediening moet `KNOWN_REMOTE_SRC` aangepast worden.
- **Eén apparaat per bridge op dit moment.** De code zendt altijd naar het broadcast-adres; meerdere onafhankelijk bestuurbare motoren vereist een uitbreiding (apparatenregister + per-apparaat MQTT-topics).
