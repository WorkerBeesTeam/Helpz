#include "dtls_controller.h"

namespace Helpz {
namespace DTLS {

Controller::Controller(Tools* dtls_tools) :
    dtls_tools_(dtls_tools),
    protocol_timer_(this)
{
}

Tools *Controller::dtls_tools()
{
    return dtls_tools_;
}

} // namespace DTLS
} // namespace Helpz
