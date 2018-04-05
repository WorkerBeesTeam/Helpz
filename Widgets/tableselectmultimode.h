#ifndef TABLESELECTMULTIMODE_H
#define TABLESELECTMULTIMODE_H

#include <QTableView>

namespace Helpz {

class TableSelectMultiMode : public QTableView
{
    Q_OBJECT
public:
    using QTableView::QTableView;

    void keyPressEvent(QKeyEvent *keyEvent);
    void keyReleaseEvent(QKeyEvent *keyEvent);
};

} // namespace Helpz

#endif // TABLESELECTMULTIMODE_H
