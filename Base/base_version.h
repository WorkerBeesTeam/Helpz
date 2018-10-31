#ifndef HELPZ_BASE_VERSION_H
#define HELPZ_BASE_VERSION_H

#include <QString>

namespace Helpz {
namespace Base {

quint8 ver_major();
quint8 ver_minor();
int ver_build();
QString ver_str();

} // namespace Base
} // namespace Helpz

#endif // HELPZ_BASE_VERSION_H
