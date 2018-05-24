#include "dtls_version.h"
#include "version.h"

namespace Helpz {
namespace DTLS {

unsigned short ver_major() { return HelpzDTLS::Version::MAJOR; }
unsigned short ver_minor() { return HelpzDTLS::Version::MINOR; }
unsigned short ver_build() { return HelpzDTLS::Version::BUILD; }
QString ver_str() { return HelpzDTLS::Version::getVersionString(); }

} // namespace DTLS
} // namespace Helpz
