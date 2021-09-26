#ifndef EXCLUSIVEFILE_H
#define EXCLUSIVEFILE_H

#include "makeSmart.h"

#include <QFile>
#include <QLocalServer>
#include <QObject>
#include <QWidget>

MAKESMART(ExclusiveFile);

class ExclusiveFile : public QFile
{
    Q_OBJECT

    QString filename_;
    bool    busy_;
    bool    notified_;
    bool    locked_;

    bool getHandle(void *&h);

public:
    ExclusiveFile(QString);

    bool open(OpenMode mode);
    void close();

    bool lock();
    bool isLocked() const;
    void unlock();

    bool isBusy() const;
    bool isNotified() const;

signals:
    void fileActivated();
    void fileIsBusy(bool *);
    void fileLocked();
    void fileClosed();
};

#endif // EXCLUSIVEFILE_H
