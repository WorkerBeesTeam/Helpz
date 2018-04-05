#include "uncheckslotcheckbox.h"

namespace Helpz {

void UnCheckSlotCheckBox::setDefaultCheckState(Qt::CheckState state)
{
    defaultCheckState = state;
}

void UnCheckSlotCheckBox::unCheck()
{
    setCheckState( defaultCheckState );
}

} // namespace Helpz

