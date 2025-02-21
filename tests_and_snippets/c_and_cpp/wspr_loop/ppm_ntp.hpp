#ifndef PPM_NTP_HPP
#define PPM_NTP_HPP

#include <string>

extern std::string run_command(const std::string &command);
extern bool is_ntp_synchronized();
extern bool check_ntp_status();
extern bool restart_ntp_service();
extern bool ensure_ntp_stable();
extern bool update_ppm();

#endif // PPM_NTP_HPP
