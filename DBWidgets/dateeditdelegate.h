#ifndef DATEEDITDELEGATE_H
#define DATEEDITDELEGATE_H

#include <QStyledItemDelegate>

namespace Helpz {

class DateEditDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    DateEditDelegate(QObject *parent = 0) : QStyledItemDelegate(parent) {}
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &) const;
    QString displayText(const QVariant &value, const QLocale &locale) const;
};

} // namespace Helpz

#endif // DATEEDITDELEGATE_H
