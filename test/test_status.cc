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
#include <string>
#include "../src/status.h"

const lest::test specification[] = {
  "PJL fields are extracted without quotes",
  [] {
    std::string r = "@PJL INFO STATUS\r\nCODE=40000\r\nDISPLAY=\"Sleep\"\r\nONLINE=TRUE\r\n\f";
    EXPECT(pjl_field(r, "CODE=") == "40000");
    EXPECT(pjl_field(r, "DISPLAY=") == "Sleep");
    EXPECT(pjl_field(r, "ONLINE=") == "TRUE");
    EXPECT(pjl_field(r, "MISSING=") == "");
  },

  "Ready and Sleep raise no reasons",
  [] {
    pjl_reasons r = pjl_interpret(10001, "Ready");
    EXPECT(!r.no_paper); EXPECT(!r.cover_open); EXPECT(!r.jam);
    EXPECT(!r.toner_low); EXPECT(!r.toner_empty);
    r = pjl_interpret(40000, "Sleep");
    EXPECT(!r.no_paper);
  },

  "HP-style codes map to reasons",
  [] {
    EXPECT(pjl_interpret(41001, "").no_paper);
    EXPECT(pjl_interpret(40021, "").cover_open);
    EXPECT(pjl_interpret(40022, "").jam);
    EXPECT(pjl_interpret(10006, "").toner_low);
  },

  "Brother display wording maps to reasons regardless of case",
  [] {
    EXPECT(pjl_interpret(0, "No Paper").no_paper);
    EXPECT(pjl_interpret(0, "Cover is Open").cover_open);
    EXPECT(pjl_interpret(0, "Jam Inside").jam);
    EXPECT(pjl_interpret(0, "Toner Low").toner_low);
    EXPECT(pjl_interpret(0, "Replace Toner").toner_empty);
    EXPECT(pjl_interpret(0, "Drum End Soon").drum);
    EXPECT(pjl_interpret(0, "Manual Feed").manual_feed);
  },
};

int main() { return lest::run(specification); }
