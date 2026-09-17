// This file is part of the brlaser printer driver.
//
// Copyright 2020 Peter De Wachter
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
#include "tempfile.h"
#include "../src/job.h"

#include <string>
#include <vector>

static std::vector<uint8_t> g_lines;
static size_t g_next;
static bool feed(std::vector<uint8_t> &buf) {
  if (g_next + buf.size() > g_lines.size()) return false;
  std::copy(g_lines.begin() + g_next, g_lines.begin() + g_next + buf.size(), buf.begin());
  g_next += buf.size();
  return true;
}

static page_params params(int profile) {
  page_params p = {};
  p.num_copies = 1; p.resolution = 600; p.vendor_profile = profile;
  p.sourcetray = "AUTO"; p.mediatype = profile ? "REGULAR" : "PLAIN"; p.papersize = "A4";
  return p;
}

static std::string run(page_params p, int lines, int linesize, const std::vector<uint8_t> &data) {
  tempfile f;
  g_lines = data; g_next = 0;
  {
    job j(f.file(), "name");
    j.encode_page(p, lines, linesize, feed);
  }
  std::vector<uint8_t> d = f.data();
  return std::string(d.begin(), d.end());
}

static size_t count(const std::string &s, const std::string &needle) {
  size_t n = 0, pos = 0;
  while ((pos = s.find(needle, pos)) != std::string::npos) { ++n; pos += needle.size(); }
  return n;
}

const lest::test specification[] = {
  "An empty job produces no output",
  [] {
    tempfile f;
    {
      job j(f.file(), "name");
    }
    EXPECT(f.data().empty());
  },

  "Generic profile keeps the classic header and 64-line bands",
  [] {
    std::string out = run(params(0), 130, 4, std::vector<uint8_t>(130 * 4, 0xFF));
    EXPECT(out.find("@PJL SET SOURCETRAY = AUTO") != std::string::npos);
    EXPECT(out.find("LESSPAPERCURL") == std::string::npos);
    EXPECT(out.find("\033*b1030m") != std::string::npos);
    // 130 lines -> bands of 64, 64, 2
    EXPECT(count(out, std::string("w\0\x40", 3)) == 2);
  },

  "Brother profile sends Brother's PJL, tray PCL and 128-line bands",
  [] {
    page_params p = params(1);
    p.media_position = 5; p.improve_output = 1; p.sleep_minutes = 5;
    std::string out = run(p, 130, 4, std::vector<uint8_t>(130 * 4, 0xFF));
    EXPECT(out.find("@PJL SET LESSPAPERCURL=ON") != std::string::npos);
    EXPECT(out.find("@PJL SET FIXINTENSITYUP=OFF") != std::string::npos);
    EXPECT(out.find("@PJL SET TRANSFERLEVELUP=ON") != std::string::npos);
    EXPECT(out.find("@PJL SET TIMEOUTSLEEP = 5") != std::string::npos);
    EXPECT(out.find("\033&l2H") != std::string::npos);          // manual feed slot
    EXPECT(count(out, std::string("w\0\x80", 3)) == 1);          // one full 128-line band
  },

  "Brother profile uses native duplex commands",
  [] {
    page_params p = params(1); p.duplex = true; p.tumble = false;
    EXPECT(run(p, 2, 4, std::vector<uint8_t>(8, 1)).find("\033&l1S") != std::string::npos);
    p.tumble = true;
    EXPECT(run(p, 2, 4, std::vector<uint8_t>(8, 1)).find("\033&l2S") != std::string::npos);
    page_params g = params(0); g.duplex = true;
    std::string out = run(g, 2, 4, std::vector<uint8_t>(8, 1));
    EXPECT(out.find("\033&l2S") != std::string::npos);
    EXPECT(out.find("\033&l1S") == std::string::npos);
  },

  "HQ1200 Brother mode: mode 1032, two entries per row, marker before content rows",
  [] {
    page_params p = params(1); p.resolution = 1200; p.hq1200 = true;
    // 3 rows: blank, one black pixel, blank
    std::vector<uint8_t> data(3 * 4, 0); data[4] = 0x80;
    std::string out = run(p, 3, 4, data);
    EXPECT(out.find("\033*b1032m") != std::string::npos);
    EXPECT(out.find("@PJL SET RESOLUTION = 600") != std::string::npos);
    EXPECT(out.find("RAS1200MODE") == std::string::npos);
    size_t b = out.find("\033*b1032m");
    std::string band = out.substr(out.find('w', b) + 1);
    // header: two bytes = row count 3; then ff ff (blank) 30 <row> ff ff (blank)
    EXPECT((unsigned char)band[0] == 0 && (unsigned char)band[1] == 3);
    EXPECT((unsigned char)band[2] == 0xFF && (unsigned char)band[3] == 0xFF);
    EXPECT((unsigned char)band[4] == 0x30);
    EXPECT(band.rfind("\xff\xff\x31\x30\x33\x32M\f") != std::string::npos);  // ends with ff ff 1032M FF
  },

  "Blank pages are skipped when asked and sent otherwise",
  [] {
    page_params p = params(1); p.skip_blank = true;
    std::string out = run(p, 4, 4, std::vector<uint8_t>(16, 0));
    EXPECT(out.find("\033*b") == std::string::npos);      // no raster for the skipped page
    p.skip_blank = false;
    EXPECT(run(p, 4, 4, std::vector<uint8_t>(16, 0)).find("\033*b1030m") != std::string::npos);
  },
};

int main() {
  return lest::run(specification);
}
