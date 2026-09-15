# Contributing

Thanks for considering a contribution to Somfy Bridge — a local, cloud-free
ESP32 bridge between Somfy io-homecontrol and Home Assistant.

## Ground rules (non-negotiable)

This project only supports **authorized use of your own equipment**:

- No replay attacks, no brute-forcing of keys/addresses, no bypassing of
  io-homecontrol's encryption or authentication.
- Pairing must go through the normal PROG procedure, as a legitimately
  authorized new remote — exactly like a real Somfy accessory would.
- No hardcoded or invented protocol parameters. Every constant in
  `lib/iohc/` must cite a verifiable source (public documentation, a
  hardware-validated reference implementation, or a real captured/decoded
  frame) — see the existing comments in `lib/iohc/iohc_constants.h` for the
  expected citation style.
- Uncertain or unverified values must be flagged explicitly in a comment,
  never silently assumed.

Pull requests that weaken these constraints (e.g. adding brute-force key
recovery, or removing source citations) will not be merged.

## Development setup

- **Firmware:** [PlatformIO](https://platformio.org/), target board
  `heltec_wifi_lora_32_V3`. `cp include/secrets.h.example include/secrets.h`,
  fill in placeholder values, then `pio run`.
- **Protocol logic (`lib/iohc/`)** is hardware-independent and can be built
  and tested on the host with plain `g++` — no ESP32 required. See the
  `test/` directory and the [README](README.md#project-structure) for the
  exact build commands. CI ([`.github/workflows/tests.yml`](.github/workflows/tests.yml))
  runs the full host test suite plus a firmware build on every push and PR.

## Submitting a change

1. Fork the repo and create a branch off `master`.
2. If you touch `lib/iohc/`, add or update a host-native test in `test/`
   that proves the change against a real captured frame or a cited
   reference implementation — not just against your own new code.
3. Run the full host test suite locally and make sure `pio run` still
   compiles before opening a PR.
4. Describe in the PR what you verified the change against (a capture, a
   doc section, a reference implementation) — "it works on my motor" is
   not sufficient justification for a protocol change.

## Reporting bugs / requesting features

Please use the issue templates. For anything security-relevant, see
[SECURITY.md](SECURITY.md) instead of opening a public issue.

## Translations

Code comments and Serial/OLED strings are in English; the README is
maintained in both Dutch ([README.md](README.md)) and English
([README.en.md](README.en.md)). Contributions adding further language
translations of the README are welcome as a new `README.<lang>.md` file
linked from the language switcher at the top of the existing two.
