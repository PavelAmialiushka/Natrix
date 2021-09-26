#ifndef DOCUMENTAUTOSAVER_H
#define DOCUMENTAUTOSAVER_H

#include "document.h"
#include "exclusiveFile.h"

class QSharedMemory;

namespace geometry
{

class DocumentAutoSaver : public QObject
{
    Q_OBJECT

    int                           recordIndex_;
    int                           modifyCounter_;
    Document *                    document_;
    QSharedPointer<ExclusiveFile> autoSaveFile_;

    QString dataName_;
    QString pathName_;

public slots:
    void checkAutoSaveStatus();
    void fileNameChanged();

public:
    DocumentAutoSaver(Document *parent);
    ~DocumentAutoSaver();

    static QStringList takeUnsavedRecords();

    void createAutoSaveRecord();
    void lockAutoSaveRecord(int index);
    void unlockAutoSaveRecord();
};

} // namespace geometry
#endif // DOCUMENTAUTOSAVER_H
