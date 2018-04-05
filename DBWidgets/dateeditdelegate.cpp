#include <QDebug>
#include <QDateTimeEdit>

#include "dateeditdelegate.h"

namespace Helpz {

QWidget *DateEditDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const
{
    QDateTimeEdit* editor = new QDateTimeEdit(parent);
    editor->setCalendarPopup( true );
    editor->setDisplayFormat("hh:mm dd.MM.yy");
    return editor;
}

void DateEditDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QDateTimeEdit * dateEdit = static_cast<QDateTimeEdit*>(editor);
    auto val = index.model()->data(index).toDateTime();
    if (!val.isValid())
        val = val.currentDateTime();
    dateEdit->setDateTime( val );
}

void DateEditDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QDateTimeEdit * dateEdit = static_cast<QDateTimeEdit*>(editor);
    model->setData( index, dateEdit->dateTime().toString("yyyy-MM-dd hh:mm:ss") );
}

void DateEditDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &) const
{
    editor->setGeometry(option.rect);
}

QString DateEditDelegate::displayText(const QVariant &value, const QLocale &locale) const
{
    return locale.toString(value.toDateTime(), "dd MMMM yyyyг. hh:mm");
}

} // namespace Helpz
