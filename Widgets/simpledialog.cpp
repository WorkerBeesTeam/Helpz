#include <QBoxLayout>
#include <QPushButton>

#include "simpledialog.h"

namespace Helpz {

SimpleDialog::SimpleDialog(QWidget *parent) :
    QDialog(parent)
{
    okBtn = new QPushButton("Добавить");
    cBtn = new QPushButton("Отменить");
    connect(okBtn, SIGNAL(clicked()), SLOT(accept()));
    connect(cBtn, SIGNAL(clicked()), SLOT(reject()));

    vbox = new QVBoxLayout;
}

int SimpleDialog::exec()
{
    QHBoxLayout* hbox = new QHBoxLayout;
    hbox->addStretch();
    hbox->addWidget( okBtn );
    hbox->addWidget( cBtn );

    vbox->addLayout( hbox );
    setLayout( vbox );

    return QDialog::exec();
}

} // namespace Helpz

