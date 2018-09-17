#ifndef HELPZ_NET_VERSION_H
#define HELPZ_NET_VERSION_H

#include <QString>

namespace Helpz {
namespace Network {

quint8 ver_major();
quint8 ver_minor();
int ver_build();
QString ver_str();

} // namespace Network
} // namespace Helpz

#endif // HELPZ_NET_VERSION_H
