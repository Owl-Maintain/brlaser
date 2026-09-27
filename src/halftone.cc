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

#include "halftone.h"
#include <algorithm>
#include <cmath>
#include "brlut.h"

// 8x8 clustered-dot (spiral) matrix, values 0..63.
static const uint8_t kCluster[8][8] = {
  {24, 10, 12, 26, 35, 47, 49, 37},
  { 8,  0,  2, 14, 45, 59, 61, 51},
  {22,  6,  4, 16, 43, 57, 63, 53},
  {30, 20, 18, 28, 33, 41, 55, 39},
  {34, 46, 48, 36, 25, 11, 13, 27},
  {44, 58, 60, 50,  9,  1,  3, 15},
  {42, 56, 62, 52, 23,  7,  5, 17},
  {32, 40, 54, 38, 31, 21, 19, 29},
};

halftone::halftone(int mode, unsigned width)
    : mode_(mode), width_(width), row_(0),
      err_cur_(width + 2, 0), err_next_(width + 2, 0) {
  // Dot-gain compensation: a laser engine prints a 50% pattern darker than
  // 50%.  Brother's own filter maps 50% grey to about 38% coverage; this
  // curve gives about 41%.
  for (int i = 0; i < 256; ++i) {
    double v = i / 255.0;
    gamma_[i] = static_cast<uint8_t>(255.0 * std::pow(v, 1.3) + 0.5);
  }
}

uint8_t halftone::brother_pattern(int screen, uint8_t value, int row, int byte_col) {
  // Brother pads three bytes on the left of every line, so the column
  // phase is offset by 3 to match its output.
  return kBrotherLut[screen & 1][value][row & 31][(byte_col + 3) & 3];
}

void halftone::dither(const std::vector<uint8_t> &grey, std::vector<uint8_t> &out) {
  std::fill(out.begin(), out.end(), 0);
  const unsigned w = width_;
  if (mode_ == 1) {
    std::fill(err_next_.begin(), err_next_.end(), 0);
    bool ltr = (row_ % 2) == 0;
    for (unsigned k = 0; k < w; ++k) {
      unsigned x = ltr ? k : w - 1 - k;
      int v = gamma_[grey[x]] + err_cur_[x + 1];
      int o = v >= 128 ? 255 : 0;
      int e = v - o;
      if (o) out[x >> 3] |= 0x80 >> (x & 7);
      if (ltr) {
        err_cur_[x + 2] += e * 7 / 16;
        err_next_[x]     += e * 3 / 16;
        err_next_[x + 1] += e * 5 / 16;
        err_next_[x + 2] += e * 1 / 16;
      } else {
        err_cur_[x]      += e * 7 / 16;
        err_next_[x + 2] += e * 3 / 16;
        err_next_[x + 1] += e * 5 / 16;
        err_next_[x]     += e * 1 / 16;
      }
    }
    std::swap(err_cur_, err_next_);
  } else if (mode_ == 2) {
    for (unsigned x = 0; x < w; ++x) {
      int v = gamma_[grey[x]];
      int t = (kCluster[row_ & 7][x & 7] * 4) + 2;
      if (v > t) out[x >> 3] |= 0x80 >> (x & 7);
    }
  } else if (mode_ == 4 || mode_ == 5) {
    const unsigned bytes = (w + 7) / 8;
    for (unsigned bx = 0; bx < bytes; ++bx) {
      uint8_t acc = 0;
      for (unsigned bit = 0; bit < 8; ++bit) {
        unsigned x = bx * 8 + bit;
        if (x >= w) break;
        acc |= brother_pattern(mode_ - 4, grey[x], row_, bx) & (0x80 >> bit);
      }
      out[bx] = acc;
    }
  } else {
    for (unsigned x = 0; x < w; ++x) {
      if (grey[x] >= 128) out[x >> 3] |= 0x80 >> (x & 7);
    }
  }
  ++row_;
}
