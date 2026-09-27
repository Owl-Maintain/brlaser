// Estimated toner and drum levels for printers that report no percentage.
// State lives in BRLASER_STATE_DIR/supplies.conf (key=value lines):
//   toner_pages, toner_yield, drum_pages, drum_yield   baseline page count and rated yield
//   toner_empty_seen, drum_empty_seen                  1 while the printer says "Replace ..."
#ifndef SUPPLIES_H
#define SUPPLIES_H
#include <string>
namespace supplies {
struct levels { int toner = -1; int drum = -1; };
// Record what the printer's display text says.  When a "Replace Toner"
// (or Drum) message has been seen and the printer is later Ready with the
// message gone, the baseline is reset to the current page count.
// display_lower is the lower-cased DISPLAY text; pagecount may be -1.
void note_status(const std::string &display_lower, long pagecount);
levels estimate(long pagecount);
// CUPS ATTR:/STATE: lines for the levels.
void report(long pagecount, bool toner_low, bool toner_empty);
const char *state_file();
}
#endif
