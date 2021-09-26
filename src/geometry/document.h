#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <QDomElement>
#include <QFile>
#include <QObject>
#include <QSharedPointer>
#include <QString>

#include "exclusiveFile.h"
#include "label.h"
#include "marker.h"
#include "object.h"

class QIODevice;
class QDomDocument;

namespace geometry
{

class Scene;

enum DocumentType
{
    DocumentIsometric = 1,
    DocumentDimetric,
    DocumentPlane,
};

class Document : public QObject
{
    Q_OBJECT

    int     modifyCounter_;
    QString fileName_;
    QString originalFileName_;

    Scene *scene_;

    QSharedPointer<ExclusiveFile> file_;
    bool                          autoRestored_;

signals:
    void replaceScene();
    void fileNameChanged();

public:
    Document(Scene *parent);

    QString fileName() const;

    PExclusiveFile file() const;
    void           setFile(QString, PExclusiveFile);

    QString originalFileName() const;
    void    setOriginalFile();

    bool autoRestored() const;
    bool modified() const;
    bool modified(int &counter) const;
    bool isNewDocument() const;
    void clearModify();

    Scene *scene() const;

    void newDocument(DocumentType type = DocumentIsometric);
    bool saveDocument(QIODevice *);

    static QString preLoadDocumentOriginalName(QString);
    bool           loadDocument(QIODevice *);

    bool v1LoadDocument(QDomDocument &);

    static bool loadSceneData(QDomElement,
                              Scene *,
                              QSet<PObject> & objects,
                              QSet<PLabel> &  labels,
                              QList<PMarker> &markers);
    static void saveSceneData(QDomDocument,
                              QDomElement,
                              QSet<PObject>  object,
                              QSet<PLabel>   labels,
                              QList<PMarker> markers);
};

} // namespace geometry

#endif // DOCUMENT_H
