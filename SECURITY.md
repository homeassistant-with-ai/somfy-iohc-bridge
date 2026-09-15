# Security Policy

## Scope and intent

This project pairs a DIY ESP32 device as an **authorized additional
remote** on your own Somfy io-homecontrol motor, via the normal PROG
procedure. It does not, and will not, support:

- Replay attacks
- Brute-forcing of keys, addresses, or sequence numbers
- Bypassing io-homecontrol's encryption or authentication
- Controlling equipment you do not own or have not been given permission to control

If you believe a change to this repository would enable any of the above,
please report it as a security issue rather than opening a public PR or
issue.

## Reporting a vulnerability

Please use GitHub's private vulnerability reporting for this repository
(**Security** tab → **Report a vulnerability**) rather than a public issue,
so any real weakness can be fixed before it's disclosed. This applies to:

- A flaw in this project's own code (e.g. the CRC/AES/frame-building logic in `lib/iohc/`) that could leak your pairing key, sequence counter, or otherwise weaken your bridge's own security
- A way to make the RX-sniffing feature (`src/rx_control.cpp`) react to, or be spoofed by, frames that are not actually from your own paired remote

## Out of scope

- Weaknesses in the io-homecontrol protocol itself, or in Somfy's own
  hardware/cloud services — those are Somfy's to fix, not this project's.
- MQTT/WiFi credential handling issues caused by publishing your own
  `include/secrets.h` (which is `.gitignore`d specifically to prevent this)
  or by using this firmware on an untrusted network.

## Supported versions

This is a small hobby/DIY project without a formal release/support
cycle. Security fixes are applied to the `master` branch; please build
from the latest commit.
