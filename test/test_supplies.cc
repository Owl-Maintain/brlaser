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
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <unistd.h>
#include "../src/supplies.h"

struct tempdir {
  std::string path;
  tempdir() {
    char tmpl[] = "/tmp/brlaser-test-XXXXXX";
    path = mkdtemp(tmpl);
    setenv("BRLASER_STATE_DIR", path.c_str(), 1);
  }
  ~tempdir() {
    unlink((path + "/supplies.conf").c_str());
    rmdir(path.c_str());
  }
};

const lest::test specification[] = {
  "Without a baseline the levels are unknown",
  [] {
    tempdir d;
    supplies::note_status("ready", 1000);
    supplies::levels l = supplies::estimate(1000);
    EXPECT(l.toner == -1);
    EXPECT(l.drum == -1);
  },

  "Replace Toner flags the toner empty until the printer is ready again",
  [] {
    tempdir d;
    supplies::note_status("replace toner", 1500);
    EXPECT(supplies::estimate(1500).toner == 0);
    supplies::note_status("ready", 1503);
    EXPECT(supplies::estimate(1503).toner == 100);
    EXPECT(supplies::estimate(2103).toner == 50);    // default yield 1200 pages
    EXPECT(supplies::estimate(9999).toner == 0);
  },

  "Drum works the same way with its own yield",
  [] {
    tempdir d;
    supplies::note_status("replace drum", 3000);
    supplies::note_status("sleep", 3010);
    EXPECT(supplies::estimate(9010).drum == 50);     // default yield 12000 pages
    EXPECT(supplies::estimate(3010).toner == -1);    // toner untouched
  },

  "A page count before the baseline is reported as unknown, not over 100",
  [] {
    tempdir d;
    supplies::note_status("replace toner", 500);
    supplies::note_status("ready", 500);
    EXPECT(supplies::estimate(400).toner == -1);
  },
};

int main() { return lest::run(specification); }
