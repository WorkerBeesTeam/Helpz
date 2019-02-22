#include "dtls_controller.h"

namespace Helpz {
namespace DTLS {

Controller::Controller(Tools* dtls_tools) :
    dtls_tools_(dtls_tools)
{

}

Tools *Controller::dtls_tools()
{
    return dtls_tools_;
}

} // namespace DTLS
} // namespace Helpz
