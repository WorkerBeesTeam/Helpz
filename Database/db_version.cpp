#include "db_version.h"

#define STR(x) #x

namespace Helpz {
namespace DB {

quint8 ver_major() { return VER_MJ; }
quint8 ver_minor() { return VER_MN; }
int ver_build() { return VER_B; }
QString ver_str() { return STR(VER_MJ) "." STR(VER_MN) "." STR(VER_B); }

} // namespace DB
} // namespace Helpz
