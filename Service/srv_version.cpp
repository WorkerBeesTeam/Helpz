#include "srv_version.h"
#include "version.h"

namespace Helpz {
namespace Service {

unsigned short ver_major() { return HelpzService::Version::MAJOR; }
unsigned short ver_minor() { return HelpzService::Version::MINOR; }
unsigned short ver_build() { return HelpzService::Version::BUILD; }
QString ver_str() { return HelpzService::Version::getVersionString(); }

} // namespace Service
} // namespace Helpz
