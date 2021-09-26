#include "manipulatorTools.h"
#include "line.h"
#include "manipulator.h"
#include "neighbourhood.h"
#include "sceneProcedures.h"

#include <QStringList>
#include <QVariant>

namespace geometry
{

struct closer_to_clicked
{
    WorldSystem *pw;
    point2d      clicked;
    closer_to_clicked(WorldSystem *pw, point2d clicked)
        : pw(pw)
        , clicked(clicked)
    {
    }

    bool operator()(point3d a, point3d b) const
    {
        return pw->distance2d(clicked, a) < pw->distance2d(clicked, b);
    }
};

ManipulatorTool::ManipulatorTool(Manipulator *m)
    : manipulator_(m)
    , scene_(m->scene_)
    , joinToPrevious_(0)
    , abortPrevious_(0)
    , preserveSelection_(0)
    , supportModes_(Clicking)
    , fastRedrawPossible_(false)
{
}

bool ManipulatorTool::support(int mode) const
{
    return supportModes_ & mode;
}

ManipulatorTool::~ManipulatorTool()
{
}

QString ManipulatorTool::helperTab = "<br>&gt;&nbsp;&nbsp;";

QString ManipulatorTool::helperText() const
{
    QStringList strs;
    strs << QString::fromUtf8("параллельно осям") << QString::fromUtf8("в горизонтальной плоскости")
         << QString::fromUtf8("в плоскости X-Z") << QString::fromUtf8("в плоскости Y-Z");

    QString text = QString::fromUtf8("нажимите <b>TAB</b> для выбора плоскости (");

    QStringList modes;
    for(int index = 0; index < strs.size(); ++index)
    {
        if(!planeMakeSense(scene_->worldSystem(), index))
            continue;

        if(index == manipulator_->planeMode())
            modes << QString::fromUtf8("<big><b>%1</b></big>").arg(strs[index]);
        else
            modes << strs[index];
    }
    text += modes.join(",") + QString(")");

    return text;
}

void ManipulatorTool::move(point2d pt)
{
    manipulator_->debugLog("move");

    rollbackAndPrepare();
    PNeighbourhood n = scene_->getClosestObjects(pt);

    ManipulatorLogger logger(manipulator_, scene_);
    logger.reportTool(this);

    int s, c, a;
    std::tie(s, c, a) = manipulator_->modifiers();
    if(s || c || a)
        logger << QString("modifiers %1%2%3")
                      .arg(!s ? "_" : "S")
                      .arg(!c ? "_" : "C")
                      .arg(!a ? "_" : "A");
    logger << QString("move %1").arg(pt.serialSave());

    do_move(pt, n);

    // устанавливаем активные объекты
    scene_->setHoverObjects(n->hoverState);
    scene_->setCursor(cursorName_);
    scene_->setCursorPoint(pt);

    // запись результатов движения
    logger.makeLogRecord();
}

void ManipulatorTool::drag(point2d pt, bool started)
{
    manipulator_->debugLog("drag");

    rollbackAndPrepare();
    PNeighbourhood n = scene_->getClosestObjects(manipulator_->previousDragPosition(), pt);

    do_drag(pt, n, started);

    scene_->setHoverObjects(n->hoverState);
    scene_->setCursor(cursorName_);
    scene_->setCursorPoint(pt);
}

void ManipulatorTool::drop(point2d pt)
{
    manipulator_->debugLog("drop");

    rollbackAndPrepare();
    PNeighbourhood n = scene_->getClosestObjects(manipulator_->previousDragPosition(), pt);
    do_drop(pt, n);

    scene_->setHoverObjects(n->hoverState);
    scene_->setCursor(cursorName_);
    scene_->setCursorPoint(pt);

    if(!preserveSelection_)
        scene_->clearSelection();

    commit();
}

void ManipulatorTool::changeSize(int sizeF, bool absolute)
{
    do_changeSize(sizeF, absolute);
}

void ManipulatorTool::toggleMode()
{
    do_toggleMode();
}

bool ManipulatorTool::isFastRedrawPossible() const
{
    return fastRedrawPossible_;
}

bool ManipulatorTool::updateToolProperties(ScenePropertyValue v)
{
    return do_updateToolProperties(v);
}

ManipulatorLogger::ManipulatorLogger(Manipulator *manip, Scene *scene)
    : manip(manip)
    , scene(scene)
{
    snapshot = scene->objects();
}

ManipulatorLogger &ManipulatorLogger::operator<<(QString text)
{
    result << text;
    return *this;
}

static QString objectToString(PObject object)
{
    QString result = object->typeName();
    foreach(PNode node, object->nodes())
    {
        result += " " + node->globalPoint().toQString();
    }
    return result;
}

void ManipulatorLogger::makeLogRecord()
{
    auto after    = scene->objects();
    auto appended = after - snapshot;
    auto removed  = snapshot - after;

    QStringList result = this->result;
    result << QString("# scene contains %1 objects").arg(after.size());

    if(appended.size() || removed.size())
        result << "# some objects were modified: ";

    foreach(PObject object, appended)
        result << "+" + objectToString(object);

    foreach(PObject object, removed)
        result << "-" + objectToString(object);

    manip->logAppendCommandText(result);
}

void ManipulatorLogger::reportTool(QObject *tool)
{
    QString toolString = QString(tool->metaObject()->className()).section("::", 1);
    (*this) << QString("\n#> %1").arg(toolString);
}

void ManipulatorTool::click(point2d pt)
{
    manipulator_->debugLog("click");

    rollbackAndPrepare();
    PNeighbourhood n = scene_->getClosestObjects(pt);

    ManipulatorLogger logger(manipulator_, scene_);
    logger.reportTool(this);

    int s, c, a;
    std::tie(s, c, a) = manipulator_->modifiers();
    if(s || c || a)
        logger << QString("modifiers %1%2%3")
                      .arg(!s ? "_" : "S")
                      .arg(!c ? "_" : "C")
                      .arg(!a ? "_" : "A");
    logger << QString("click %1").arg(pt.serialSave());

    do_click(pt, n);

    // устанавливаем активные объекты
    scene_->setHoverObjects(n->hoverState);
    scene_->setCursor(cursorName_);
    scene_->setCursorPoint(pt);

    if(!preserveSelection_)
        scene_->clearSelection();

    // принимаем выделения
    logger.makeLogRecord();
    commit();
}

void ManipulatorTool::doubleClick(point2d pt)
{
    manipulator_->debugLog("doubleClick");

    rollbackAndPrepare();
    PNeighbourhood n = scene_->getClosestObjects(pt);

    ManipulatorLogger logger(manipulator_, scene_);
    logger.reportTool(this);

    int s, c, a;
    std::tie(s, c, a) = manipulator_->modifiers();
    if(s || c || a)
        logger << QString("modifiers %1%2%3")
                      .arg(!s ? "_" : "S")
                      .arg(!c ? "_" : "C")
                      .arg(!a ? "_" : "A");
    logger << QString("doubleClick %1").arg(pt.serialSave());

    do_doubleClick(pt, n);

    // устанавливаем активные объекты
    scene_->setHoverObjects(n->hoverState);
    scene_->setCursor(cursorName_);
    scene_->setCursorPoint(pt);

    if(!preserveSelection_)
        scene_->clearSelection();

    // принимаем выделения
    logger.makeLogRecord();
    commit();
}

void ManipulatorTool::commit()
{
    manipulator_->debugLog("commit");

    do_commit();

    if(commandList_)
    {
        // маркируем изменение только когда комманда наверняка выполнена
        commandList_.addAndExecute(cmdModify(scene_));

        // добавляем команду в список Undo
        manipulator_->appendCommand(takeCommandList(), joinToPrevious());
    }

    if(abortPrevious())
    {
        manipulator_->removeLastCommand();
    }

    if(auto t = nextTool())
    {
        // при изменении иструмента вызываются виртуальные
        // члены инструментов прошлого и будущего
        manipulator_->setTool(t, false);
    }
}

void ManipulatorTool::changeRotation(int d)
{
    do_changeRotation(d);
}

void ManipulatorTool::do_setUp()
{
}

void ManipulatorTool::do_tearDown()
{
}

void ManipulatorTool::setUp()
{
    manipulator_->debugLog("setUp");

    do_setUp();
}

void ManipulatorTool::tearDown()
{
    manipulator_->debugLog("tearDown");

    do_tearDown();
}

void ManipulatorTool::rollbackAndPrepare()
{
    // отменяем предыдущие действия
    rollback();
    scene_->recalcCaches();
    scene_->clearOldGItemsCache();

    manipulator_->debugLog("prepare");

    // выполняем подготовку именно этого действия
    do_prepare();
}

void ManipulatorTool::rollback()
{
    manipulator_->debugLog("rollback");

    // отмена команд (дублируется в prepare)
    commandList_.undoAll();
    commandList_.clear();

    setCursor("");
    clearNextTool();

    // сбрасываем адорнеры
    scene_->clearAdorners();
    scene_->clearGrips();

    do_rollback();
}

void ManipulatorTool::do_prepare()
{
}

void ManipulatorTool::do_rollback()
{
}

void ManipulatorTool::do_move(point2d start, PNeighbourhood nei)
{
    do_click(start, nei);
}

void ManipulatorTool::do_click(point2d, PNeighbourhood)
{
}

void ManipulatorTool::do_doubleClick(point2d, PNeighbourhood)
{
}

void ManipulatorTool::do_drag(point2d, PNeighbourhood, bool)
{
}

void ManipulatorTool::do_drop(point2d, PNeighbourhood)
{
}

void ManipulatorTool::do_commit()
{
}

void ManipulatorTool::do_changeRotation(int d)
{
}

void ManipulatorTool::do_changeSize(int sizeF, bool absolute)
{
}

void ManipulatorTool::do_toggleMode()
{
}

void ManipulatorTool::do_takeToolProperty(SceneProperties &)
{
}

void ManipulatorTool::takeToolProperty(SceneProperties &prop)
{
    return do_takeToolProperty(prop);
}

bool ManipulatorTool::do_updateToolProperties(ScenePropertyValue)
{
    return false;
}

UndoList ManipulatorTool::takeCommandList()
{
    UndoList copy;
    qSwap(copy, commandList_);
    return copy;
}

void ManipulatorTool::clearNextTool()
{
    nextTool_.clear();
}

PManipulatorTool ManipulatorTool::nextTool() const
{
    return nextTool_;
}

QList<point3d> ManipulatorTool::generateDirections() const
{
    QList<point3d> variants;

    variants << point3d(1, 0, 0) << point3d(-1, 0, 0) << point3d(0, 1, 0) << point3d(0, -1, 0)
             << point3d(0, 0, 1) << point3d(0, 0, -1);

    if(int pm = manipulator_->planeMode())
    {
        QList<point3d> current;
        switch(pm)
        {
        case 1:
            current << point3d(1);
            current << point3d(0, 1);
            break;
        case 2:
            current << point3d(1);
            current << point3d(0, 0, 1);
            break;
        case 3:
            current << point3d(0, 1);
            current << point3d(0, 0, 1);
            break;
        }

        QList<point3d> alters;
        alters << current[0] << current[1];

        point3d       dbase = current[0];
        point3d       dv    = current[1];
        QList<double> angles, sins, coss;
        angles << 0 << 90 << -90 << -180 << 15 << 30 << 45 << 60 << 75 << -15 << -30 << -45 << -60
               << -75 << 105 << 120 << 135 << 150 << 165 << -105 << -120 << -135 << -150 << -165;

        foreach(double angle, angles)
        {
            sins << sin(angle * M_PI / 180);
            coss << cos(angle * M_PI / 180);
        }

        variants.clear();
        for(int index = 0; index < coss.size(); ++index)
        {
            double cos = coss[index];
            double sin = sins[index];
            variants << (dv * sin + dbase * cos).normalized();
        }
    }

    return variants;
}

point3d ManipulatorTool::bestNormalDirection(point3d dir) const
{
    foreach(point3d d, generateDirections())
        if(d.isNormal(dir))
            return d;
    return 1;
}

std::tuple<point3d, bool> ManipulatorTool::selectDirection(QList<point3d> variants,
                                                           point3d        start,
                                                           point2d        clicked,
                                                           double         minimumSize)
{
    WorldSystem &pw = *scene_->worldSystem();

    // превращаем набор направлений
    // в набор финишных точек
    QList<point3d>         points;
    QMap<point3d, point3d> point_2_direction;
    foreach(point3d direction, variants)
    {
        // направление на плоскости
        // длина - минимальная длина линии или аналог.
        point2d direction2 = pw.toUser(direction * minimumSize);

        // направление вырождено, поэтом использовать его бессмысленно
        if(!direction2)
            continue;

        // точка на плоскости
        auto    line     = wsLine(scene_->worldSystem(), start, start + direction);
        point3d clicked3 = clicked.project_to_ray(pw.toUser(start), direction2) >> line;

        double d = clicked.distance(pw.toUser(start));
        if(d < 1)
        {
            // если удалось кликнуть точно в точке старта
            // назначаем финишную точку
            clicked3 = start + direction;
        }
        else if(clicked3 == start)
        {
            // точка клика лежит позади от направления
            continue;
        }

        points << clicked3;
        point_2_direction[clicked3] = direction;
    }

    // вариантов движения не найдено
    // ничего не пытаемся сделать
    if(points.empty())
        return std::make_tuple(point3d(), false);

    // определяем наиболее близкую к курсору точку
    qStableSort(points.begin(), points.end(), closer_to_clicked(&pw, clicked));
    point3d result_end       = points[0];
    point3d result_direction = point_2_direction[result_end];
    manipulator_->setPreferedDirection(result_direction);

    if(result_direction.isAxeParallel())
        manipulator_->setBaseDirection(result_direction);

    if(result_end.distance(start) <= minimumSize)
    {
        result_end = (result_end - start).normalized() * minimumSize + start;
        // Q_ASSERT( result_end.distance(start) => minimumSize);
    }
    return std::make_tuple(result_end, true);
}

bool ManipulatorTool::joinToPrevious() const
{
    return joinToPrevious_;
}

bool ManipulatorTool::abortPrevious() const
{
    return abortPrevious_;
}

void ManipulatorTool::setJoinToPrevious(bool s)
{
    joinToPrevious_ = s;
}

void ManipulatorTool::setCursor(QString s)
{
    cursorName_ = s;
}

} // namespace geometry
