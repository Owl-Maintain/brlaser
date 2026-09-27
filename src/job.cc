// This file is part of the brlaser printer driver.
//
// Copyright 2013 Peter De Wachter
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

#include "job.h"
#include <assert.h>
#include <algorithm>
#include <vector>
#include "line.h"
#include "block.h"
#include "status.h"


job::job(FILE *out, const std::string &job_name)
    : out_(out),
      job_name_(job_name),
      page_params_(),
      pages_(0),
      live_status_(false),
      status_(nullptr) {
  // Delete dubious characters from job name
  std::replace_if(job_name_.begin(), job_name_.end(), [](char c) {
      return c < 32 || c >= 127 || c == '"' || c == '\\';
    }, ' ');
}

job::~job() {
  if (pages_ != 0) {
    end_job();
  }
  delete status_;
}

void job::begin_job() {
  for (int i = 0; i < 128; ++i) {
    putc(0, out_);
  }
  fprintf(out_, "\033%%-12345X@PJL\n");
  if (!user_.empty()) {
    // Same job identity lines as Brother's filter, so the printer's job log
    // and status monitor show who printed what.
    fprintf(out_, "@PJL SET JOBNAME=\"%s\"\n", title_.c_str());
    fprintf(out_, "@PJL SET USERNAME=\"%s\"\n", user_.c_str());
  }
  fprintf(out_, "@PJL JOB NAME=\"%s\"\n", job_name_.c_str());
  if (!user_.empty()) {
    fprintf(out_, "@PJL SET LOGINUSER=\"%s\"\n", user_.c_str());
  }
  if (live_status_) {
    fputs("@PJL USTATUS DEVICE=ON\n@PJL USTATUS JOB=ON\n", out_);
  }
}

void job::end_job() {
  fprintf(out_, "\033%%-12345X@PJL\n");
  fprintf(out_, "@PJL EOJ NAME=\"%s\"\n", job_name_.c_str());
  if (live_status_) {
    fputs("@PJL USTATUS DEVICE=OFF\n@PJL USTATUS JOB=OFF\n@PJL INFO PAGECOUNT\n", out_);
  }
  fprintf(out_, "\033%%-12345X\n");
  fflush(out_);
  if (live_status_ && status_) {
    // The HL-L2300D never sends USTATUS JOB END, so only drain what has
    // already arrived; the USB backend waits for completion anyway.
    status_->wait_for_job_end(job_name_, 2.0);
  }
}

void job::write_page_header() {
  fprintf(out_, "\033%%-12345X@PJL\n");
  if (page_params_.hq1200) {
    // Brother's HL-L2300D filter keeps RESOLUTION at 600 and switches the
    // PCL raster mode to 1032 instead.
    fprintf(out_, "@PJL SET RESOLUTION = 600\n");
  } else if (page_params_.ras1200) {
    fprintf(out_, "@PJL SET RAS1200MODE = TRUE\n");
    fprintf(out_, "@PJL SET RESOLUTION = 600\n");
  } else {
    fprintf(out_, "@PJL SET RAS1200MODE = FALSE\n");
    fprintf(out_, "@PJL SET RESOLUTION = %d\n",
            page_params_.resolution);
    if (page_params_.resolution == 1200) {
      fprintf(out_, "@PJL SET PAPERFEEDSPEED = %s\n",
              page_params_.page_speed ? "FULL" : "HALF");
    }
  }
  fprintf(out_, "@PJL SET ECONOMODE = %s\n",
          page_params_.economode ? "ON" : "OFF");
  fprintf(out_, "@PJL SET SOURCETRAY = %s\n",
          page_params_.sourcetray.c_str());
  fprintf(out_, "@PJL SET MEDIATYPE = %s\n",
          page_params_.mediatype.c_str());
  if (page_params_.vendor_profile == 1) {
    // Brother's own HL-L2300D-family filter always sends these three.
    fprintf(out_, "@PJL SET LESSPAPERCURL=%s\n",
            page_params_.improve_output == 1 ? "ON" : "OFF");
    fprintf(out_, "@PJL SET FIXINTENSITYUP=%s\n",
            page_params_.improve_output == 2 ? "ON" : "OFF");
    fprintf(out_, "@PJL SET TRANSFERLEVELUP=ON\n");
    fprintf(out_, "@PJL TRANSFERLEVEL=0\n");
  }
  if (page_params_.sleep_minutes > 0) {
    fprintf(out_, "@PJL DEFAULT AUTOSLEEP = ON\n");
    fprintf(out_, "@PJL DEFAULT TIMEOUTSLEEP = %d\n", page_params_.sleep_minutes);
    fprintf(out_, "@PJL SET AUTOSLEEP = ON\n");
    fprintf(out_, "@PJL SET TIMEOUTSLEEP = %d\n", page_params_.sleep_minutes);
  }
  fprintf(out_, "@PJL SET DENSITY=%d\n",
          page_params_.density_adjust);
  fprintf(out_, "@PJL SET DEVBIASADJUST=%d\n",
          page_params_.density_adjust);
  fprintf(out_, "@PJL SET PAPER = %s\n",
          page_params_.papersize.c_str());
  fprintf(out_, "@PJL SET PAGEPROTECT = AUTO\n");
  fprintf(out_, "@PJL SET ORIENTATION = PORTRAIT\n");
  fprintf(out_, "@PJL ENTER LANGUAGE = PCL\n");

  fputs("\033E", out_);
  fprintf(out_, "\033&l%dX",
          std::max(1, page_params_.num_copies));

  if (page_params_.vendor_profile == 1) {
    if (page_params_.media_position == 5) {
      fputs("\033&l2H", out_);      // manual feed slot
    } else if (page_params_.media_position == 0) {
      fputs("\033&l7H", out_);      // auto select
    }
  }

  if (!page_params_.hq1200) {
    fprintf(out_, "\033&u%dD",
            page_params_.resolution);
    fprintf(out_, "\033*t%dR",
            page_params_.ras1200 ? 600 : page_params_.resolution);
  }

  if (page_params_.vendor_profile == 1) {
    // Brother's own filter: 1S = long-edge (no page rotation), 2S = short-edge.
    if (page_params_.duplex && !page_params_.tumble) fputs("\033&l1S", out_);
    else if (page_params_.duplex) fputs("\033&l2S", out_);
    else fputs("\033&l0S", out_);
  } else if (page_params_.duplex || page_params_.tumble) {
    fputs("\033&l2S", out_);
  } else {
    fputs("\033&l0S", out_);
  }
}

bool job::encode_page(const page_params &page_params,
                      int lines,
                      int linesize,
                      nextline_fn nextline) {
  if (pages_ == 0) {
    live_status_ = page_params.live_status;
    user_ = page_params.user;
    title_ = page_params.title;
    if (live_status_) status_ = new pjl_status();
    begin_job();
  }
  ++pages_;

  const bool need_header = !(page_params_ == page_params);

  std::vector<uint8_t> line(linesize);
  std::vector<uint8_t> reference(linesize);
  block block;

  // Brother's HL-L2300D-family filter uses 128 lines per band.
  const int lines_per_band = page_params.vendor_profile == 1 ? 128 : 64;

  // Optional blank-page skipping needs the whole page before deciding.
  std::vector<std::vector<uint8_t>> page;
  int line_index = 0;
  auto fetch = [&](std::vector<uint8_t> &buf) -> bool {
    if (!page_params.skip_blank) {
      return nextline(buf);
    }
    if (line_index < static_cast<int>(page.size())) {
      buf = page[line_index++];
      return true;
    }
    return false;
  };
  if (page_params.skip_blank) {
    page.reserve(lines);
    bool blank = true;
    for (int i = 0; i < lines && nextline(line); ++i) {
      if (blank) {
        blank = std::all_of(line.begin(), line.end(), [](uint8_t b) { return b == 0; });
      }
      page.push_back(line);
    }
    if (blank) {
      fprintf(stderr, "INFO: brlaser: skipping blank page %d\n", pages_);
      --pages_;
      return false;
    }
  }

  if (!fetch(line)) {
    return false;
  }
  if (need_header) {
    page_params_ = page_params;
    write_page_header();
  }

  if (page_params.hq1200) {
    // Mode 1032: rows are 16-bit units, three zero pad units on the left
    // (Brother does the same), one 0x30 marker entry before every
    // non-blank row and two 0xFF entries for a blank row.  The band header
    // counts raster rows, so a band of 128 rows carries 256 entries.
    const int pad = 3;
    const int units = pad + (linesize + 1) / 2;
    auto to_units = [&](const std::vector<uint8_t> &src, std::vector<uint16_t> &dst) {
      dst.assign(units, 0);
      for (int i = 0; i + 1 < linesize; i += 2)
        dst[pad + i / 2] = (src[i] << 8) | src[i + 1];
      if (linesize % 2)
        dst[pad + linesize / 2] = src[linesize - 1] << 8;
    };
    std::vector<uint16_t> uline, uref;
    fputs("\033*b1032m", out_);
    to_units(line, uline);
    int rows_in_band = 0;
    bool band_start = true;
    for (int i = 0; ; ++i) {
      std::vector<uint8_t> encoded;
      bool blank = std::none_of(uline.begin(), uline.end(), [](uint16_t u) { return u; });
      if (band_start) {
        encoded = encode_line16(uline);
      } else {
        encoded = encode_line16(uline, uref);
        if (!block.line_fits(encoded.size() + 1)) {
          block.flush_rows(out_, rows_in_band);
          rows_in_band = 0;
          encoded = encode_line16(uline);
        }
      }
      if (blank) {
        block.add_line(std::vector<uint8_t>(1, 0xFF));
        block.add_line(std::vector<uint8_t>(1, 0xFF));
      } else {
        block.add_line(std::vector<uint8_t>(1, 0x30));
        block.add_line(std::move(encoded));
      }
      ++rows_in_band;
      band_start = false;
      std::swap(uline, uref);
      if (rows_in_band == lines_per_band) {
        block.flush_rows(out_, rows_in_band);
        if (status_) status_->poll();
        rows_in_band = 0;
        band_start = true;
      }
      if (i + 1 >= lines || !fetch(line)) break;
      to_units(line, uline);
    }
    block.flush_rows(out_, rows_in_band);
    fputs("1032M\f", out_);
    fflush(out_);
    return true;
  }

  block.add_line(encode_line(line));
  std::swap(line, reference);

  fputs("\033*b1030m", out_);

  for (int i = 1; i < lines && fetch(line); ++i) {
    std::vector<uint8_t> encoded;
    if (i % lines_per_band == 0) {
      block.flush(out_);
      if (status_) status_->poll();
      encoded = encode_line(line);
    } else {
      encoded = encode_line(line, reference);
      if (!block.line_fits(encoded.size())) {
        block.flush(out_);
        encoded = encode_line(line);
      }
    }
    block.add_line(std::move(encoded));
    std::swap(line, reference);
  }

  block.flush(out_);
  fputs("1030M\f", out_);
  fflush(out_);
  return true;
}
