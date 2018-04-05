#ifndef LINEEDITWITHPLACEHOLDERSLOT_H
#define LINEEDITWITHPLACEHOLDERSLOT_H

#include <QLineEdit>

namespace Helpz {

class LineEditWithPlaceHolderSlot : public QLineEdit
{
    Q_OBJECT
public:
    using QLineEdit::QLineEdit;
public slots:
    void setPlaceholderText(const QString& text);
};

} // namespace Helpz

#endif // LINEEDITWITHPLACEHOLDERSLOT_H
