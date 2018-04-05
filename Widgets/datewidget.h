#ifndef DATEWIDGET_H
#define DATEWIDGET_H

#include <QDateEdit>

namespace Helpz {

class DateWidget : public QDateEdit
{
    Q_OBJECT
    Q_PROPERTY(QVariant data READ getData WRITE setData)
public:
    explicit DateWidget(QWidget *parent = 0);

    QVariant getData();
    void setData(const QVariant& value);
};

} // namespace Helpz

#endif // DATEWIDGET_H
