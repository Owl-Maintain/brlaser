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

// Turns a CUPS page header (plus the vendor profile read from the PPD) into
// the page_params the encoder uses: paper and media names, tray, options.
#ifndef PARAMS_H
#define PARAMS_H
#include <cups/raster.h>
#include <string>
#include "job.h"

page_params build_page_params(const cups_page_header2_t &header, int vendor_profile,
                              const std::string &user, const std::string &title);
#endif
