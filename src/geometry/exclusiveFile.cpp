#include "exclusiveFile.h"

#include <io.h>
#include <windows.h>

ExclusiveFile::ExclusiveFile(QString name)
    : QFile(name)
    , filename_(name)
    , busy_(false)
    , notified_(false)
{
}

bool ExclusiveFile::open(QIODevice::OpenMode mode)
{
    if(!QFile::open(mode))
    {
        return false;
    }

    if(!lock())
    {
        busy_ = true;

        notified_ = false;
        emit fileIsBusy(&notified_);

        close();
        return false;
    }

    emit fileLocked();
    return true;
}

void ExclusiveFile::close()
{
    unlock();
    QFile::close();
    emit fileClosed();
}

bool ExclusiveFile::getHandle(void *&h)
{
    if(static_cast<std::ptrdiff_t>(handle()) == -1)
        return false;

    h = (HANDLE)_get_osfhandle(handle());
    if(reinterpret_cast<std::ptrdiff_t>(h) == -1)
        return false;

    return true;
}

bool ExclusiveFile::lock()
{
    Q_ASSERT(isOpen());
    locked_ = true;
    HANDLE h;
    if(getHandle(h))
        return (bool)::LockFile(h, 0, 0, -1, -1);
    return false;
}

bool ExclusiveFile::isLocked() const
{
    return locked_;
}

void ExclusiveFile::unlock()
{
    locked_ = false;
    HANDLE h;
    if(getHandle(h))
        ::UnlockFile(h, 0, 0, -1, -1);
}

bool ExclusiveFile::isBusy() const
{
    return busy_;
}

bool ExclusiveFile::isNotified() const
{
    return notified_;
}
