#ifndef CONSOLEREADER_H
#define CONSOLEREADER_H

#include <QSocketNotifier>

namespace Helpz {

class ConsoleReader : public QObject
{
    Q_OBJECT
public:
    explicit ConsoleReader(QObject *parent = 0);
signals:
    void textReceived(const QString& message);
public slots:
    void text();
private:
    QSocketNotifier notifier;
};

} // namespace Helpz

#endif // CONSOLEREADER_H
