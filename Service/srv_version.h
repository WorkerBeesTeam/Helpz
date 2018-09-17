#ifndef HELPZ_SRV_VERSION_H
#define HELPZ_SRV_VERSION_H

#include <QString>

namespace Helpz {
namespace Service {

quint8 ver_major();
quint8 ver_minor();
int ver_build();
QString ver_str();

} // namespace Service
} // namespace Helpz

#endif // HELPZ_SRV_VERSION_H
