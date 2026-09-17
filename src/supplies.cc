#include "supplies.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <string>
#include <sys/stat.h>
#include "config.h"

#ifndef BRLASER_STATE_DIR
#define BRLASER_STATE_DIR "/var/lib/brlaser"
#endif

namespace supplies {

static const long kDefaultTonerYield = 1200;   // TN-2330 rating
static const long kDefaultDrumYield = 12000;   // DR-2325 rating

static std::string state_dir() {
  const char *env = getenv("BRLASER_STATE_DIR");   // tests and packagers may override
  return env && *env ? env : BRLASER_STATE_DIR;
}

const char *state_file() {
  static std::string path;
  path = state_dir() + "/supplies.conf";
  return path.c_str();
}

typedef std::map<std::string, long> conf_t;

static conf_t load() {
  conf_t c;
  FILE *f = fopen(state_file(), "r");
  if (!f) return c;
  char line[256];
  while (fgets(line, sizeof(line), f)) {
    char *eq = strchr(line, '=');
    if (!eq || line[0] == '#') continue;
    *eq = 0;
    c[line] = atol(eq + 1);
  }
  fclose(f);
  return c;
}

static bool save(const conf_t &c) {
  mkdir(state_dir().c_str(), 0775);
  std::string tmp = std::string(state_file()) + ".tmp";
  FILE *f = fopen(tmp.c_str(), "w");
  if (!f) {
    fprintf(stderr, "DEBUG: brlaser: cannot write %s\n", tmp.c_str());
    return false;
  }
  fprintf(f, "# brlaser supply baselines; edit with brlaser-supplies\n");
  for (auto &kv : c) fprintf(f, "%s=%ld\n", kv.first.c_str(), kv.second);
  fclose(f);
  chmod(tmp.c_str(), 0664);
  return rename(tmp.c_str(), state_file()) == 0;
}

static long get(const conf_t &c, const char *k, long d = -1) {
  auto it = c.find(k);
  return it == c.end() ? d : it->second;
}

void note_status(const std::string &d, long pagecount) {
  conf_t c = load();
  bool changed = false;
  struct item { const char *name; const char *word; long yield; };
  const item items[] = { {"toner", "toner", kDefaultTonerYield}, {"drum", "drum", kDefaultDrumYield} };
  for (const item &it : items) {
    std::string seen = std::string(it.name) + "_empty_seen";
    bool replace_now = (d.find(std::string("replace ") + it.word) != std::string::npos)
                    || (d.find(std::string(it.word) + " ended") != std::string::npos)
                    || (d.find(std::string(it.word) + " end") != std::string::npos && it.name[0] == 'd');
    if (replace_now) {
      if (get(c, seen.c_str(), 0) != 1) { c[seen] = 1; changed = true; }
    } else if (get(c, seen.c_str(), 0) == 1 && pagecount >= 0 &&
               (d.find("ready") != std::string::npos || d.find("sleep") != std::string::npos)) {
      // Message cleared: a new cartridge went in.  Reset the baseline.
      c[std::string(it.name) + "_pages"] = pagecount;
      if (get(c, (std::string(it.name) + "_yield").c_str()) <= 0)
        c[std::string(it.name) + "_yield"] = it.yield;
      c[seen] = 0;
      changed = true;
      fprintf(stderr, "INFO: brlaser: new %s detected, level estimate reset at page %ld\n", it.name, pagecount);
    }
  }
  if (changed) save(c);
}

static int pct(long now, long base, long yield) {
  if (now < 0 || base < 0 || yield <= 0 || now < base) return -1;
  long left = 100 - ((now - base) * 100) / yield;
  return static_cast<int>(left < 0 ? 0 : (left > 100 ? 100 : left));
}

levels estimate(long pagecount) {
  conf_t c = load();
  levels l;
  l.toner = pct(pagecount, get(c, "toner_pages"), get(c, "toner_yield"));
  l.drum = pct(pagecount, get(c, "drum_pages"), get(c, "drum_yield"));
  if (get(c, "toner_empty_seen", 0) == 1) l.toner = 0;
  if (get(c, "drum_empty_seen", 0) == 1) l.drum = 0;
  return l;
}

void report(long pagecount, bool toner_low, bool toner_empty) {
  levels l = estimate(pagecount);
  if (toner_low && (l.toner < 0 || l.toner > 10)) l.toner = 10;
  if (toner_empty) l.toner = 0;
  fprintf(stderr, "ATTR: marker-colors=#000000,#000000 marker-names=\"Black Toner%s\",\"Drum Unit%s\" marker-types=toner,opc marker-levels=%d,%d marker-high-levels=100,100 marker-low-levels=10,10\n",
          l.toner >= 0 ? " (estimated)" : "", l.drum >= 0 ? " (estimated)" : "", l.toner, l.drum);
  if (l.toner >= 0 && l.toner <= 10) fprintf(stderr, "STATE: +marker-supply-low-warning\n");
  if (l.drum >= 0 && l.drum <= 10) fprintf(stderr, "STATE: +com.brother.drum-warning\n");
  if (pagecount >= 0) fprintf(stderr, "ATTR: printer-alert=\"page-count=%ld\"\n", pagecount);
}

}  // namespace supplies
