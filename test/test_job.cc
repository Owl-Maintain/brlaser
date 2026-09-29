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
#include <algorithm>
#include <string>
#include "tempfile.h"
#include "../src/job.h"

namespace {

bool blank_line(std::vector<uint8_t> &buf) {
  std::fill(buf.begin(), buf.end(), 0);
  return true;
}

page_params test_params(int resolution, int page_speed, bool ras1200) {
  page_params p = { };
  p.num_copies = 1;
  p.resolution = resolution;
  p.page_speed = page_speed;
  p.ras1200 = ras1200;
  p.sourcetray = "AUTO";
  p.mediatype = "PLAIN";
  p.papersize = "A4";
  return p;
}

std::string encoded_page(const page_params &p) {
  tempfile f;
  {
    job j(f.file(), "name");
    j.encode_page(p, 1, 1, blank_line);
  }
  std::vector<uint8_t> data = f.data();
  return std::string(data.begin(), data.end());
}

}  // namespace

const lest::test specification[] = {
  "An empty job produces no output",
  [] {
    tempfile f;
    {
      job j(f.file(), "name");
    }
    EXPECT(f.data().empty());
  },

  "RAS1200 page headers use 1200dpi paper feed",
  [] {
    std::string data = encoded_page(test_params(1200, 2, true));
    EXPECT(data.find("@PJL SET RAS1200MODE = TRUE\n") != std::string::npos);
    EXPECT(data.find("@PJL SET RESOLUTION = 1200\n") != std::string::npos);
    EXPECT(data.find("@PJL SET PAPERFEEDSPEED = HALF\n") != std::string::npos);
    EXPECT(data.find("@PJL SET RESOLUTION = 600\n") == std::string::npos);
  },
};

int main() {
  return lest::run(specification);
}
