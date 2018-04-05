#include <QDebug>

#include "datewidget.h"

namespace Helpz {

DateWidget::DateWidget(QWidget *parent) :
    QDateEdit(parent)
{
    setSpecialValueText("Нет");
    setMaximumDate( QDate::currentDate() );
}

QVariant DateWidget::getData()
{
    return QVariant( date() );
}

void DateWidget::setData(const QVariant &value)
{
    QDate date = value.isNull() ? minimumDate() : value.toDate();
    setDate( date );
}

} // namespace Helpz
