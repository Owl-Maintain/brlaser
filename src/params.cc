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

#include "params.h"
#include <array>
#include <map>

page_params build_page_params(const cups_page_header2_t &header, int vendor_profile,
                              const std::string &user, const std::string &title) {
  static const std::array<std::string, 6> sources = {{
    "AUTO", "T1", "T2", "T3", "MP", "MANUAL"
  }};
  static const std::map<std::string, std::string> sizes = {
    { "A4", "A4" },
    { "A5", "A5" },
    { "A6", "A6" },
    { "B5", "B5" },
    { "B6", "B6" },
    { "EnvC5", "C5" },
    { "EnvMonarch", "MONARCH" },
    { "EnvPRC5", "DL" },
    { "EnvDL", "DL" },
    { "Executive", "EXECUTIVE" },
    { "Legal", "LEGAL" },
    { "Letter", "LETTER" },
    // Names taken from Brother's own HL-L2300D-family filter.
    { "A5Rotated", "A5L" },
    { "ISOB5", "B5" },
    { "Env10", "COM10" },
    { "FanFoldGermanLegal", "FOLIO" },
    { "Folio", "FOLIO" },
    { "3x5", "P3X5" }
  };
  // Brother's HBP filter uses different media-type keywords from the
  // generic brlaser PPD; translate when the vendor profile asks for it.
  static const std::map<std::string, std::string> brother_media = {
    { "PLAIN", "REGULAR" },
    { "THICKER", "THICK2" },
    { "ENV", "ENVELOPES" },
    { "ENV-THICK", "ENVTHICK" },
    { "ENV-THIN", "ENVTHIN" }
  };

  page_params p = { };
  p.num_copies = header.NumCopies;
  p.resolution = header.HWResolution[0];
  p.page_speed = header.cupsInteger[12];
  p.hq1200 = (p.resolution == 1200 && p.page_speed == 3 && vendor_profile == 1);
  p.ras1200 = (p.resolution == 1200 && p.page_speed == 2 && !p.hq1200);
  p.economode = header.cupsInteger[10];
  p.density_adjust = (header.cupsInteger[11] - 100);
  p.mediatype = header.MediaType;
  p.vendor_profile = vendor_profile;
  p.improve_output = header.cupsInteger[14];
  p.sleep_minutes = header.cupsInteger[15];
  p.skip_blank = header.cupsInteger[9] != 0;
  p.media_position = header.MediaPosition;
  p.live_status = (vendor_profile == 1);
  p.user = user;
  p.title = title;
  if (p.vendor_profile == 1) {
    auto m = brother_media.find(p.mediatype);
    if (m != brother_media.end())
      p.mediatype = m->second;
  }
  p.duplex = header.Duplex;
  p.tumble = header.Tumble;

  if (header.MediaPosition < sources.size())
    p.sourcetray = sources[header.MediaPosition];
  else
    p.sourcetray = sources[0];

  auto size_it = sizes.find(header.cupsPageSizeName);
  if (size_it != sizes.end())
    p.papersize = size_it->second;
  else
    p.papersize = "A4";

  return p;
}

