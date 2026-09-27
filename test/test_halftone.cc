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
#include <stdint.h>
#include <vector>
#include "../src/halftone.h"

static double coverage(int mode, uint8_t grey, unsigned width = 256, int rows = 64) {
  halftone h(mode, width);
  std::vector<uint8_t> in(width, grey), out((width + 7) / 8);
  long ones = 0;
  for (int r = 0; r < rows; ++r) {
    h.dither(in, out);
    for (uint8_t b : out) for (int i = 0; i < 8; ++i) ones += (b >> i) & 1;
  }
  return ones / double(width * rows);
}

const lest::test specification[] = {
  "White stays white and black stays black in every mode",
  [] {
    for (int mode = 1; mode <= 5; ++mode) {
      EXPECT(coverage(mode, 0) == 0.0);
      EXPECT(coverage(mode, 255) == 1.0);
    }
  },

  "Threshold mode switches at 50%",
  [] {
    EXPECT(coverage(3, 127) == 0.0);
    EXPECT(coverage(3, 128) == 1.0);
  },

  "Error diffusion and clustered dot follow the dot-gain curve (mid grey near 40%)",
  [] {
    double d = coverage(1, 128), c = coverage(2, 128);
    EXPECT(d > 0.33); EXPECT(d < 0.50);
    EXPECT(c > 0.33); EXPECT(c < 0.50);
    EXPECT(coverage(1, 64) < coverage(1, 192));
  },

  "Brother screens reproduce the measured tables byte for byte",
  [] {
    // 50% grey, graphics screen: Brother's driver emits 0x3c,0x3c for the
    // first two bytes of an unpadded line (row 0), and 0x38,0x38 on row 1.
    halftone h(4, 16);
    std::vector<uint8_t> in(16, 128), out(2);
    h.dither(in, out);
    EXPECT(out[0] == halftone::brother_pattern(0, 128, 0, 0));
    EXPECT(out[1] == halftone::brother_pattern(0, 128, 0, 1));
    h.dither(in, out);
    EXPECT(out[0] == halftone::brother_pattern(0, 128, 1, 0));
    // the pattern repeats every 32 rows
    EXPECT(halftone::brother_pattern(0, 200, 5, 2) == halftone::brother_pattern(0, 200, 37, 2));
    // and every 4 bytes
    EXPECT(halftone::brother_pattern(1, 90, 7, 1) == halftone::brother_pattern(1, 90, 7, 5));
  },

  "Brother graphics screen is monotonic in grey",
  [] {
    double prev = -1;
    for (int g = 0; g < 256; g += 16) {
      double c = coverage(4, g, 64, 32);
      EXPECT(c >= prev);
      prev = c;
    }
  },
};

int main() { return lest::run(specification); }
