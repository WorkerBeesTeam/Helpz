#ifndef UNCHECKSLOTCHECKBOX_H
#define UNCHECKSLOTCHECKBOX_H

#include <QCheckBox>

namespace Helpz {

class UnCheckSlotCheckBox : public QCheckBox
{
    Q_OBJECT
    Qt::CheckState defaultCheckState = Qt::Unchecked;
public:
    using QCheckBox::QCheckBox;
    void setDefaultCheckState(Qt::CheckState state);
public slots:
    void unCheck();
};

} // namespace Helpz

#endif // UNCHECKSLOTCHECKBOX_H
