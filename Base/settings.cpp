
#include "zstring.h"
#include "settings.h"

namespace Helpz {

Settings::Settings()
{

}

/*static*/ std::string Settings::get_param_value(const std::string &text, const std::string &param, std::size_t *begin_pos, std::size_t *end_pos)
{
    size_t pos = text.find(param);
    pos = text.find('=', pos + param.size());
    if (pos == std::string::npos)
        return {};

    ++pos;
    size_t pos_end = text.find('\n', pos);
    std::string value = text.substr(pos, pos_end - pos);
    String::trim(value);

    if (begin_pos) *begin_pos = pos;
    if (end_pos) *end_pos = pos_end;
    return value;
}

/*static*/ bool Settings::set_param_value(std::string &text, const std::string &param, const std::string &value)
{
    size_t pos = std::string::npos, pos_end;
    const std::string old_value = get_param_value(text, param, &pos, &pos_end);
    if (pos == std::string::npos || old_value == value)
        return false;

    text.replace(pos, pos_end - pos, value);
    return true;
}

} // namespace Helpz
