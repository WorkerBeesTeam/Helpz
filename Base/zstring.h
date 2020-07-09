#ifndef HELPZ_STRING_H
#define HELPZ_STRING_H

#include <string>

namespace Helpz {

class String
{
public:
    String();

    static void ltrim(std::string &s);
    static void rtrim(std::string &s);
    static void trim(std::string &s);
};

} // namespace Helpz

#endif // HELPZ_STRING_H
