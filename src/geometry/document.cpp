#include "document.h"
#include "objectFactory.h"
#include "scene.h"

#include "canvasRectangle.h"
#include "label.h"
#include "manipulator.h"
#include "marker.h"
#include "textLabel.h"

#include "line.h"
#include "neighbourhood.h"
#include "object.h"
#include "sceneProcedures.h"

#include "cryptopp/gzip.h"
#include "cryptopp/zlib.h"

#include <Qt>

namespace
{
///////////////////////////////////////////////////////////////////////////

QByteArray gUncompress(const QByteArray &data)
{
    CryptoPP::ZlibDecompressor unzipper;

    try
    {

        unzipper.Put((const unsigned char *)data.data(), (size_t)data.size());

        QByteArray result(unzipper.MaxRetrievable(), Qt::Uninitialized);
        unzipper.Get((unsigned char *)result.data(), (size_t)result.size());

        return result;
    }
    catch(CryptoPP::ZlibDecompressor::Err &)
    {
        return QByteArray{};
    }
}
} // namespace

///////////////////////////////////////////////////////////////////////////

namespace geometry
{

Document::Document(Scene *parent)
    : scene_(parent)
    , autoRestored_(false)
{
}

QString Document::fileName() const
{
    return fileName_;
}

QString Document::originalFileName() const
{
    return originalFileName_;
}

void Document::setOriginalFile()
{
    setFile(originalFileName(), PExclusiveFile());
    autoRestored_ = true;
}

bool Document::autoRestored() const
{
    return autoRestored_;
}

void Document::setFile(QString filename, PExclusiveFile file)
{
    if(file_ && file_->isOpen())
        file_->close();

    fileName_     = filename;
    file_         = file;
    autoRestored_ = false;

    emit fileNameChanged();
}

QSharedPointer<ExclusiveFile> Document::file() const
{
    return file_;
}

void Document::clearModify()
{
    modifyCounter_ = scene_->modifyCounter();
}

bool Document::modified() const
{
    return modifyCounter_ != scene_->modifyCounter() || autoRestored_;
}

bool Document::modified(int &counter) const
{
    bool res = counter != scene_->modifyCounter();
    counter  = scene_->modifyCounter();
    return res;
}

Scene *Document::scene() const
{
    return scene_;
}

bool Document::isNewDocument() const
{
    return fileName_.isEmpty();
}

void Document::newDocument(DocumentType type)
{
    QSharedPointer<Scene> newbyScene(new Scene(type));

    newbyScene->swap(scene_);
    emit replaceScene();

    if(file_ && file_->isOpen())
        file_->close();

    autoRestored_ = false;

    fileName_.clear();
    clearModify();
}

namespace
{
int programVersion = 200;
int fileVersion    = 200;
} // namespace

bool Document::saveDocument(QIODevice *dest)
{
    if(auto file = dynamic_cast<QFile *>(dest))
    {
        file->seek(0);
        file->resize(0);
    }

    auto cursorPoint = scene_->cursorPoint();
    scene_->manipulator()->moveOut();

    QDomDocument doc;
    auto         rootElement = doc.createElement("skt_project");
    doc.appendChild(rootElement);

    rootElement.setAttribute("program_version", programVersion);
    rootElement.setAttribute("version", fileVersion);
    rootElement.setAttribute("planeType", scene_->planeType());

    // исключительно для целей автовосстановления
    rootElement.setAttribute("fileName", fileName());

    QDomElement contents = doc.createElement("contents");
    rootElement.appendChild(contents);

    QSet<PMarker> markers;
    foreach(PObject object, scene_->objects())
        markers += scene_->markersOfLeader(object).toSet();
    foreach(PLabel label, scene_->labels())
        markers += scene_->markersOfLeader(label).toSet();

    // сохранение
    Document::saveSceneData(doc, contents, scene_->objects(), scene_->labels(), markers.toList());

    scene_->manipulator()->move(cursorPoint);

    // Scene
    QDomElement sceneElement = doc.createElement("scene");
    contents.appendChild(sceneElement);

    sceneElement.setAttribute("windowlt", scene_->visibleLeftTop().serialSave());
    sceneElement.setAttribute("windowrb", scene_->visibleRightBottom().serialSave());

    QTextStream stream(dest);
    doc.save(stream, 2);
    return true;
}

void Document::saveSceneData(QDomDocument   doc,
                             QDomElement    contents,
                             QSet<PObject>  objects,
                             QSet<PLabel>   labels,
                             QList<PMarker> markers)
{
    QList<PObjectToSelect> pos;

    foreach(PObject object, objects)
    {
        pos << object;

        QDomElement objectElement = doc.createElement("object");
        contents.appendChild(objectElement);
        ObjectFactory::saveObject(object, objectElement);
        objectElement.setAttribute("id", reinterpret_cast<size_t>(object.data()));
    }

    foreach(PLabel label, labels)
    {
        pos << label;
        QDomElement labelElement = doc.createElement("label");
        contents.appendChild(labelElement);

        Label::saveLabel(label, labelElement);
        labelElement.setAttribute("id", reinterpret_cast<size_t>(label.data()));
    }

    foreach(PMarker marker, markers)
    {
        QDomElement markerElement = doc.createElement("marker");
        contents.appendChild(markerElement);

        markerElement.setAttribute("marker", reinterpret_cast<size_t>(marker.data()));
        markerElement.setAttribute("leader", reinterpret_cast<size_t>(marker->leader.data()));
        markerElement.setAttribute("follower", reinterpret_cast<size_t>(marker->follower.data()));
        markerElement.setAttribute("point", marker->point.serialSave());
    }
}

QString Document::preLoadDocumentOriginalName(QString fileName)
{
    QFile file(fileName);
    file.open(QIODevice::ReadOnly);

    QByteArray source = file.readAll();

    QByteArray decoded = gUncompress(source);
    if(decoded.size())
        source = decoded;

    QDomDocument doc;
    int          errorLine;
    QString      errorMsg;
    if(!doc.setContent(source, &errorMsg, &errorLine))
    {
        Q_ASSERT(!"xml load error");
        return "";
    }

    auto tempProjects = doc.elementsByTagName("skt_project");
    if(!tempProjects.size())
    {
        Q_ASSERT(!"incorrect format");
        return "";
    }

    QDomElement project;

    // поиск подходящего номера версии
    for(int index = 0; index < tempProjects.size(); ++index)
    {
        project            = tempProjects.at(index).toElement();
        bool okReadVersion = false;
        int  fileVersion   = project.toElement().attribute("version", "0").toInt(&okReadVersion);
        if(!okReadVersion)
        {
            Q_ASSERT(!"incorrect format");
            continue;
        }
        if(programVersion < fileVersion)
        {
            project.clear();
            continue;
        }
    }

    return project.attribute("fileName");
}

bool Document::loadDocument(QIODevice *file)
{
    file->seek(0);
    QByteArray source = file->readAll();

    QByteArray decoded = gUncompress(source);
    if(decoded.size())
        source = decoded;

    QDomDocument doc;
    int          errorLine;
    QString      errorMsg;
    if(!doc.setContent(source, &errorMsg, &errorLine))
    {
        Q_ASSERT(!"xml load error");
        return 0;
    }

    // поиск заголовка
    auto tempV1 = doc.elementsByTagName("PipeS");
    if(tempV1.size())
        return v1LoadDocument(doc);

    auto tempProjects = doc.elementsByTagName("skt_project");
    if(!tempProjects.size())
    {
        Q_ASSERT(!"incorrect format");
        return 0;
    }

    QDomElement project;

    // поиск подходящего номера версии
    for(int index = 0; index < tempProjects.size(); ++index)
    {
        project            = tempProjects.at(index).toElement();
        bool okReadVersion = false;
        int  fileVersion   = project.toElement().attribute("version", "0").toInt(&okReadVersion);
        if(!okReadVersion)
        {
            Q_ASSERT(!"incorrect format");
            continue;
        }
        if(programVersion < fileVersion)
        {
            project.clear();
            continue;
        }
    }

    // загружаем тип плоскости для инициализации сцены
    int planeType = project.attribute("planeType", "1").toInt();

    originalFileName_ = project.attribute("fileName");

    //
    // вот теперь начинается загрузка сцены
    //
    QSharedPointer<Scene> newbyScene(new Scene(planeType, true));

    if(project.isNull())
    {
        Q_ASSERT(!"incorrect version");
        return 0;
    }

    // входим в область содержимого сцены
    //
    QDomNodeList tempContents = project.elementsByTagName("contents");
    if(!tempContents.size())
    {
        Q_ASSERT(!"incorrect format");
        return 0;
    };
    QDomElement contents = tempContents.at(0).toElement();

    QSet<PObject>  objects;
    QSet<PLabel>   labels;
    QList<PMarker> markers;
    if(!Document::loadSceneData(contents, newbyScene.data(), objects, labels, markers))
    {
        return false;
    }

    foreach(auto object, objects)
        newbyScene->attach(object);
    foreach(auto label, labels)
        newbyScene->attachLabel(label);
    foreach(auto marker, markers)
        newbyScene->attachMarker(marker);

    // итак, до сих пор все было хорошо
    // загружаем тонкости настройки сцены
    QDomNodeList tempScene = contents.elementsByTagName("scene");
    if(!tempScene.size())
    {
        Q_ASSERT(!"incorrect format");
        return 0;
    }
    QDomElement sceneElement = tempScene.at(0).toElement();

    bool    ltok, rbok;
    point2d lt = point2d::serialLoad(sceneElement.attribute("windowlt"), &ltok);
    point2d rb = point2d::serialLoad(sceneElement.attribute("windowrb"), &rbok);
    if(!(ltok && rbok))
    {
        Q_ASSERT(!"incorrect format");
        return 0;
    }

    // абра кадабра
    newbyScene->swap(scene_);
    emit replaceScene();

    scene_->setRecomendedWindow(lt, rb);
    return 1;
}

bool Document::loadSceneData(QDomElement     contents,
                             Scene *         newbyScene,
                             QSet<PObject> & objects,
                             QSet<PLabel> &  labels,
                             QList<PMarker> &markers)
{
    // cache объектов для загрузки
    QMap<QString, PObjectToSelect> pos;
    QMap<QString, PMarker>         markerCache;

    // загружаем объекты, входящие в сцену
    //
    QDomNodeList domObjects = contents.elementsByTagName("object");
    for(int index = 0; index < domObjects.size(); ++index)
    {
        QDomElement objectElement = domObjects.at(index).toElement();
        PObject     loaded        = ObjectFactory::createObject(newbyScene, objectElement);
        if(!loaded)
        {
            Q_ASSERT(!"object load error");
            return false;
        }

        QString id = objectElement.attribute("id", "");
        if(!id.isEmpty())
            pos[id] = loaded;

        objects << loaded;
    }

    // загружаем метки, входящие в сцену
    //
    QDomNodeList domLabels = contents.elementsByTagName("label");
    for(int index = 0; index < domLabels.size(); ++index)
    {
        QDomElement labelElement = domLabels.at(index).toElement();
        PLabel      loaded       = Label::createLabel(newbyScene, labelElement);
        if(!loaded)
        {
            Q_ASSERT(!"label load error");
            return 0;
        }

        QString id = labelElement.attribute("id", "");
        if(!id.isEmpty())
            pos[id] = loaded;

        labels << loaded;
    }

    // загружаем маркеры
    QDomNodeList domMarkers = contents.elementsByTagName("marker");
    for(int index = 0; index < domMarkers.size(); ++index)
    {
        QDomElement markerElement = domMarkers.at(index).toElement();

        QString id       = markerElement.attribute("marker");
        QString pts      = markerElement.attribute("point");
        QString leader   = markerElement.attribute("leader");
        QString follower = markerElement.attribute("follower");

        auto marker = Marker::create(
            point3d::serialLoad(pts), pos.value(leader), pos.value(follower).dynamicCast<Label>());

        Q_ASSERT(marker->follower && marker->leader);
        markerCache[id] = marker;

        markers << marker;
    }
    return true;
}

namespace
{

QString axes2dr(QString axes)
{
    const char *axes_templates[] = {
        "0.000000000000,1.000000000000,0.000000000000",
        "0.000000000000,-1.000000000000,0.000000000000",
        "1.000000000000,0.000000000000,0.000000000000",
        "-1.000000000000,0.000000000000,0.000000000000",
        "0.000000000000,0.000000000000,-1.000000000000",
        "0.000000000000,0.000000000000,1.000000000000",
    };

    if(axes == "1")
        return axes_templates[0];
    if(axes == "2")
        return axes_templates[1];
    if(axes == "4")
        return axes_templates[2];
    if(axes == "8")
        return axes_templates[3];
    if(axes == "16")
        return axes_templates[4];
    if(axes == "32")
        return axes_templates[5];

    Q_ASSERT(!"no case");
    return "";
}

QString xyz2gp(double x, double y, double z)
{
    const int scale   = 5;
    const int delta_x = 75;
    const int delta_y = 440;
    return QString("%1,%2,%3")
        .arg(y * scale + delta_x, 0, 'f', 12)
        .arg(x * scale + delta_y, 0, 'f', 12)
        .arg(-z * scale, 0, 'f', 12);
}

QString classname2class(QString cname)
{
    if(cname == "FreeEnd")
        return "EndCupJoiner";
    if(cname == "Bend")
        return "BendJoiner";
    if(cname == "IConnector")
        return "WeldJoiner";
    if(cname == "Tee")
        return "TeeJoiner";

    if(cname == "Line")
        return "Line";
    if(cname == "Support")
        return "Line";

    if(cname == "Valve")
        return "ValveElement";
    if(cname == "SmallValve")
        return "ValveElement";
    if(cname == "Trans")
        return "TransElement";
    if(cname == "Semisphere")
        return "SemisphereElement";
    if(cname == "FlangesPlusBlind")
        return "FlangePairBlindElement";
    if(cname == "Flanges")
        return "FlangePairElement";
    if(cname == "CondTapper")
        return "CondTapperElement";
    if(cname == "Diaphragm")
        return "DiaphragmElement";
    if(cname == "CheckValve")
        return "CheckValveElement";
    if(cname == "WeldedValve")
        return "ValveElement";
    if(cname == "SmallWeldedValve")
        return "ValveElement";

    return "";
}

void classname2flangeAndScale(QString cname, int &flange, int &scale)
{
    if(cname == "FreeEnd")
        flange = 0, scale = 0;
    if(cname == "Bend")
        flange = 0, scale = 0;
    if(cname == "IConnector")
        flange = 0, scale = 0;
    if(cname == "Tee")
        flange = 0, scale = 0;

    if(cname == "Line")
        flange = 0, scale = 0;
    if(cname == "Support")
        flange = 0, scale = 0;

    if(cname == "Valve")
        flange = 1, scale = 0;
    if(cname == "SmallValve")
        flange = 1, scale = 1;
    if(cname == "Trans")
        flange = 0, scale = 0;
    if(cname == "Semisphere")
        flange = 0, scale = 0;
    if(cname == "FlangesPlusBlind")
        flange = 1, scale = 0;
    if(cname == "Flanges")
        flange = 1, scale = 0;
    if(cname == "CondTapper")
        flange = 1, scale = 0;
    if(cname == "Diaphragm")
        flange = 1, scale = 0;
    if(cname == "CheckValve")
        flange = 1, scale = 0;
    if(cname == "WeldedValve")
        flange = 0, scale = 0;
    if(cname == "SmallWeldedValve")
        flange = 0, scale = 1;
}

} // namespace

bool Document::v1LoadDocument(QDomDocument &document)
{
    QDomNodeList v1list = document.elementsByTagName("PipeS");
    if(!v1list.size())
        return false;
    auto pipeS = v1list.item(0).toElement();

    auto mainList = pipeS.elementsByTagName("MAIN");
    if(!mainList.size())
        return false;
    auto main = mainList.item(0).toElement();

    auto modelList = main.elementsByTagName("model");
    if(!modelList.size())
        return false;
    auto model = modelList.item(0).toElement();

    QSharedPointer<Scene> newbyScene(new Scene(1, true));

    // cache объектов для загрузки
    QMap<QString, PObjectToSelect> posMap;

    QList<PObject> supports;

    auto items = model.elementsByTagName("ITEM");
    for(int index = 0; index < items.size(); ++index)
    {
        QVariantMap vitem;
        auto        item = items.at(index).toElement();

        auto cls = item.attribute("classname");

        auto cls2 = classname2class(cls);
        Q_ASSERT(!cls2.isEmpty());
        vitem["class"] = cls2;

        int isFlange = 0;
        int hasScale = 0;
        classname2flangeAndScale(cls, isFlange, hasScale);
        if(isFlange)
            vitem["flanges"] = 1;
        if(hasScale)
            vitem["scale"] = -2;

        auto id     = item.attribute("no");
        vitem["id"] = id;

        QVariantList vnodes;
        auto         nodes = item.elementsByTagName("NODE");
        for(int jindex = 0; jindex < nodes.size(); ++jindex)
        {
            QVariantMap vnode;
            auto        node = nodes.at(jindex).toElement();

            auto axes = node.attribute("axes");
            auto no   = node.attribute("no");
            auto x    = node.attribute("x").toDouble(0);
            auto y    = node.attribute("y").toDouble(0);
            auto z    = node.attribute("z").toDouble(0);

            vnode["dr"]   = axes2dr(axes);
            vnode["type"] = "0";
            vnode["gp"]   = xyz2gp(x, y, z);
            vnode["no"]   = no.toInt(0);

            vnodes << vnode;
        }

        // сортируем по номеру
        std::sort(vnodes.begin(), vnodes.end(), [](QVariant a, QVariant b) {
            return a.toMap()["no"] < b.toMap()["no"];
        });
        if(vnodes.size() == 3)
        {
            // нормализуем тройник по новым правилам правилам
            auto a = point3d::serialLoad(vnodes[0].toMap()["dr"].toString());
            auto b = point3d::serialLoad(vnodes[1].toMap()["dr"].toString());
            auto c = point3d::serialLoad(vnodes[2].toMap()["dr"].toString());

            if(a.isParallel(b))
                qSwap(vnodes[0], vnodes[2]);
            else if(a.isParallel(c))
                qSwap(vnodes[0], vnodes[1]);
            else if(b.isParallel(c))
            { /*fine*/
            }
        }

        vitem["@children"] = vnodes;

        PObject loadedObject = ObjectFactory::createObject(newbyScene.data(), vitem);
        if(!loadedObject)
        {
            Q_ASSERT(!"object load error");
            return 0;
        }
        newbyScene->attach(loadedObject);

        if(cls == "Support")
        {
            supports << loadedObject;
        }

        if(!id.isEmpty())
            posMap[id] = loadedObject;
    };

    QVariantList v_labels;
    auto         labels = model.elementsByTagName("LABEL");
    for(int index = 0; index < labels.size(); ++index)
    {
        QVariantMap v_label;
        auto        label     = labels.at(index).toElement();
        auto        classname = label.attribute("classname");

        // general
        auto no = label.attribute("no");

        // canvas
        auto info = label.attribute("info");
        //        auto scale = label.attribute("scale").toDouble(0);

        // text
        auto text = QString::fromUtf8(QByteArray::fromBase64(label.attribute("text").toLatin1()));
        auto text_rotate     = label.attribute("text_rotate").toInt(0);
        auto text_size       = label.attribute("text_size").toInt(0);
        auto base_item_index = label.attribute("base_item_index");
        auto base_pos        = label.attribute("base_pos").toDouble();

        QVariantList v_points;
        auto         points = label.elementsByTagName("POINT");
        for(int jindex = 0; jindex < points.size(); ++jindex)
        {
            QVariantMap v_point;
            auto        point = points.at(jindex).toElement();

            auto no = point.attribute("no").toInt(0);
            auto x  = point.attribute("x").toInt(0);
            auto y  = point.attribute("y").toInt(0);

            v_point["no"] = no;
            v_point["x"]  = x;
            v_point["y"]  = y;

            v_points << v_point;
        }

        v_label["points"] = v_points;
        v_labels << v_label;

        if(classname == "Text")
        {
            TextInfo tinfo;

            auto x = v_points[0].toMap()["x"].toInt();
            auto y = v_points[0].toMap()["y"].toInt();

            tinfo.basePoint = geometry::point2d(x, y);
            tinfo.alignment = 0; // влево

            tinfo.setTextScaleNo(text_size * 2 / 3);

            tinfo.rotationAngleDegrees = text_rotate * 30;
            tinfo.setText(text);

            auto label = TextLabel::create(newbyScene.data(), tinfo);
            newbyScene->attachLabel(label);

            if(!base_item_index.isEmpty())
            {
                PObjectToSelect leader = posMap[base_item_index];
                if(PObject p = leader.dynamicCast<Object>())
                {
                    auto point = p->globalPoint(0).partition_by(p->globalPoint(1), base_pos);

                    auto marker = Marker::create(point, leader, label);
                    newbyScene->attachMarker(marker);
                }
            }
        }
        if(classname == "Canvas")
        {
            auto x     = v_points[0].toMap()["x"].toInt();
            auto y     = v_points[0].toMap()["y"].toInt();
            auto delta = point2d(x, y);

            CanvasInfo inf = CanvasInfo::def();
            inf.leftTop += delta;
            inf.rightBottom += delta;
            inf.center += delta;

            auto canvas = CanvasRectangle::create(newbyScene.data(), inf);
            newbyScene->attachLabel(canvas);
        }
    }

    while(!supports.isEmpty())
    {
        PObject line = supports.takeLast();

        PNode   l1   = newbyScene->findConnectedNode(line->nodeAt(0), true).node();
        PObject left = l1->object();
        PNode   l2   = geometry::secondNode(l1);

        PObject newline = Line::create(
            newbyScene.data(), line->globalPoint(1), l2->globalPoint(), Object::NoSample);
        Command cmd = geometry::cmdReplaceObject2to1(newbyScene.data(), line, left, newline);

        PNode   l3    = newbyScene->findConnectedNode(line->nodeAt(1), true).node();
        PObject right = l3->object();
        PNode   l4    = geometry::secondNode(l3);

        PObject newline2 =
            Line::create(newbyScene.data(), l2->globalPoint(), l4->globalPoint(), Object::NoSample);
        cmd << cmdReplaceObject2to1(newbyScene.data(), newline, right, newline2);

        // вставляем непосредственно опору
        cmd << cmdPlaceLabelAndMarker(
            newbyScene.data(), 0, newline2, (line->globalPoint(0) + line->globalPoint(1)) / 2);

        cmd.doit();

        if(supports.contains(left) || supports.contains(right))
        {
            supports.removeAll(left);
            supports.removeAll(right);
        }
    }

    // абра кадабра
    newbyScene->swap(scene_);
    emit replaceScene();

    return true;
}

} // namespace geometry
