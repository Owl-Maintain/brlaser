#include "status.h"
#include <cups/cups.h>
#include <cups/sidechannel.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "supplies.h"
#include <fcntl.h>
#include <unistd.h>

#include <sys/stat.h>
// Under CUPS, fd 3 is the back channel pipe from the backend.  Run by hand,
// fd 3 may be the raster file itself, so only accept a pipe or socket.
static bool have_back_channel() {
  struct stat st;
  if (fstat(3, &st) != 0) return false;
  return S_ISFIFO(st.st_mode) || S_ISSOCK(st.st_mode);
}

pjl_status::pjl_status() : job_ended_(false), pagecount_(-1) {}

std::string pjl_field(const std::string &s, const char *key) {
  size_t p = s.find(key);
  if (p == std::string::npos) return "";
  p += strlen(key);
  size_t e = s.find_first_of("\r\n", p);
  std::string v = s.substr(p, e == std::string::npos ? std::string::npos : e - p);
  if (!v.empty() && v.front() == '"') v.erase(0, 1);
  if (!v.empty() && v.back() == '"') v.pop_back();
  return v;
}

pjl_reasons pjl_interpret(long code, const std::string &display) {
  pjl_reasons r;
  std::string d = display;
  for (auto &ch : d) ch = tolower(ch);
  r.no_paper    = (code >= 41000 && code < 42000) || (code >= 30010 && code <= 30015) || d.find("no paper") != std::string::npos;
  r.cover_open  = (code == 40021) || d.find("cover") != std::string::npos;
  r.jam         = (code == 40022) || (code >= 42000 && code < 43000) || d.find("jam") != std::string::npos;
  r.toner_low   = (code == 10006) || d.find("toner low") != std::string::npos;
  r.toner_empty = d.find("replace toner") != std::string::npos || d.find("toner ended") != std::string::npos;
  r.drum        = d.find("drum") != std::string::npos;
  r.manual_feed = d.find("manual feed") != std::string::npos;
  return r;
}

void pjl_status::handle(const std::string &msg) {
  if (msg.find("PAGECOUNT=") != std::string::npos) {
    pagecount_ = atol(pjl_field(msg, "PAGECOUNT=").c_str());
    return;
  }
  if (msg.find("USTATUS JOB") != std::string::npos) {
    if (msg.find("END") != std::string::npos) job_ended_ = true;
    return;
  }
  std::string code = pjl_field(msg, "CODE=");
  std::string display = pjl_field(msg, "DISPLAY=");
  if (code.empty() || code == last_code_) return;
  last_code_ = code;
  std::string d = display;
  for (auto &ch : d) ch = tolower(ch);
  last_display_ = d;
  pjl_reasons r = pjl_interpret(atol(code.c_str()), display);
  if (r.toner_empty || d.find("replace drum") != std::string::npos) supplies::note_status(d, -1);
  fprintf(stderr, "STATE: %cmedia-empty-warning %ccover-open-warning %cmedia-jam-warning %cmarker-supply-low-warning %cmarker-supply-empty-warning\n",
          r.no_paper ? '+' : '-', r.cover_open ? '+' : '-', r.jam ? '+' : '-', r.toner_low ? '+' : '-', r.toner_empty ? '+' : '-');
  if (!display.empty()) fprintf(stderr, "INFO: %s%s\n", display.c_str(), r.manual_feed ? " - load paper in the manual feed slot" : "");
}

void pjl_status::poll() {
  if (!have_back_channel()) return;
  char tmp[4096];
  ssize_t n;
  while ((n = cupsBackChannelRead(tmp, sizeof(tmp), 0.0)) > 0) {
    buf_.append(tmp, n);
  }
  size_t p;
  while ((p = buf_.find('\f')) != std::string::npos) {
    handle(buf_.substr(0, p));
    buf_.erase(0, p + 1);
  }
}

void pjl_status::wait_for_job_end(const std::string &name, double seconds) {
  if (!have_back_channel()) return;
  time_t end = time(NULL) + static_cast<time_t>(seconds);
  while (!job_ended_ && time(NULL) < end) {
    char tmp[4096];
    ssize_t n = cupsBackChannelRead(tmp, sizeof(tmp), 1.0);
    if (n > 0) buf_.append(tmp, n);
    size_t p;
    while ((p = buf_.find('\f')) != std::string::npos) {
      handle(buf_.substr(0, p));
      buf_.erase(0, p + 1);
    }
  }
  if (job_ended_) fprintf(stderr, "INFO: Printed.\n");
  else fprintf(stderr, "INFO: Sent to printer.\n");
  if (pagecount_ >= 0) {
    supplies::note_status(last_display_, pagecount_);
    supplies::report(pagecount_, false, false);
  }
}
