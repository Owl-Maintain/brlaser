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

// In-filter halftoning: 8-bit grey lines to 1-bit packed lines.
//   mode 1  Floyd-Steinberg error diffusion (serpentine)
//   mode 2  8x8 clustered dot
//   mode 3  threshold at 50%
//   mode 4  Brother "Graphics" screen, mode 5 Brother "Text" screen
//           (tables measured from Brother's own driver output)
#ifndef HALFTONE_H
#define HALFTONE_H
#include <stdint.h>
#include <vector>

class halftone {
 public:
  halftone(int mode, unsigned width);
  int mode() const { return mode_; }
  unsigned width() const { return width_; }
  // Dither one grey line (width bytes) into out ((width+7)/8 bytes); advances the row.
  void dither(const std::vector<uint8_t> &grey, std::vector<uint8_t> &out);
  // Brother screen pattern byte for a grey value at a given row and byte column.
  static uint8_t brother_pattern(int screen, uint8_t value, int row, int byte_col);
 private:
  int mode_;
  unsigned width_;
  int row_;
  std::vector<int> err_cur_, err_next_;
  uint8_t gamma_[256];
};
#endif
