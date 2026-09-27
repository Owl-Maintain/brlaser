// commandtobrlaser: CUPS command filter for Brother laser printers driven by brlaser.
// Handles ReportLevels (PJL INFO STATUS -> printer-state-reasons, marker attributes)
// and PrintSelfTestPage (PJL EXECUTE TESTPRINT).  GPL-2.0-or-later, same as brlaser.
#include <cups/cups.h>
#include <cups/sidechannel.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <ctype.h>
#include "supplies.h"
#include "status.h"

static std::string pjl_query(const char *q, double timeout) {
  std::string cmd = std::string("\033%-12345X@PJL\n") + q + "\n\033%-12345X\n";
  fwrite(cmd.data(), 1, cmd.size(), stdout);
  fflush(stdout);
  std::string reply;
  char buf[4096];
  double waited = 0;
  while (waited < timeout) {
    ssize_t n = cupsBackChannelRead(buf, sizeof(buf), 0.5);
    waited += 0.5;
    if (n > 0) {
      reply.append(buf, n);
      if (reply.find('\f') != std::string::npos) break;
    }
  }
  return reply;
}

static std::string lower(std::string s) { for (auto &c : s) c = tolower(c); return s; }

static void report_levels() {
  std::string st = pjl_query("@PJL INFO STATUS", 6.0);
  std::string code = pjl_field(st, "CODE=");
  std::string display = pjl_field(st, "DISPLAY=");
  std::string online = pjl_field(st, "ONLINE=");
  fprintf(stderr, "DEBUG: commandtobrlaser status code=%s display=\"%s\" online=%s\n",
          code.c_str(), display.c_str(), online.c_str());
  if (st.empty()) {
    fprintf(stderr, "WARNING: commandtobrlaser: no PJL status reply from printer\n");
    return;
  }
  std::string d = lower(display);
  long c = atol(code.c_str());
  std::vector<std::string> on, off;
  auto set = [&](const char *reason, bool active) { (active ? on : off).push_back(reason); };
  // HP-style PJL status codes, which Brother follows for this family.
  pjl_reasons r = pjl_interpret(c, display);
  bool toner_low = r.toner_low, toner_empty = r.toner_empty, no_paper = r.no_paper;
  bool cover_open = r.cover_open, jam = r.jam, drum = r.drum;
  bool bin_full = (c == 30016) || d.find("output full") != std::string::npos;
  set("marker-supply-low-warning", toner_low);
  set("marker-supply-empty-warning", toner_empty);
  set("media-empty-warning", no_paper);
  set("cover-open-warning", cover_open);
  set("media-jam-warning", jam);
  set("marker-waste-almost-full-warning", false);
  set("output-area-full-warning", bin_full);
  set("com.brother.drum-warning", drum);
  std::string line = "STATE:";
  for (auto &r : on) line += " +" + r;
  for (auto &r : off) line += " -" + r;
  fprintf(stderr, "%s\n", line.c_str());
  std::string pc = pjl_query("@PJL INFO PAGECOUNT", 4.0);
  std::string pages = pjl_field(pc, "PAGECOUNT=");
  long now = pages.empty() ? -1 : atol(pages.c_str());
  supplies::note_status(d, now);
  supplies::report(now, toner_low, toner_empty);
  if (!display.empty())
    fprintf(stderr, "INFO: %s%s\n", display.c_str(), pages.empty() ? "" : (" (page count " + pages + ")").c_str());
  fprintf(stderr, "DEBUG: commandtobrlaser pagecount=%ld state=%s\n", now, supplies::state_file());
}

int main(int argc, char *argv[]) {
  if (argc < 6) {
    fprintf(stderr, "ERROR: commandtobrlaser job-id user title copies options [file]\n");
    return 1;
  }
  FILE *f = argc > 6 ? fopen(argv[6], "r") : stdin;
  if (!f) { fprintf(stderr, "ERROR: commandtobrlaser: cannot open command file\n"); return 1; }
  char buf[1024];
  bool did = false;
  while (fgets(buf, sizeof(buf), f)) {
    if (buf[0] == '#' || buf[0] == '\n') continue;
    buf[strcspn(buf, "\r\n")] = 0;
    if (!strcasecmp(buf, "ReportLevels")) { report_levels(); did = true; }
    else if (!strcasecmp(buf, "PrintSelfTestPage")) {
      fputs("\033%-12345X@PJL\n@PJL EXECUTE TESTPRINT\n\033%-12345X\n", stdout); fflush(stdout);
      fprintf(stderr, "PAGE: 1 1\n"); did = true;
    }
    else fprintf(stderr, "WARNING: commandtobrlaser: unknown command \"%s\"\n", buf);
  }
  if (!did) fprintf(stderr, "INFO: commandtobrlaser: nothing to do\n");
  return 0;
}
