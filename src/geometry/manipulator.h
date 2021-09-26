#ifndef MANIPULATOR_H
#define MANIPULATOR_H

#include "command.h"
#include "scene.h"
#include "toolInfo.h"
#include "worldSystem.h"

#include "manipulatorTools.h"

#include <tuple>

#include <QSet>
#include <QString>
#include <QStringList>

namespace geometry
{

enum ManipulatorModes
{
    SelectMode,

    // all
    Normal = 0,

    // select
    Append = 1, // control
    Remove = 2, // shift + control
    Modify = 4, // shift

    ShiftKey   = 32,
    ControlKey = 64,
    AltKey     = 128,
};

class Manipulator : public QObject
{
    Q_OBJECT

    friend class ManipulatorTool;
    Scene *          scene_;
    PManipulatorTool tool_;
    PManipulatorTool prevTool_;
    PToolInfo        toolInfo_;

    // направления
    point3d preferedDirection_;
    point3d baseDirection_;

    // modifiers
    int shift_;
    int control_;
    int alt_;

    // режим для отладки
    bool simpleDebugMode_;

    // текущий режим
    int planeMode_;

    // подный набор команд
    UndoList undoList_;

    // запоминается предыдущее положение события
    // используется для воспроизведения относительных
    // комманд имеющих вид @...
    point2d lastPoint_;

    // запоминается предыдущая точка drag используется
    // для непрерывного drag`а
    point2d previousDragPosition_;

    QStringList debugLog_;

    // члены логирования
    QStringList   logList_;
    bool          logging_;
    bool          playing_;
    QSet<PObject> objectsBeforeOperation_;

public:
    Manipulator(Scene *sc);
    ~Manipulator();
    void           saveLogs();
    static QString executeFile(QString fileName);
    static QString executeFile(QByteArray data);

    void paste(QByteArray);
    void setTool(PManipulatorTool tool, bool roolbackCurrent = true, bool updateToolInfo = false);
    PManipulatorTool prevTool() const;

    void wantSetTool(PManipulatorTool tool);
    void acceptSetTool();

    QSharedPointer<ManipulatorTool> tool() const;

    // инструменты логгинга и текстовых команд
    void debugLog(QString action);

    // запись логов
    void logAppendCommandText(QVariant);
    void logRemoveCommandText(int lineCount);

    // воспроизведение логов
    QStringList preprocessCommands(QStringList list);
    QStringList takeFirstCommandBlock(QStringList &list);
    QStringList executeCommand(QString);

    // undo commands
    void undo();
    bool canUndo() const;
    void redo();
    bool canRedo() const;
    void appendCommand(UndoList const &, bool joinToLast);
    void removeLastCommand();

    void deleteSelected();
    void selectAll();
    void selectNone();

    // toolbox
    PToolInfo toolInfo() const;
    void      setToolInfo(PToolInfo);
    QString   helperText() const;

    void updatePropertiesUp();

    void takeToolProperty(SceneProperties &props);

    // здесь принимаем свойства
    void updatePropertiesDown(ScenePropertyValue v);

signals:
    void toolChanged();
    void updatePlaneMode();

public:
    // modifiers
    void clearCounters();

    void togglePlane(); // select next plane
    int  planeMode() const;
    void setPlaneMode(int mode);

    // tool commands
    void cancel();             // escape pressed
    void moveToTheSamePoint(); // repeate move

    // режимы зависящие от клавиш-модификаторов
    int                       getMode(int, int = 0) const;
    void                      setModifiers(int shift, int ctrl, int alt);
    std::tuple<int, int, int> modifiers() const;

    void moveOut();     // cursor is out of window
    void move(point2d); // cursor in window
    void click(point2d);

    void doubleClick(point2d);

    point2d previousDragPosition() const;
    void    dragStart(point2d);
    void    drag(point2d);
    void    drop(point2d);

    void changeFlanges(bool);
    void changeSize(int sizeF, bool absolute = false);

public:
    // направление
    void    setPreferedDirection(point3d);
    point3d preferedDirection() const;

    // базовое направление
    void    setBaseDirection(point3d);
    point3d baseDirection() const;

    bool simpleDebugMode() const;
    void setSimpleDebugMode(bool simpleDebugMode);
    bool updateDefaultProperties(ScenePropertyValue v);
};

} // namespace geometry
#endif // MANIPULATOR_H
