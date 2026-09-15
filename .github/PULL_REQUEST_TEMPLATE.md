## What does this change?

## Why?

## How was this verified?

- [ ] Ran the host-native test suite (`test/*.cpp`) and all tests pass
- [ ] `pio run` still compiles
- [ ] If `lib/iohc/` was touched: added/updated a test backed by a real
      captured frame or a cited reference implementation (see
      [CONTRIBUTING.md](../CONTRIBUTING.md))
- [ ] Tested on real hardware against a real motor (describe below), if applicable

## Checklist

- [ ] No hardcoded protocol constants without a source citation
- [ ] No changes that enable replay attacks, brute-forcing, or bypassing io-homecontrol security (see [SECURITY.md](../SECURITY.md))
- [ ] No personal data (WiFi/MQTT credentials, real remote addresses) included in the diff
