#ifndef HELPZ_DB_VERSION_H
#define HELPZ_DB_VERSION_H

#include <QString>

namespace Helpz {
namespace DB {
    quint8 ver_major();
    quint8 ver_minor();
    int ver_build();
    QString ver_str();
} // namespace DB
} // namespace Helpz

#endif // HELPZ_DB_VERSION_H
