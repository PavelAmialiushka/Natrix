#ifndef SINGLETONFILEWINDOW_H
#define SINGLETONFILEWINDOW_H

#include "exclusiveFile.h"

#include <QObject>

class SingletonFileWindow : public QObject
{
    Q_OBJECT

    ExclusiveFile *file_;

    QLocalServer *server_;
    WId           hMainWindow;

public:
    SingletonFileWindow(ExclusiveFile *parent, QWidget *window);

    using QObject::connect;
    static void connect(ExclusiveFile *parent, QWidget *window);

    void setWindow(QWidget *);
    bool sendMessageToOtherInstance(QString);
    bool activateWindow(size_t);

public slots:
    void receiveConnection();

    void fileIsBusy(bool *);
    void fileLocked();
    void fileClosed();
};

#endif // SINGLETONFILEWINDOW_H
