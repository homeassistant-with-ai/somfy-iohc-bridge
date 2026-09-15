#include "nvs_store.h"
#include <Preferences.h>
#include <string.h>

namespace store {

static Preferences prefs;
static const char* NS = "somfy_iohc";

void init() {
  // nothing to do upfront; Preferences opens its own session per call
}

Identity load() {
  Identity id{};
  prefs.begin(NS, /*readOnly=*/true);
  id.paired = prefs.getBool("paired", false);
  if (id.paired) {
    prefs.getBytes("src", id.src, 3);
    prefs.getBytes("key", id.install_key, 16);
    id.seq = prefs.getUShort("seq", 0);
  }
  prefs.end();
  return id;
}

void save(const Identity& identity) {
  prefs.begin(NS, /*readOnly=*/false);
  prefs.putBool("paired", identity.paired);
  prefs.putBytes("src", identity.src, 3);
  prefs.putBytes("key", identity.install_key, 16);
  prefs.putUShort("seq", identity.seq);
  prefs.end();
}

void save_seq(uint16_t seq) {
  // Writes only the changed field (cheaper than rewriting the whole
  // identity on every transmission).
  prefs.begin(NS, /*readOnly=*/false);
  prefs.putUShort("seq", seq);
  prefs.end();
}

void clear() {
  prefs.begin(NS, /*readOnly=*/false);
  prefs.clear();
  prefs.end();
}

} // namespace store
