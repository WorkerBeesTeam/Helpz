#ifndef HELPZ_NET_VERSION_H
#define HELPZ_NET_VERSION_H

#include <QString>

namespace Helpz {
namespace Network {

unsigned short ver_major();
unsigned short ver_minor();
unsigned short ver_build();
QString ver_str();

} // namespace Network
} // namespace Helpz

#endif // HELPZ_NET_VERSION_H
