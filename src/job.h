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

#ifndef JOB_H
#define JOB_H

#include <stdint.h>
#include <stdio.h>
#include <string>
#include <vector>

struct page_params {
  int num_copies;
  int resolution;
  int page_speed;
  bool ras1200;
  bool hq1200;          // Brother mode 1032 (HL-L2300D family HQ1200)
  bool duplex;
  bool tumble;
  bool economode;
  int density_adjust;
  int vendor_profile;   // 0 = generic brlaser, 1 = Brother HBP (HL-L2300D family)
  int improve_output;   // 0 = off, 1 = less paper curl, 2 = improve toner fixing
  int sleep_minutes;    // 0 = printer default
  bool skip_blank;
  bool live_status;     // poll PJL unsolicited status while printing (profile)
  std::string user;
  std::string title;
  int media_position;
  std::string sourcetray;
  std::string mediatype;
  std::string papersize;

  bool operator==(const page_params &o) const {
    return num_copies == o.num_copies
      && resolution == o.resolution
      && page_speed == o.page_speed
      && ras1200 == o.ras1200
      && hq1200 == o.hq1200
      && duplex == o.duplex
      && tumble == o.tumble
      && economode == o.economode
      && density_adjust == o.density_adjust
      && vendor_profile == o.vendor_profile
      && improve_output == o.improve_output
      && sleep_minutes == o.sleep_minutes
      && skip_blank == o.skip_blank
      && live_status == o.live_status
      && media_position == o.media_position
      && sourcetray == o.sourcetray
      && mediatype == o.mediatype
      && papersize == o.papersize;
  }
};

class job {
 public:
  typedef bool (*nextline_fn)(std::vector<uint8_t> &buf);

  explicit job(FILE *out, const std::string &job_name);
  ~job();

  // Returns false when the page was skipped (blank-page skipping).
  bool encode_page(const page_params &params,
                   int lines,
                   int linesize,
                   nextline_fn nextline);

  int pages() const {
    return pages_;
  }

 private:
  void begin_job();
  void end_job();
  void write_page_header();

  FILE *out_;
  std::string job_name_;
  page_params page_params_;
  int pages_;
  bool live_status_;
  std::string user_, title_;
  class pjl_status *status_;
};

#endif  // JOB_H
