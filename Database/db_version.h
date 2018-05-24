#ifndef HELPZ_DB_VERSION_H
#define HELPZ_DB_VERSION_H

#include <QString>

namespace Helpz {
namespace Database {
    unsigned short ver_major();
    unsigned short ver_minor();
    unsigned short ver_build();
    QString ver_str();
} // namespace Database
} // namespace Helpz

#endif // HELPZ_DB_VERSION_H
