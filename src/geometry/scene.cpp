#include "scene.h"
#include "document.h"
#include "global.h"
#include "joiner.h"
#include "line.h"
#include "manipulator.h"
#include "marker.h"
#include "neighbourhood.h"
#include "sceneImpl.h"
#include "sceneProcedures.h"
#include "straightLineAdorner.h"

#include <algorithm>

#include <QMap>
#include <QMultiMap>
#include <QPainter>
#include <QSet>
#include <QtGlobal>

#include "documentAutoSaver.h"
#include "glueObject.h"

#include <tuple>

namespace geometry
{

Scene::Scene(int type, bool empty)
    : pimpl(new SceneImpl(type))
    , manipulator_(new Manipulator(this))
    , document_(new Document(this))
{
    pimpl->scene = this;

    // включаем автосохранение
    new DocumentAutoSaver(document());

    if(!empty)
    {
        pimpl->initCanvas(this, empty);
    }
}

QGraphicsScene *Scene::graphicsScene() const
{
    return &pimpl->gscene;
}

void Scene::preparePrinter(QPrinter *printer, int canvasNo)
{
    CanvasInfo info = canvas(canvasNo)->info();

    if(info.landscape)
    {
        printer->setPageOrientation(QPageLayout::Landscape);
    }
    else
    {
        printer->setPageOrientation(QPageLayout::Portrait);
    }

    QPrinter::PageSize size = QPrinter::PageSize::A4;
    if(info.size == CanvasSize::A3)
        size = QPrinter::PageSize::A3;
    else if(info.size == CanvasSize::A5)
        size = QPrinter::PageSize::A5;
    printer->setPageSize(size);
    printer->setFullPage(true);

    printer->setMargins({0, 0, 0, 0});
}

void Scene::render(QPainter *painter, QPrinter *printer, int canvasNo)
{
    // запоминаем выделенные и ховер элементы
    decltype(pimpl->hoverObjects) hos;
    qSwap(hos, pimpl->hoverObjects);

    decltype(pimpl->selectedObjects) sos;
    qSwap(sos, pimpl->selectedObjects);

    point2d cp = cursorPoint();
    manipulator()->moveOut();

    // обновляем элементы без заданного стиля
    auto allSelectedObject = hos.keys().toSet() | sos;
    pimpl->updateSelection(allSelectedObject);

    auto ads = pimpl->adorners;
    foreach(auto a, ads)
        if(a->isTemporal())
            pimpl->detachAdorner(a);

    clearGrips();

    bool drawCanvases = false;
    if(!drawCanvases)
        foreach(PCanvasRectangle c, pimpl->canvases)
            pimpl->detachGraphicsItem(c);

    // корректируем настройки страницы принтера в соответствии с выбранным канвасом

    int printedPageCount = 0;
    for(int index = 0; index < canvasCount(); ++index)
    {
        int pageNo = index + 1;

        // пропускаем ненужные страницы
        if(printer && printer->fromPage() && pageNo < printer->fromPage())
            continue;
        if(printer && printer->toPage() && pageNo > printer->toPage())
            continue;
        if(!printer && canvasNo && pageNo != canvasNo)
            continue;

        // печать страницы
        if(printer && printedPageCount != 0)
        {
            // можно менять ориентирование страницы, но не размер
            //            preparePrinter(printer, index);
            printer->newPage();
        }

        CanvasInfo const &inf = canvas(index)->info();
        QRectF            source(inf.leftTop.toQPoint(), inf.rightBottom.toQPoint());

        // в ручную определяем размеры полей страницы, поскольку qt лажает
        // но не забываем, что printer может быть равен 0
        QRect page;
        if(printer)
        {
            auto paper = printer->paperRect(QPrinter::DevicePixel).toRect();
            auto lay   = printer->pageLayout();
            lay.setUnits(QPageLayout::Millimeter);

            auto mar = lay.minimumMargins();
            auto res = printer->paperRect(QPrinter::Unit::DevicePixel).width() /
                       printer->paperRect(QPrinter::Unit::Millimeter).width();

            mar *= res;
            page = paper.adjusted(+mar.left(), +mar.top(), -mar.right(), -mar.bottom());

            // пересчитываем прямоугольник, чтобы он был в середине страницы
            double dx = page.width() - page.height() * source.width() / source.height();
            if(dx > 0)
                page.adjust(dx / 2, 0, -dx / 2, 0);
            double dy = page.height() - page.width() * source.height() / source.width();
            if(dy > 0)
                page.adjust(0, dy / 2, 0, -dy / 2);
        }
        else
        {
            page = painter->window();
        }

        // рисование
        graphicsScene()->update();
        graphicsScene()->render(painter, page, source, Qt::KeepAspectRatio);

        ++printedPageCount;
        if(!printer)
            break;
    }

    if(!drawCanvases)
        foreach(PCanvasRectangle c, pimpl->canvases)
            pimpl->updateGraphicsItem(c, 1);

    // возвращаем стиль на место
    setCursorPoint(cp);
    manipulator()->moveToTheSamePoint();

    foreach(auto a, ads)
        if(a->isTemporal())
            pimpl->attachAdorner(a);
    qSwap(hos, pimpl->hoverObjects);
    qSwap(sos, pimpl->selectedObjects);
    pimpl->updateSelection(allSelectedObject);
}

void Scene::swap(Scene *other)
{
    auto prepareSwap = [&](Scene *self) { self->manipulator_->cancel(); };

    prepareSwap(this);
    prepareSwap(other);

    // подменяем владельцев объектов и меток
#define REPL_S(A, B, C)                                                                            \
    foreach(auto object, A->pimpl->C)                                                              \
    object->replaceScene(B)

    REPL_S(this, other, objects);
    REPL_S(other, this, objects);
    REPL_S(this, other, labels);
    REPL_S(other, this, labels);
#undef REPL_S

    // подмена сущностей
    qSwap(this->pimpl, other->pimpl);
    // ссылку на сцены менять не надо
    qSwap(this->pimpl->scene, other->pimpl->scene);

    auto afterSwap = [&](Scene *self) { self->setWindowToCanvasRectangle(0); };

    afterSwap(this);
    afterSwap(other);
}

Document *Scene::document() const
{
    return document_;
}

Scene::~Scene()
{
    delete manipulator_;
    delete document_;
    delete pimpl;
}

PWorldSystem Scene::worldSystem() const
{
    return pimpl->worldSystem;
}

void Scene::setWorldSystem(PWorldSystem pl)
{
    pimpl->worldSystem = pl;
}

Manipulator *Scene::manipulator() const
{
    return manipulator_;
}

SceneProperties Scene::getPropertiesFromScene()
{
    SceneProperties props;

    props.defaultTextInfo    = defaultTextInfo();
    props.defaultLineInfo    = defaultLineInfo();
    props.defaultElementInfo = defaultElementInfo();
    props.loadDefaults();

    props.selection = selectedObjects();
    manipulator_->takeToolProperty(props);

    props.loadSelected();

    return props;
}

QList<int> Scene::textAngles() const
{
    WorldSystem &ws = *worldSystem();
    QList<int>   angles0;
    angles0 << 0 << -90 << (int)round(ws.xangle()) << (int)round(ws.xangle() + 180)
            << (int)round(ws.yangle()) << (int)round(ws.yangle() + 180);

    QList<int> angles;
    foreach(int a, angles0)
    {
        // нормируем их
        a = (360 + a) % 360;
        if(a > 180)
            a -= 360;
        if(-90 <= a && a <= 90)
            if(!angles.contains(a))
                angles << a;
    }
    std::sort(angles.begin(), angles.end());
    return angles;
}

void Scene::attach(PObject po)
{
    Q_ASSERT(!pimpl->objects.contains(po));
    pimpl->objects << po;
    pimpl->onAttachObject(po);
    emit update();
}

void Scene::detach(PObject po)
{
    Q_ASSERT(pimpl->objects.contains(po));
    pimpl->objects.remove(po);
    pimpl->onDetachObject(po);

    pimpl->selectedObjects.remove(po);
    pimpl->hoverObjects.remove(po);

    emit update();
}

void Scene::attachLabel(PLabel label)
{
    Q_ASSERT(!pimpl->labels.contains(label));

    if(auto canvas = label.dynamicCast<CanvasRectangle>())
    {
        pimpl->addCanvas(canvas);
    }

    pimpl->labels << label;
    pimpl->invalidatedObjects << label;
    //    pimpl->updateGraphicsItem(label, 1);

    emit update();
}

void Scene::detachLabel(PLabel label)
{
    Q_ASSERT(pimpl->labels.contains(label));

    pimpl->labels.remove(label);
    pimpl->detachGraphicsItem(label);

    if(auto canvas = label.dynamicCast<CanvasRectangle>())
        pimpl->canvases.removeOne(canvas);

    pimpl->selectedObjects.remove(label);
    pimpl->hoverObjects.remove(label);

    emit update();
}

void Scene::updateLabel(PLabel label)
{
    Q_ASSERT(pimpl->labels.contains(label));
    if(label->isActiveLabel())
        pimpl->updateGraphicsItem(label, 1);
    else
        pimpl->invalidatedObjects << label;

    emit update();
}

void Scene::attachMarker(PMarker marker)
{
    Q_ASSERT(marker->leader && marker->follower);
    if(pimpl->markers.contains(marker))
        return;

    pimpl->markers << marker;
    pimpl->markerLeaders.insert(marker->leader, marker);
    pimpl->markerFollower.insert(marker->follower, marker);

    if(pimpl->labels.contains(marker->follower))
        updateLabel(marker->follower);
}

void Scene::detachMarker(PMarker marker)
{
    // наден
    pimpl->markerLeaders.remove(marker->leader, marker);
    pimpl->markerFollower.remove(marker->follower, marker);
    pimpl->markers.remove(marker);

    // маркер удаляется, значит
    // метка тоже (вероятно) удаляется
    if(pimpl->labels.contains(marker->follower))
        updateLabel(marker->follower);
}

QList<PMarker> Scene::markersOfLeader(PObjectToSelect object) const
{
    return pimpl->markerLeaders.values(object);
}

QList<PMarker> Scene::markers() const
{
    return pimpl->markers.toList();
}

void Scene::attachGrip(PGrip gr)
{
    Q_ASSERT(pimpl->grips.contains(gr));

    pimpl->grips.append(gr);
    pimpl->updateGraphicsItem(gr, 1);

    emit update();
}

void Scene::detachGrip(PGrip gr)
{
    pimpl->updateGraphicsItem(gr, 0);
    pimpl->grips.removeAll(gr);

    emit update();
}

QList<PMarker> Scene::markersOfFollower(PLabel follower) const
{
    return pimpl->markerFollower.values(follower);
}

QByteArray Scene::stringCopySelection()
{
    QSet<PObjectToSelect> total;
    foreach(auto it, pimpl->objects)
        total << it;
    foreach(auto it, pimpl->labels)
        if(!it.dynamicCast<CanvasRectangle>())
            total << it;

    auto selected = pimpl->selectedObjects;
    auto toDelete = total - selected;

    // добавляем Glue объекты
    QSet<PObjectToSelect> glues;
    foreach(auto it, toDelete)
    {
        if(auto glue = it.dynamicCast<GlueObject>())
        {
            auto elements = neighbours(this, glue);
            if(elements.size() < 2)
                continue;

            if(selected.contains(elements[0]) && selected.contains(elements[1]))
                glues.insert(glue);
        }
    }
    selected += glues;
    toDelete -= glues;

    EraseData data;
    eraseObjectsToSelect(this, data, toDelete);

    QSet<PObject> copyingObjects;
    QSet<PLabel>  copyingLabels;
    foreach(auto it, total)
        if(auto obj = it.dynamicCast<Object>())
            copyingObjects << obj;
        else if(auto lab = it.dynamicCast<Label>())
            copyingLabels << lab;

    copyingObjects += data.toAdd;
    copyingObjects += data.replaceMap.values().toSet();
    copyingObjects -= data.toDelete;
    copyingObjects -= data.toReplace;

    copyingLabels -= data.labelsToDelete;

    QSet<PMarker> copyingMarkers = pimpl->markers;
    copyingMarkers -= data.toEraseMarkers.toSet();
    copyingMarkers -= data.toReconnectMarkers.toSet();
    copyingMarkers += data.markerReplaceMap.values().toSet();

    if(!copyingLabels.size() && !copyingObjects.size())
    {
        return QByteArray();
    }

    QDomDocument doc;
    auto         contents = doc.createElement("sktx_copyclip_contents");
    doc.appendChild(contents);

    contents.setAttribute("planeType", planeType());

    // сохранение
    Document::saveSceneData(doc, contents, copyingObjects, copyingLabels, copyingMarkers.toList());

    QByteArray  array;
    QTextStream stream(&array);
    doc.save(stream, 2);
    return array;
}

PNeighbourhood Scene::getObjectsRectangle(point2d a, point2d b) const
{
    return pimpl->getObjectsRectangle(a, b);
}

ObjectInfo Scene::analizeCloseObject(PObjectToSelect sel, point2d b, bool preferNodes) const
{
    auto nei = PNeighbourhood(new Neighbourhood);
    pimpl->analizeCloseObject(nei, sel, b, preferNodes);
    return nei->objectInfo[sel.dynamicCast<Object>()];
}

void Scene::analizeCloseObject(PNeighbourhood  nei,
                               PObjectToSelect sel,
                               point2d         b,
                               bool            preferNodes) const
{
    return pimpl->analizeCloseObject(nei, sel, b, preferNodes);
}

PNeighbourhood Scene::getClosestObjects(point2d a, point2d b) const
{
    return pimpl->getClosestObjects(a, b, false);
}

PNeighbourhood Scene::getClosestObjectsEx(point2d a, double radius) const
{
    return pimpl->getClosestObjects(a, a, radius);
}

PNeighbourhood Scene::getClosestObjects(point2d pt) const
{
    return pimpl->getClosestObjects(pt, pt, false);
}

void Scene::clear()
{
    manipulator()->setToolInfo(ToolSetFactory::inst().toolLine());

    pimpl->objects.clear();
    pimpl->labels.clear();
    pimpl->selectedObjects.clear();
    pimpl->hoverObjects.clear();
    pimpl->adorners.clear();
    pimpl->nodesCache.clear();

    pimpl->gscene.clear();
    pimpl->gitems.clear();
    pimpl->gcontur.clear();

    pimpl->invalidatedAdornerObjects.clear();
    pimpl->invalidatedCrossObjects.clear();
    emit update();
}

bool Scene::noCursorPoint() const
{
    pimpl->updateCursorItems();
    return pimpl->cursorPoint == INCORRECT_POINT;
}

void Scene::setCursorPoint()
{
    pimpl->cursorPoint = INCORRECT_POINT;
    pimpl->updateCursorItems();
    emit update();
}

void Scene::setHoverObjects(QSet<PObjectToSelect> objects, HoverState state, HoverState mask)
{
    foreach(PObjectToSelect object, objects)
    {
        auto &hs = pimpl->hoverObjects[object];
        hs       = static_cast<HoverState>((hs & (~mask)) | (state & mask));

        // если элемент не имеет состояния, то удаляем его из списка
        if(hs == NoHover)
        {
            pimpl->hoverObjects.remove(object);
        }

        // если элемент выделен, то дублируем информацию в отдельном поле
        if(mask & Selected)
        {
            if(state & Selected)
                pimpl->selectedObjects << object;
            else
                pimpl->selectedObjects.remove(object);
        }
    }

    // обновляем элементы
    pimpl->updateSelection(objects);
}

void Scene::setHoverObjects(QMap<PObjectToSelect, HoverState> const &ho)
{
    // очищаем состояние предыдущих ховер объектов
    auto prev = pimpl->hoverObjects.keys().toSet() - ho.keys().toSet();
    setHoverObjects(prev, NoHover, static_cast<HoverState>(HasContour));

    // устаналиваем новые состояния элементов
    foreach(PObjectToSelect obj, ho.keys())
    {
        QSet<PObjectToSelect> s;
        s << obj;
        setHoverObjects(s, ho[obj], static_cast<HoverState>(HasContour));
    }
}

QSet<PObjectToSelect> Scene::getHoverObjects(HoverState state) const
{
    QSet<PObjectToSelect> result;
    foreach(auto obj, pimpl->hoverObjects.keys())
    {
        if(pimpl->hoverObjects[obj] & state)
            result << obj;
    }
    return result;
}

void Scene::setCursorPoint(point2d pt)
{
    pimpl->cursorPoint = pt;
    pimpl->updateCursorItems();

    emit update();
}

point2d Scene::cursorPoint() const
{
    return pimpl->cursorPoint;
}

double Scene::scale() const
{
    return pimpl->scale;
}

double Scene::epsilon() const
{
    return pimpl->epsilon();
}

void Scene::setCursor(QString s)
{
    onSetCursor(s);
}

void Scene::setTextEdit(ITextEditor *info)
{
    onSetTextEdit(info);
}

void Scene::updateVisibleWindow(point2d lt, point2d rb, double scale)
{
    pimpl->visibleLt_ = lt;
    pimpl->visibleRb_ = rb;
}

int Scene::bestVisibleCanvas() const
{
    QPointF point = ((pimpl->visibleLt_ + pimpl->visibleRb_) / 2).toQPoint();

    QList<QRectF> rects;
    foreach(PCanvasRectangle canvas, pimpl->canvases)
    {
        auto rect = canvas->makeQRect();
        rects << rect;

        if(rect.contains(point))
            return pimpl->canvases.indexOf(canvas);
    }

    auto iter = std::min_element(rects.begin(), rects.end(), [&](QRectF &a, QRectF &b) {
        auto aa = a.center() - point;
        auto bb = b.center() - point;
        return aa.x() * aa.x() + aa.y() * aa.y() < bb.x() * bb.x() + bb.y() * bb.y();
    });

    int index = iter - rects.begin();
    return index;
}

int Scene::canvasCount() const
{
    return pimpl->canvases.size();
}

point2d Scene::visibleLeftTop() const
{
    return pimpl->visibleLt_;
}

point2d Scene::visibleRightBottom() const
{
    return pimpl->visibleRb_;
}

void Scene::setRecomendedWindow(point2d lt, point2d rb)
{
    pimpl->recomendedLt_ = lt;
    pimpl->recomendedRb_ = rb;

    emit viewRectangleChanged();
}

point2d Scene::recomendedLeftTop() const
{
    return pimpl->recomendedLt_;
}
point2d Scene::recomendedRightBottom() const
{
    return pimpl->recomendedRb_;
}

PCanvasRectangle Scene::canvas(int index) const
{
    index = qMax(0, qMin(index, pimpl->canvases.size() - 1));
    return pimpl->canvases[index];
}

void Scene::setWindowToCanvasRectangle(int index)
{
    auto lt = canvas(0)->info().leftTop;
    auto rb = canvas(0)->info().rightBottom;
    for(int index = 1; index < canvasCount(); ++index)
    {
        auto p = canvas(index)->info().leftTop;
        lt.x   = std::min(p.x, lt.x);
        lt.y   = std::min(p.y, lt.y);

        p    = canvas(index)->info().rightBottom;
        rb.x = std::max(p.x, rb.x);
        rb.y = std::max(p.y, rb.y);
    }

    setRecomendedWindow(lt, rb);
}

void Scene::setWindowToModelExtents()
{
    QRectF rect;
    foreach(PObject o1, pimpl->objects)
    {
        foreach(PGItem item,
                pimpl->gcontur.value(o1.dynamicCast<ObjectToSelect>(), GItemsList(&pimpl->gscene)))
        {
            rect |= item->boundingRect();
        };
    }

    double top = rect.top(), bottom = rect.bottom(), left = rect.left(), right = rect.right();

    auto de = qMax((recomendedLeftTop() - point2d(left, top)).length(),
                   (recomendedRightBottom() - point2d(right, bottom)).length());

    if(left == right || top == bottom || de < 10)
        setWindowToCanvasRectangle(0);
    else
        setRecomendedWindow(point2d(left, top), point2d(right, bottom));
}

static auto isTemporal = [](auto a, auto b) -> bool { return a->isTemporal() < b->isTemporal(); };

void Scene::clearAdorners()
{ // удаляем временные, остальные оставляем

    std::sort(pimpl->adorners.begin(), pimpl->adorners.end(), isTemporal);
    while(!pimpl->adorners.empty())
    {
        PAdorner last = pimpl->adorners.last();
        if(!last->isTemporal())
            break;
        pimpl->detachAdorner(last);
    }

    emit update();
}

void Scene::pushAdorner(Adorner *ado)
{
    pushAdorner(PAdorner(ado));
}

void Scene::pushAdorner(PAdorner ado)
{
    Q_ASSERT(ado);
    pimpl->attachAdorner(ado);
    emit update();
}

QList<PAdorner> Scene::getAdorners() const
{
    return pimpl->adorners;
}

void Scene::clearGrips()
{
    std::sort(pimpl->grips.begin(), pimpl->grips.end(), isTemporal);
    while(!pimpl->grips.empty())
    {
        auto last = pimpl->grips.last();
        if(!last->isTemporal())
            break;
        pimpl->detachGrip(last);
    }

    emit update();
}

void Scene::pushGrip(PGrip grip)
{
    Q_ASSERT(grip);
    pimpl->attachGrip(grip);
    emit update();
}

void Scene::removeGrip(PGrip old)
{
    pimpl->detachGrip(old);
    emit update();
}

QList<PGrip> Scene::getGrips() const
{
    return pimpl->grips;
}

/**
 * параметр force заставляет искать ноды, которые геометрически расположены
 * как надо но не могут быть связаны непосредственно по свойствам объектов
 * например линия-линия
 */
ConnectionData Scene::findConnectedNode(PNode node, bool force) const
{
    return pimpl->findConnectedNode(node, force);
}

void Scene::recalcCaches() const
{
    const_cast<SceneImpl *>(pimpl)->recalcCaches();
}

void Scene::recalcDeepCaches() const
{
    const_cast<SceneImpl *>(pimpl)->recalcDeepCaches();
}

void Scene::clearOldGItemsCache() const
{
    const_cast<SceneImpl *>(pimpl)->clearOldGItemsCache();
}

bool Scene::isSelected(PObjectToSelect object) const
{
    return pimpl->selectedObjects.contains(object);
}

QSet<PObjectToSelect> const &Scene::selectedObjects() const
{
    return pimpl->selectedObjects;
}

void Scene::addSelectedObjects(QSet<PObjectToSelect> mod)
{
    setHoverObjects(mod, Selected, Selected);
}

void Scene::removeSelectedObjects(QSet<PObjectToSelect> mod)
{
    setHoverObjects(mod, NoHover, Selected);
}

void Scene::clearSelection()
{
    setHoverObjects(pimpl->selectedObjects, NoHover, Selected);
}

bool Scene::hasUnselected() const
{
    // определяем выделены ли все элементы
    QSet<PObjectToSelect> so;

    // все элементы
    foreach(auto p, labels())
        so.insert(p);
    foreach(auto p, objects())
        so.insert(p);

    foreach(auto c, pimpl->canvases)
        so.remove(c);

    // отнимаем выделенные
    so -= selectedObjects();
    return !so.empty();
}

LineInfo Scene::defaultLineInfo() const
{
    return pimpl->defaultLineInfo;
}

void Scene::setDefaultLineInfo(LineInfo v)
{
    pimpl->defaultLineInfo = v;
}

TextInfo Scene::defaultTextInfo() const
{
    return pimpl->defaultTextInfo;
}

void Scene::setDefaultTextInfo(TextInfo value)
{
    pimpl->defaultTextInfo = value;
}

ElementInfo Scene::defaultElementInfo() const
{
    return pimpl->defaultElementInfo;
}

void Scene::setDefaultElementInfo(ElementInfo el)
{
    pimpl->defaultElementInfo = el;
}

QSet<PLabel> Scene::labels() const
{
    return pimpl->labels;
}

void Scene::invalidateObject(PObject obj)
{
    //    pimpl->updateGraphicsItem(obj);
    pimpl->invalidatedObjects << obj;

    emit update();
}

QSet<PObject> Scene::objects() const
{
    return pimpl->objects;
}

bool Scene::contains(PObject object) const
{
    return pimpl->objects.contains(object);
}

int Scene::modifyCounter() const
{
    return pimpl->modifyCounter;
}

void Scene::modify(int delta)
{
    pimpl->modifyCounter += delta;
    emit updateUI();
}

int Scene::planeType() const
{
    return pimpl->wsType;
}

QStringList Scene::dumpContents() const
{
    return pimpl->dumpContents();
}

} // namespace geometry
