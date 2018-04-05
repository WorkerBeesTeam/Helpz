#ifndef SIMPLEDIALOG_H
#define SIMPLEDIALOG_H

#include <QDialog>

class QVBoxLayout;

namespace Helpz {

class SimpleDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SimpleDialog(QWidget *parent = 0);
    int exec();
    QPushButton* okBtn;
    QPushButton* cBtn;
    QVBoxLayout* vbox;
};

} // namespace Helpz

#endif // SIMPLEDIALOG_H
