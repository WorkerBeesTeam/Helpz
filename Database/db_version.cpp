#include "db_version.h"
#include "version.h"

namespace Helpz {
namespace Database {

unsigned short ver_major() { return HelpzDB::Version::MAJOR; }
unsigned short ver_minor() { return HelpzDB::Version::MINOR; }
unsigned short ver_build() { return HelpzDB::Version::BUILD; }
QString ver_str() { return HelpzDB::Version::getVersionString(); }

} // namespace Database
} // namespace Helpz
