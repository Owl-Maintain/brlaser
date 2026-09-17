// This file is part of the brlaser printer driver.
//
// Copyright 2026 jessssssux
//
// brlaser is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// brlaser is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with brlaser.  If not, see <http://www.gnu.org/licenses/>.

#include "lest.hpp"
#include <string.h>
#include <cups/raster.h>
#include "../src/params.h"

static cups_page_header2_t header(const char *size, const char *media, int slot, int xdpi = 600) {
  cups_page_header2_t h;
  memset(&h, 0, sizeof h);
  strncpy(h.cupsPageSizeName, size, sizeof h.cupsPageSizeName - 1);
  strncpy(h.MediaType, media, sizeof h.MediaType - 1);
  h.MediaPosition = slot;
  h.HWResolution[0] = h.HWResolution[1] = xdpi;
  h.NumCopies = 1;
  h.cupsInteger[11] = 100;
  return h;
}

const lest::test specification[] = {
  "Generic profile passes media names through and knows the classic sizes",
  [] {
    page_params p = build_page_params(header("EnvDL", "THICKER", 5), 0, "", "");
    EXPECT(p.mediatype == "THICKER");
    EXPECT(p.papersize == "DL");
    EXPECT(p.sourcetray == "MANUAL");
    EXPECT(p.vendor_profile == 0);
    EXPECT(!p.live_status);
  },

  "Brother profile translates media names to Brother's keywords",
  [] {
    EXPECT(build_page_params(header("A4", "PLAIN", 0), 1, "", "").mediatype == "REGULAR");
    EXPECT(build_page_params(header("A4", "THICKER", 0), 1, "", "").mediatype == "THICK2");
    EXPECT(build_page_params(header("A4", "ENV", 0), 1, "", "").mediatype == "ENVELOPES");
    EXPECT(build_page_params(header("A4", "ENV-THICK", 0), 1, "", "").mediatype == "ENVTHICK");
    EXPECT(build_page_params(header("A4", "ENV-THIN", 0), 1, "", "").mediatype == "ENVTHIN");
    EXPECT(build_page_params(header("A4", "LABEL", 0), 1, "", "").mediatype == "LABEL");
  },

  "Brother paper names for the added sizes",
  [] {
    EXPECT(build_page_params(header("A5Rotated", "PLAIN", 0), 1, "", "").papersize == "A5L");
    EXPECT(build_page_params(header("FanFoldGermanLegal", "PLAIN", 0), 1, "", "").papersize == "FOLIO");
    EXPECT(build_page_params(header("3x5", "PLAIN", 0), 1, "", "").papersize == "P3X5");
    EXPECT(build_page_params(header("Env10", "PLAIN", 0), 1, "", "").papersize == "COM10");
    EXPECT(build_page_params(header("ISOB5", "PLAIN", 0), 1, "", "").papersize == "B5");
    EXPECT(build_page_params(header("Unknown", "PLAIN", 0), 1, "", "").papersize == "A4");
  },

  "HQ1200 Brother mode needs 1200 dpi, page speed 3 and the profile",
  [] {
    cups_page_header2_t h = header("A4", "PLAIN", 0, 1200);
    h.cupsInteger[12] = 3;
    EXPECT(build_page_params(h, 1, "", "").hq1200);
    EXPECT(!build_page_params(h, 0, "", "").hq1200);
    h.cupsInteger[12] = 2;
    page_params p = build_page_params(h, 1, "", "");
    EXPECT(!p.hq1200);
    EXPECT(p.ras1200);
  },

  "Options are read from the cupsInteger slots",
  [] {
    cups_page_header2_t h = header("A4", "PLAIN", 0);
    h.cupsInteger[9] = 1; h.cupsInteger[10] = 1; h.cupsInteger[11] = 103;
    h.cupsInteger[14] = 2; h.cupsInteger[15] = 5;
    page_params p = build_page_params(h, 1, "jess", "doc");
    EXPECT(p.skip_blank); EXPECT(p.economode); EXPECT(p.density_adjust == 3);
    EXPECT(p.improve_output == 2); EXPECT(p.sleep_minutes == 5);
    EXPECT(p.user == "jess"); EXPECT(p.title == "doc"); EXPECT(p.live_status);
  },
};

int main() { return lest::run(specification); }
