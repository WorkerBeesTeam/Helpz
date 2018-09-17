#ifndef HELPZ_DTLS_VERSION_H
#define HELPZ_DTLS_VERSION_H

#include <QString>

namespace Helpz {
namespace DTLS {

quint8 ver_major();
quint8 ver_minor();
int ver_build();
QString ver_str();

} // namespace DTLS
} // namespace Helpz

#endif // HELPZ_DTLS_VERSION_H
