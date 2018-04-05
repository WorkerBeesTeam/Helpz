#include <QIcon>
#include <QPixmap>
#include <QImage>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QToolButton>
#include <QWidgetAction>
#include <QDebug>

#include "filterlineedit.h"

namespace Helpz {

FilterLineEdit::FilterLineEdit(QWidget *parent) :
    QLineEdit(parent)
{
    QMenu *menu = new QMenu(this);
    csAction = menu->addAction("Учитывать регистр");
    csAction->setCheckable(true);
    connect(csAction, SIGNAL(toggled(bool)), SLOT(filterChanged(bool)));

//    QAction *patternAction = menu->addAction("Только слова целиком");
//    patternAction->setCheckable(true);
//    connect(patternAction, SIGNAL(toggled(bool)), SLOT(filterChanged(bool)));

    const QIcon icon = QIcon(QPixmap(":/tool/find"));
    QToolButton *optionsButton = new QToolButton;
    optionsButton->setCursor(Qt::ArrowCursor);
    optionsButton->setFocusPolicy(Qt::NoFocus);
    optionsButton->setStyleSheet("* { border: none; }");
    optionsButton->setIcon(icon);
    optionsButton->setMenu(menu);
    optionsButton->setPopupMode(QToolButton::InstantPopup);

    QWidgetAction *optionsAction = new QWidgetAction(this);
    optionsAction->setDefaultWidget(optionsButton);
    addAction(optionsAction, QLineEdit::LeadingPosition);
}

void FilterLineEdit::filterChanged(bool b)
{
    QAction* a = qobject_cast< QAction* >( sender() );
    if ( !a )
        return;

    if ( a == csAction )
        emit caseSensitiv( b );
    else
        emit fixedString( b );
}

} // namespace Helpz
