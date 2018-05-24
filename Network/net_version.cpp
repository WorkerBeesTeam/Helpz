#include "net_version.h"
#include "version.h"

namespace Helpz {
namespace Network {

unsigned short ver_major() { return HelpzNetwork::Version::MAJOR; }
unsigned short ver_minor() { return HelpzNetwork::Version::MINOR; }
unsigned short ver_build() { return HelpzNetwork::Version::BUILD; }
QString ver_str() { return HelpzNetwork::Version::getVersionString(); }

} // namespace Network
} // namespace Helpz
