#include "manipulator.h"
#include "manipulatorElementStart.h"
#include "manipulatorLineContinue.h"
#include "manipulatorLineStart.h"
#include "manipulatorMatch.h"
#include "manipulatorPasteTool.h"
#include "manipulatorSelector.h"
#include "manipulatorTools.h"

#include "toolInfo.h"

#include "canvasRectangle.h"

#include "sceneProcedures.h"

#include "bendJoiner.h"
#include "element.h"
#include "line.h"
#include "teeJoiner.h"
#include "textLabel.h"

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QVariant>

// this pargma works only as global
#ifndef _MSC_VER
#pragma GCC diagnostic ignored "-Wunused-function"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#endif

#include "cryptopp/files.h"
#include "cryptopp/gzip.h"

#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif

namespace geometry
{

bool Manipulator::simpleDebugMode() const
{
    return simpleDebugMode_;
}

void Manipulator::setSimpleDebugMode(bool simpleDebugMode)
{
    simpleDebugMode_ = simpleDebugMode;
}
Manipulator::Manipulator(Scene *scene)
    : scene_(scene)
    , tool_(QSharedPointer<ManipulatorTool>(new LineStart(this, scene->defaultLineInfo())))
{
    logging_ = true;
    playing_ = false;
    shift_ = control_ = alt_ = 0;
    clearCounters();
}

void Manipulator::debugLog(QString action)
{
    QString report = QString("%1 0x%2 %3")
                         .arg(action.leftJustified(14))
                         .arg(reinterpret_cast<uint64_t>(tool_.data()), 8, 16)
                         .arg(QString(tool_->metaObject()->className()).section("::", 1));
    debugLog_.push_front(report);
}

Manipulator::~Manipulator()
{
}

PManipulatorTool Manipulator::prevTool() const
{
    return prevTool_;
}

void Manipulator::setTool(QSharedPointer<ManipulatorTool> tool,
                          bool                            roolbackCurrent,
                          bool                            updateToolInfo)
{
    if(updateToolInfo)
    {
        PToolInfo info = ToolSetFactory::inst().searchTool(this, tool);
        if(info)
            toolInfo_ = info;

        // а теперь устанавливаем именно свой
    }

    debugLog("setting tool");

    Q_ASSERT(tool);

    // отменяем действия комманд инструмента
    if(roolbackCurrent)
    {
        // prepare не нужен, поскольку старый инструмент
        // не будет уже запущен
        tool_->rollback();
    }

    // гасим инструмент
    if(tool_)
    {
        tool_->tearDown();
    }

    // сбрасываем все временные объекты
    scene_->clearAdorners();
    scene_->clearGrips();
    clearCounters();

    // устанаваливаем новый инструмент
    prevTool_ = tool_;
    tool_     = tool;
    tool_->setUp();

    // оповещаем об изменениях
    emit toolChanged();

#ifndef NDEBUG
    if(!simpleDebugMode_)
#endif
        moveToTheSamePoint();

    updatePropertiesUp();
}

void Manipulator::saveLogs()
{
    QStringList argv   = QCoreApplication::instance()->arguments();
    QString     marker = "-log";
    int         index  = argv.indexOf(marker);
    if(index >= 0 && index + 1 < argv.size())
    {
        QString    oFileName = argv.at(index + 1);
        QByteArray data      = (QString("#%1\n").arg(oFileName) + logList_.join("\n")).toUtf8();

        QString check = executeFile(data);

        // gzip data
        std::string gzipData;
        CryptoPP::StringSource(
            data.data(), data.size(), new CryptoPP::Gzip(new CryptoPP::StringSink(gzipData)));

        QFile file(oFileName);
        QFile fet(oFileName + ".error.txt");
        QFile fe(oFileName + ".error");

        if(!check.isEmpty())
        {
            file.remove();

            if(fe.open(QFile::WriteOnly | QFile::Truncate))
            {
                fe.write(data);
                fe.close();
            }

            if(fet.open(QFile::WriteOnly | QFile::Truncate))
            {
                QTextStream(&fet) << "abort saving log file. due to auto check fail:\n" << check;
                fet.close();
            }
        }
        else
        {
            // удаляем инфу об ошибках
            fe.remove();
            fet.remove();

            // saving to file
            if(file.open(QFile::ReadWrite | QFile::Truncate))
            {
                file.write(gzipData.c_str(), gzipData.size());
                qWarning() << "log saved to" << oFileName;
            }
            else
            {
                qWarning() << "error opening log file:" << oFileName;
            }
            file.close();
        }
    }
}

void Manipulator::clearCounters()
{
    planeMode_ = 0;
}

void Manipulator::togglePlane()
{
    logAppendCommandText("\ntab");

    tool_->toggleMode();

    do
    {
        ++planeMode_;
        planeMode_ %= 4;
    } while(!planeMakeSense(scene_->worldSystem(), planeMode_));

    emit updatePlaneMode();
    moveToTheSamePoint();
}

void Manipulator::setPlaneMode(int mode)
{
    planeMode_ = mode;

    emit updatePlaneMode();
    moveToTheSamePoint();
}

int Manipulator::planeMode() const
{
    return planeMode_;
}

QSharedPointer<ManipulatorTool> Manipulator::tool() const
{
    return tool_;
}

void Manipulator::undo()
{
    debugLog("undo");

    ManipulatorLogger logger(this, scene_);
    logger << "undo";

    cancel();
    if(undoList_.canUndo())
        undoList_.undo();

    moveToTheSamePoint();

    logger.makeLogRecord();
}

void Manipulator::redo()
{
    debugLog("redo");

    ManipulatorLogger logger(this, scene_);
    logger << "redo";

    cancel();
    if(undoList_.canRedo())
        undoList_.redo();
    moveToTheSamePoint();

    logger.makeLogRecord();
}

void Manipulator::deleteSelected()
{
    // получаем список выделенных объектов
    auto selObjects = scene_->selectedObjects();

    // отменяем их выделение
    scene_->setHoverObjects(selObjects, NoHover, Selected);

    // удаляем
    Command cmd = cmdEraseObjectsToSelect(scene_, selObjects);
    cmd << cmdModify(scene_);
    appendCommand(cmd, false);
}

void Manipulator::selectAll()
{
    QSet<PObjectToSelect> objects;
    foreach(auto po, scene_->objects())
        objects << po;
    foreach(auto po, scene_->labels())
    {
        if(!po.dynamicCast<CanvasRectangle>())
        {
            objects << po;
        }
    }

    scene_->setHoverObjects(objects, Selected, Selected);
    scene_->updatePropertiesAfterSceneChange();
}

void Manipulator::selectNone()
{
    // получаем список выделенных объектов
    auto selObjects = scene_->selectedObjects();

    // отменяем их выделение
    scene_->setHoverObjects(selObjects, NoHover, Selected);
    scene_->updatePropertiesAfterSceneChange();
}

bool Manipulator::canUndo() const
{
    return undoList_.canUndo();
}

bool Manipulator::canRedo() const
{
    return undoList_.canRedo();
}

void Manipulator::appendCommand(UndoList const &commandList, bool joinToLast)
{
    if(joinToLast)
    {
        undoList_.appendAndJoinToLast(commandList);
    }
    else
    {
        undoList_.append(commandList);
    }
}

void Manipulator::removeLastCommand()
{
    undoList_.removeLastCommand();
}

// реакция на Escape
void Manipulator::cancel()
{
    auto doNotDestroyIt = tool_;
    logAppendCommandText("\ncancel");
    debugLog("cancel");

    // отменяем и перезапускаем текущий элемент
    tool_->rollbackAndPrepare();

    scene_->clearSelection();
    scene_->updatePropertiesAfterSceneChange();

    // устанавливаем новый инструмент
    if(tool_->nextTool())
    {
        setTool(tool_->nextTool(), false);
    }
}

void Manipulator::moveOut()
{
    auto doNotDestroyIt = tool_;
    debugLog("moveout");

    // отменяем все действия комманды
    // prepare не нужн, поскольку комманда временно не запускается
    // (нужен, чтобы рисовать grips
    tool_->rollbackAndPrepare();

    scene_->setCursorPoint();
}

void Manipulator::moveToTheSamePoint()
{
    debugLog("moveSamePoint");
    // в т.ч. и визуально
    // не ведем лог, поскольку это событие "внутреннее"
    bool log = false;
    qSwap(log, logging_);
    {
        if(!scene_->noCursorPoint())
        {
            move(scene_->cursorPoint());
        }
        else
        {
            moveOut();
        }
    }
    qSwap(log, logging_);
}

void Manipulator::setModifiers(int shift, int control, int alt)
{
    shift_   = shift;
    control_ = control;
    alt_     = alt;
}

std::tuple<int, int, int> Manipulator::modifiers() const
{
    return std::make_tuple(shift_, control_, alt_);
}

int Manipulator::getMode(int n, int m) const
{
    if((n & (ShiftKey | ControlKey | AltKey)) != 0)
    {
        int curr = (shift_ ? ShiftKey : 0) | (alt_ ? AltKey : 0) | (control_ ? ControlKey : 0);
        return n == curr || (m ? m == curr : false);
    }
    if(n == SelectMode)
    {
        if(shift_ && control_)
            return Remove;
        if(control_)
            return Append;
        if(shift_)
            return Modify;
        return Normal;
    }
    return Normal;
}

point2d Manipulator::previousDragPosition() const
{
    return previousDragPosition_;
}

void Manipulator::dragStart(point2d pt)
{
    debugLog("Manipulator: dragStart");

    auto doNotDestroyIt = tool_;
    if(tool_->support(Dragging))
    {
        // перед первым смещением устанавливаем
        // предыдущую точку до вызова метода drag
        // инструмента
        previousDragPosition_ = pt;

        tool_->drag(pt, true);
    }
    else
    {
        // клик уже был, значит текуще событие
        // рассматривается просто как перемещение мышки
        debugLog("(drag)");
        tool_->click(pt);
    }
}

void Manipulator::drag(point2d pt)
{
    debugLog("Manipulator: drag");

    auto doNotDestroyIt = tool_;
    if(tool_->support(Dragging))
    {
        tool_->drag(pt);

        // запоминаем последнюю точку
        previousDragPosition_ = pt;
    }
    else
    {
        debugLog("(drag)");
        move(pt);
    }
}

void Manipulator::drop(point2d pt)
{
    debugLog("Manipulator: drop");

    auto doNotDestroyIt = tool_;
    if(tool_->support(Dragging))
    {
        tool_->drop(pt);
    }
    else if(tool_->support(DragToClick))
    {
        debugLog("(drop)");
        tool_->click(pt);

        // дроп логически законченная операция
        // поэтому после окончания завершаем действие
        cancel();
    }
    else
    {
        debugLog("(drop)");
        tool_->move(pt);
    }
}

void Manipulator::changeFlanges(bool f)
{
    tool_->changeRotation(f);
    moveToTheSamePoint();
}

void Manipulator::changeSize(int sizeF, bool absolute)
{
    tool_->changeSize(sizeF, absolute);
    moveToTheSamePoint();
}

void Manipulator::click(point2d pt)
{
    debugLog("Manipulator: click");

    auto doNotDestroyIt = tool_;
    if(tool_->support(Clicking))
    {
        tool_->click(pt);
    }
    else
    {
        tool_->drop(pt);
    }
}

void Manipulator::doubleClick(point2d pt)
{
    debugLog("Manipulator: doubleClick");

    auto doNotDestroyIt = tool_;
    if(tool_->support(Clicking))
    {
        tool_->doubleClick(pt);
    }
    else
    {
        tool_->drop(pt);
    }
}

void Manipulator::move(point2d pt)
{
    debugLog("Manipulator: move");

    auto doNotDestroyIt = tool_;
    if(tool_->support(Clicking))
    {
        tool_->move(pt);
    }
    else
    {
        tool_->drag(pt);
        previousDragPosition_ = pt;
    }
}

void Manipulator::setPreferedDirection(point3d p)
{
    preferedDirection_ = p;
}

point3d Manipulator::preferedDirection() const
{
    return preferedDirection_;
}

void Manipulator::setBaseDirection(point3d p)
{
    baseDirection_ = p;
}

point3d Manipulator::baseDirection() const
{
    return baseDirection_;
}

PToolInfo Manipulator::toolInfo() const
{
    return toolInfo_;
}

void Manipulator::setToolInfo(PToolInfo info)
{
    if(info == toolInfo_)
        return;
    toolInfo_ = info;
    setTool(toolInfo_->createTool(this));
}

QString Manipulator::helperText() const
{
    return tool_->helperText();
}

void Manipulator::updatePropertiesUp()
{
    scene_->updatePropertiesAfterSceneChange();
}

void Manipulator::takeToolProperty(SceneProperties &props)
{
    tool_->takeToolProperty(props);
}

bool Manipulator::updateDefaultProperties(ScenePropertyValue v)
{
    bool count = false;
    switch(v.type)
    {
    case ScenePropertyType::TextSize:
    case ScenePropertyType::TextRotation:
    case ScenePropertyType::TextDecoration: {
        TextInfo textInfo = scene_->defaultTextInfo();
        if(textInfo.applyStyle(scene_, v))
        {
            count = true;
            scene_->setDefaultTextInfo(textInfo);
        }

        break;
    }
    case ScenePropertyType::ValveSize:
    case ScenePropertyType::ValveFlanges: {
        ElementInfo info = scene_->defaultElementInfo();
        if(info.applyStyle(v))
        {
            count = true;
            scene_->setDefaultElementInfo(info);
        }
        break;
    }
    case ScenePropertyType::LineStyle:
    case ScenePropertyType::BendStyle:
    case ScenePropertyType::TeeStyle:
    case ScenePropertyType::EndStyle: {
        LineInfo lineInfo = scene_->defaultLineInfo();
        if(lineInfo.apply(v))
        {
            count = true;
            scene_->setDefaultLineInfo(lineInfo);
        }
        break;
    }
    }
    return count;
}

void Manipulator::updatePropertiesDown(ScenePropertyValue v)
{
    Command cmd;

    int counter = 0;

    // пытаемся использовать свойства для модификации выделенных объектов
    auto selection = scene_->selectedObjects();
    cmd << cmdMatchStyles(scene_, selection, ScenePropertyList() << v);
    if(!cmd.empty())
        counter += 1;

    // пытаемся обновить свойства текущего инструмента
    int tool_counter = tool_->updateToolProperties(v);

    // для инструмента MatchStyle нет "встроенных" элементов
    if(!tool_.dynamicCast<MatchStyles>())
        counter += tool_counter;

    if(counter == 0)
    {
        // пытаемся обновить свойства по умолчанию
        bool propChanged = updateDefaultProperties(v);

        if(!propChanged)
        { // если они не изменились,
            // то устанавливаем инструмент MatchStyles

            SceneProperties props;

            // если уже этот инструмент, то копируем его свойства
            if(tool_.dynamicCast<MatchStyles>())
                takeToolProperty(props);

            ScenePropertyList list = props.getStyles() << v;
            setTool(PManipulatorTool(new MatchStyles(this, list)),
                    /*rollbackCurrent=*/true,
                    /*updateToolInfo=*/true);
        }
    }

    cmd << cmdModify(scene_);
    appendCommand(cmd, false);

    scene_->updatePropertiesAfterSceneChange();
}

static bool readPoint(QString string, point2d last_point, point2d *result)
{
    Q_ASSERT(result);

    bool relative = false;
    if(string.startsWith("@"))
    {
        string.remove(0, 1);
        relative = true;
    }

    bool success;
    *result = point2d::serialLoad(string, &success);
    if(relative)
        *result += last_point;
    return success;
}

void Manipulator::logAppendCommandText(QVariant vText)
{
    if(logging_)
        logList_ << vText.toStringList();
}

void Manipulator::logRemoveCommandText(int lineCount)
{
    Q_ASSERT(logList_.size() >= lineCount);

    if(logging_)
    {
        while(lineCount-- > 0)
            logList_.takeLast();
    }
}

QStringList Manipulator::preprocessCommands(QStringList list)
{
    int         lineno = 0;
    QStringList result;
    foreach(QString line, list)
    {
        ++lineno;

        // comments
        if(line.startsWith("#") || line.isEmpty())
            continue;

        // results
        if(line.startsWith("-") || line.startsWith("+"))
            result << line;
        else
        {
            // command
            result << QString("lineno %1").arg(lineno);
            result << line;
        }
    }
    return result;
}

QStringList Manipulator::takeFirstCommandBlock(QStringList &list)
{
    QStringList result;

    while(1)
    {
        if(list.isEmpty())
            break;
        QString line = list.takeFirst();

        if(line.startsWith("lineno"))
        {
            if(!result.isEmpty())
            {
                list.push_front(line);
                break;
            }
        }
        result << line;
    }

    return result;
}

static QStringList plusminusOnly(QStringList list)
{
    QStringList result;
    foreach(QString s, list)
    {
        s = s.trimmed();
        if(s.startsWith("+") || s.startsWith("-"))
            result << s;
        else if(s.startsWith('#'))
            continue;
    }
    return result;
}

static QString extractToken(QString &input)
{
    QString result;

    std::function<bool(QChar)> cp, start, point, command;
    start = [&](QChar c) {
        if(' ' == c)
            return true;

        if(point(c))
        {
            cp = point;
        }
        else if(command(c))
        {
            cp = command;
        }
        return true;
    };
    point = [&](QChar c) {
        static QString sample = ".,@-+";
        if(sample.indexOf(c) == -1 && !c.isDigit())
        {
            return false;
        }
        result += c;
        return true;
    };
    command = [&](QChar c) {
        if(!c.isLetter())
        {
            return false;
        }
        result += c;
        return true;
    };
    cp = start;

    int index = 0;
    for(; index < input.size(); ++index)
    {
        QChar c = input[index];

        if(!cp(c))
        {
            input.remove(0, index);
            return result;
        }
    }
    cp(' ');
    input.clear();

    return result;
}

QStringList Manipulator::executeCommand(QString command)
{
    QStringList temp;
    qSwap(temp, logList_);
    bool sPlaying = true;
    qSwap(playing_, sPlaying);

    command = command.trimmed();

    bool moveOnly = false;

    QStringList tokens;
    while(command.size())
        tokens << extractToken(command);

    int local_alt     = 0;
    int local_shift   = 0;
    int local_control = 0;
    while(tokens.size())
    {
        QString cmd = tokens.takeFirst();

        if(cmd == "cancel" || cmd == ".")
        {
            cancel();
        }
        else if(cmd == "undo")
        {
            undo();
        }
        else if(cmd == "redo")
        {
            redo();
        }
        else if(cmd == "tab")
        {
            togglePlane();
        }
        else if(cmd == "move")
        {
            moveOnly = true;
        }
        else if(cmd == "ctrl")
        {
            local_control = 1;
        }
        else if(cmd == "shift")
        {
            local_shift = 1;
        }
        else if(cmd == "alt")
        {
            local_alt = 1;
        }
        else if(cmd == "modifiers")
        {
            QString arg = tokens.size() ? tokens.takeFirst() : "0,0,0";

            point3d point;
            bool    ok;
            point = point.serialLoad(arg, &ok);
            if(ok)
            {
                int shift   = point.x != 0;
                int control = point.y != 0;
                int alt     = point.z != 0;

                this->setModifiers(shift, control, alt);
            }
        }
        else if(auto tool = ToolSetFactory::inst().toolByName(cmd))
        {
            setToolInfo(tool);
        }
        else
        {
            point2d point;
            if(readPoint(cmd, lastPoint_, &point))
            {
                if(local_alt || local_control || local_shift)
                {
                    move(point);

                    alt_     = local_alt;
                    shift_   = local_shift;
                    control_ = local_control;

                    click(point);

                    local_alt = local_control = local_shift = 0;
                }
                else if(moveOnly)
                {
                    move(point);
                    moveOnly = false;
                }
                else
                    click(point);

                lastPoint_ = point;
            }
            else
                qDebug() << "incorrect point" << cmd;

            // reset modifiers
            this->setModifiers(0, 0, 0);
        }

        scene_->recalcDeepCaches();
        auto d = scene_->dumpContents();
    }

    qSwap(temp, logList_);
    qSwap(playing_, sPlaying);

    return plusminusOnly(temp);
}

QString Manipulator::executeFile(QByteArray a)
{
    QScopedPointer<Scene> scene(new Scene(1));
    Manipulator *         manip = scene->manipulator();

    // do not move this line
    // s should store data until the end of function
    std::string s;
    if(a[0] != '#')
    {
        // gunzip
        CryptoPP::StringSource(reinterpret_cast<const unsigned char *>(a.constData()),
                               a.size(),
                               true,
                               new CryptoPP::Gunzip(new CryptoPP::StringSink(s)));
        // QByteArray does not copy data
        // it holds pointer to s.c_str()
        a.setRawData(s.c_str(), static_cast<uint>(s.size()));
    }

    QStringList lines    = QString::fromUtf8(a).split('\n');
    QString     fileName = lines.at(0).trimmed().mid(1);
    lines                = manip->preprocessCommands(lines);

    while(!lines.isEmpty())
    {
        QStringList commandBlock = manip->takeFirstCommandBlock(lines);
        Q_ASSERT(commandBlock.size());
        int lineNo = commandBlock.takeFirst().split(" ")[1].toInt();

        QString     command = commandBlock.takeFirst();
        QStringList result  = manip->executeCommand(command);

        auto expected = QSet<QString>(std::begin(commandBlock), std::end(commandBlock));
        auto actual   = QSet<QString>(std::begin(result), std::end(result));

        QStringList a = (actual - expected).toList(), e = (expected - actual).toList(),
                    s = (actual & expected).toList();

        a.sort();
        e.sort();
        s.sort();

        QStringList message;
        message << QString("%1:%2: regression command failed\n%1:%2: %3")
                       .arg(fileName)
                       .arg(lineNo)
                       .arg(command)
                << QString("     [ Actual and expected (%1) ]").arg(s.size()) << s
                << QString("     [ Actual but NOT EXPRECTED (%1) ]").arg(a.size()) << a
                << QString("     [ Expected but NOT PRESENT (%1) ]").arg(e.size()) << e;

        if(a.size() || e.size())
        {
            return message.join("\n");
        }
    }

    return QString();
}

void Manipulator::paste(QByteArray data)
{
    auto tool = PasteTool::create(this, tool_, data);
    if(tool)
    {
        setTool(tool);
        scene_->removeSelectedObjects(scene_->selectedObjects());
        scene_->updatePropertiesAfterSceneChange();
    }
}

QString Manipulator::executeFile(QString fileName)
{
    QFile file(fileName);
    if(!file.open(QFile::ReadOnly))
        return "cannot open file: " + fileName;

    QByteArray a = file.readAll();
    return executeFile(a);
}

} // namespace geometry
