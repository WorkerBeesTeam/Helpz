#include <algorithm>

#include "zstring.h"

namespace Helpz {

String::String()
{

}

bool trim_check(char ch)
{
    return !std::isspace(ch);
}

/*static*/ void String::ltrim(std::string &s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), trim_check));
}

/*static*/ void String::rtrim(std::string &s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), trim_check).base(), s.end());
}

/*static*/ void String::trim(std::string &s)
{
    ltrim(s);
    rtrim(s);
}

} // namespace Helpz
