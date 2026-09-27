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
#include "../src/line.h"

typedef std::vector<uint8_t> vec;
typedef std::vector<uint16_t> vec16;

static uint8_t sub(uint8_t offset, uint8_t count) { return (offset << 3) | count; }
static uint8_t rep(uint8_t offset, uint8_t count) { return 128 | (offset << 5) | count; }

const lest::test specification[] = {
  "A blank line is a single 0xFF, with or without a reference",
  [] {
    EXPECT(( encode_line16(vec16{0, 0, 0}) == vec{0xFF} ));
    EXPECT(( encode_line16(vec16{0, 0, 0}, vec16{1, 2, 3}) == vec{0xFF} ));
  },

  "An initial line is one substitute of big-endian 16-bit units",
  [] {
    EXPECT(( encode_line16(vec16{0x0102, 0x0304}) == (vec{1, sub(0, 1), 0x01, 0x02, 0x03, 0x04}) ));
  },

  "Offsets and counts are in units, values are two bytes",
  [] {
    // reference identical except unit 3 -> one edit: substitute at offset 3, count 1
    EXPECT(( encode_line16(vec16{0, 0, 0, 0x4000, 0}, vec16{0, 0, 0, 0, 0})
             == (vec{1, sub(3, 0), 0x40, 0x00}) ));
  },

  "A run of equal units becomes a repeat command with a 16-bit value",
  [] {
    vec16 line(40, 0xFFFF);
    vec16 ref(40, 0);
    vec out = encode_line16(line, ref);
    EXPECT(out[0] == 1);
    EXPECT(out[1] == rep(0, 31));      // count_low saturates at 31 ...
    EXPECT(out[2] == 40 - 2 - 31);     // ... and the overflow byte carries the rest
    EXPECT(out[3] == 0xFF);
    EXPECT(out[4] == 0xFF);
    EXPECT(out.size() == 5u);
  },

  "Trailing units equal to the reference are not encoded",
  [] {
    vec16 line{0x0001, 0x0002, 0x0003};
    vec16 ref{0x0000, 0x0002, 0x0003};
    EXPECT(( encode_line16(line, ref) == (vec{1, sub(0, 0), 0x00, 0x01}) ));
  },
};

int main() { return lest::run(specification); }
