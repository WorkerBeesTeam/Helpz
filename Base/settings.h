#ifndef HELPZ_SETTINGS_H
#define HELPZ_SETTINGS_H

#include <string>

namespace Helpz {

class Settings
{
public:
    Settings();

    static std::string get_param_value(const std::string& text, const std::string& param, std::size_t* begin_pos = nullptr, std::size_t* end_pos = nullptr);
    static bool set_param_value(std::string& text, const std::string& param, const std::string& value);
};

} // namespace Helpz

#endif // HELPZ_SETTINGS_H
