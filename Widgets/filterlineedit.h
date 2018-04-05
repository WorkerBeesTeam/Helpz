#ifndef FILTERLINEEDIT_H
#define FILTERLINEEDIT_H

#include <QLineEdit>

class QAction;

namespace Helpz {

class FilterLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    FilterLineEdit(QWidget* parent = 0);
signals:
    void caseSensitiv(bool);
    void fixedString(bool);
private slots:
    void filterChanged(bool b);
private:
    QAction *csAction;
};

} // namespace Helpz

#endif // FILTERLINEEDIT_H
