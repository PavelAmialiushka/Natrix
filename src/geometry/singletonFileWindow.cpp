#include "singletonFileWindow.h"

#include <qlocalsocket.h>

#include <QDir>
#include <io.h>
#include <windows.h>

SingletonFileWindow::SingletonFileWindow(ExclusiveFile *parent, QWidget *window)
    : QObject(parent)
    , server_(new QLocalServer(this))
    , file_(parent)
{
    connect(file_, SIGNAL(fileIsBusy(bool *)), this, SLOT(fileIsBusy(bool *)));
    connect(file_, SIGNAL(fileLocked()), this, SLOT(fileLocked()));
    connect(file_, SIGNAL(fileClosed()), this, SLOT(fileClosed()));

    // слущаем сообщения от сервера
    connect(server_, SIGNAL(newConnection()), this, SLOT(receiveConnection()));

    setWindow(window);
}

void SingletonFileWindow::connect(ExclusiveFile *parent, QWidget *window)
{
    new SingletonFileWindow(parent, window);
}

void SingletonFileWindow::setWindow(QWidget *w)
{
    hMainWindow = w->winId();
}

bool SingletonFileWindow::sendMessageToOtherInstance(QString filename)
{
    QScopedPointer<QLocalSocket> socket{new QLocalSocket};
    socket->connectToServer(filename, QIODevice::ReadWrite);

    if(socket->waitForReadyRead(1000))
    {
        QByteArray data = socket->readAll();

        QString str = QString::fromLatin1(data);

        QStringList list = str.split(",");
        if(list.size() < 2)
            return false;

        bool  ok       = false;
        ulong windowId = list[0].toULong(&ok);
        if(!ok)
            return false;

        ulong processId = list[1].toULong(&ok);
        if(!ok)
            return false;

        // если посылаем сами себе
        if(::GetCurrentProcessId() == processId)
            return false;

        return activateWindow(windowId);
    }

    socket->disconnectFromServer();
    socket->close();
    return false;
}

bool SingletonFileWindow::activateWindow(size_t windowId)
{
    HWND hwnd = (HWND)windowId;
    return ::SetForegroundWindow(hwnd) != 0;
}

void SingletonFileWindow::receiveConnection()
{
    auto socket = server_->nextPendingConnection();
    if(!socket)
        return;

    // отправляем информацию
    QByteArray data = QString("%1,%2")
                          .arg((size_t)(HWND)hMainWindow)
                          .arg((size_t)::GetCurrentProcessId())
                          .toLatin1();
    socket->write(data);
    socket->flush();

    socket->close();
}

void SingletonFileWindow::fileIsBusy(bool *notified)
{
    QString fileName = QFileInfo(file_->fileName()).absoluteFilePath().toLower();
    *notified        = sendMessageToOtherInstance(fileName);
}

void SingletonFileWindow::fileLocked()
{
    QString fileName = QFileInfo(file_->fileName()).absoluteFilePath().toLower();
    server_->listen(fileName);
}

void SingletonFileWindow::fileClosed()
{
    server_->close();
}
