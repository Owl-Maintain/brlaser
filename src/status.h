// Reads PJL unsolicited status from the CUPS back channel while a job is
// being sent and turns it into CUPS STATE:/INFO: messages.
#ifndef STATUS_H
#define STATUS_H
#include <string>

// Value of "KEY=..." in a PJL reply, without surrounding quotes; "" if absent.
std::string pjl_field(const std::string &reply, const char *key);

struct pjl_reasons {
  bool no_paper = false, cover_open = false, jam = false;
  bool toner_low = false, toner_empty = false, drum = false, manual_feed = false;
};
// Map a PJL status code and DISPLAY text (HP-style codes, Brother wording)
// to printer-state reasons.
pjl_reasons pjl_interpret(long code, const std::string &display);

class pjl_status {
 public:
  pjl_status();
  void poll();                                   // non-blocking
  void wait_for_job_end(const std::string &name, double seconds);
 private:
  void handle(const std::string &msg);
  std::string buf_;
  std::string last_code_;
  std::string last_display_;
  bool job_ended_;
  long pagecount_;
};
#endif
