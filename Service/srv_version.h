#ifndef HELPZ_SRV_VERSION_H
#define HELPZ_SRV_VERSION_H

#include <QString>

namespace Helpz {
namespace Service {

unsigned short ver_major();
unsigned short ver_minor();
unsigned short ver_build();
QString ver_str();

} // namespace Service
} // namespace Helpz

#endif // HELPZ_SRV_VERSION_H
