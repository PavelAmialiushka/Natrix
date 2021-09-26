#include "documentAutoSaver.h"

#include <QBuffer>
#include <QSettings>
#include <QSharedMemory>
#include <QSystemSemaphore>
#include <QTemporaryFile>
#include <QTimer>

namespace geometry
{

static QString SemaphoreName = "NatrixAutoSave";
static QString CounterName   = "as_counter";

class SemaphoreLock
{
    QSystemSemaphore sem;

public:
    SemaphoreLock()
        : sem(SemaphoreName, 1)
    {
        sem.acquire();
    }

    ~SemaphoreLock()
    {
        sem.release();
    }
};

#ifdef NDEBUG
#define ENABLE_AUTOSAVE
#endif

DocumentAutoSaver::DocumentAutoSaver(Document *parent)
    : QObject(parent)
    , document_(parent)
    , autoSaveFile_(0)
    , modifyCounter_(-1)
{
#ifdef ENABLE_AUTOSAVE
    createAutoSaveRecord();

    auto timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(checkAutoSaveStatus()));
    timer->start(2500);

    connect(document_, SIGNAL(fileNameChanged()), this, SLOT(fileNameChanged()));
#endif
}

DocumentAutoSaver::~DocumentAutoSaver()
{
    unlockAutoSaveRecord();
}

QStringList DocumentAutoSaver::takeUnsavedRecords()
{
    SemaphoreLock guard;

    QSettings settings;
    int       count = settings.value(CounterName, 0).toInt();

    QStringList results;

    int maxActive = 0;
    for(int index = 0; index < count; ++index)
    {
        QString keyName  = QString("as_%1").arg(index);
        QString fileName = settings.value(keyName).toString();

        // пустая запись
        if(fileName.isEmpty())
            continue;

        ExclusiveFile test(fileName);

        bool opened = test.open(QIODevice::ReadWrite);
        if(opened && test.size() > 0)
        {
            // файл открылся, значит надо восстанавливать
            results << fileName;
            settings.remove(keyName);
        }
        else if(opened)
        {
            test.close();
            QFile(fileName).remove();
            settings.remove(keyName);
        }
        else if(test.isBusy())
        {
            maxActive = index;
        }
        // ещё есть cлучай, когда файл не открылся, поскольку его удалили
        // но тогда и делать ничего не нужно
    }

    settings.setValue(CounterName, maxActive + 1);
    return results;
}
void DocumentAutoSaver::createAutoSaveRecord()
{
    SemaphoreLock guard;

    QSettings settings;
    int       count = settings.value(CounterName, 0).toInt();
    settings.setValue(CounterName, count);

    int index = 0;
    for(; index <= count; ++index)
    {
        auto recordName = QString("as_%1").arg(index);
        auto value      = settings.value(recordName, "").toString();
        if(value.isEmpty())
            break;
    }

    if(index >= count)
        settings.setValue(CounterName, index + 1);

    lockAutoSaveRecord(index);
}

void DocumentAutoSaver::lockAutoSaveRecord(int index)
{
    Q_ASSERT(!autoSaveFile_);

    QTemporaryFile temp;
    temp.setAutoRemove(false);
    temp.open();

    autoSaveFile_.reset(new ExclusiveFile(temp.fileName()));
    autoSaveFile_->setParent(this);
    autoSaveFile_->open(QIODevice::WriteOnly);
    temp.close();

    QSettings settings;
    settings.setValue(QString("as_%1").arg(index), autoSaveFile_->fileName());
}

void DocumentAutoSaver::unlockAutoSaveRecord()
{
    if(autoSaveFile_)
    {
        autoSaveFile_->close();
        autoSaveFile_->remove();
        autoSaveFile_.reset();
    }
}

// вызывается периодически
void DocumentAutoSaver::checkAutoSaveStatus()
{
    // отключено автосохранение
    if(!autoSaveFile_)
        return;

    if(document_->modified(modifyCounter_))
    {
        document_->saveDocument(autoSaveFile_.data());
    }
    else if(!document_->modified())
    {
        // просто обнуляем
        autoSaveFile_->seek(0);
        autoSaveFile_->resize(0);
    }
    autoSaveFile_->flush();
}

void DocumentAutoSaver::fileNameChanged()
{
    checkAutoSaveStatus();
}

} // namespace geometry
