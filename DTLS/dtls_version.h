#ifndef HELPZ_DTLS_VERSION_H
#define HELPZ_DTLS_VERSION_H

#include <QString>

namespace Helpz {
namespace DTLS {

unsigned short ver_major();
unsigned short ver_minor();
unsigned short ver_build();
QString ver_str();

} // namespace DTLS
} // namespace Helpz

#endif // HELPZ_DTLS_VERSION_H
