#include "dtls_version.h"

#define STR(x) #x

namespace Helpz {
namespace DTLS {

unsigned short ver_major() { return VER_MJ; }
unsigned short ver_minor() { return VER_MN; }
unsigned short ver_build() { return VER_B; }
QString ver_str() { return STR(VER_MJ) "." STR(VER_MN) "." STR(VER_B); }

} // namespace DTLS
} // namespace Helpz
