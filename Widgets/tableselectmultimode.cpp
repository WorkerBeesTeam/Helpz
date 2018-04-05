#include <QKeyEvent>
#include <QDebug>
#include "tableselectmultimode.h"

namespace Helpz {

void TableSelectMultiMode::keyPressEvent(QKeyEvent *keyEvent)
{
    if (keyEvent->modifiers() == Qt::ControlModifier) {
        if (selectionMode() != QAbstractItemView::MultiSelection) {
            clearSelection();
            setSelectionMode(QAbstractItemView::MultiSelection);
            setSelectionBehavior(QAbstractItemView::SelectItems);
        }
    }
    QTableView::keyPressEvent(keyEvent);
}

void TableSelectMultiMode::keyReleaseEvent(QKeyEvent *keyEvent)
{
    if (selectionMode() == QAbstractItemView::MultiSelection) {
        setSelectionMode(QAbstractItemView::SingleSelection);
        setSelectionBehavior(QAbstractItemView::SelectRows);
    }
    QTableView::keyReleaseEvent(keyEvent);
}

} // namespace Helpz
