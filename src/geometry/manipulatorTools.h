#ifndef MANIPULATORTOOLS_H
#define MANIPULATORTOOLS_H

#include "command.h"
#include "scene.h"

#include <QSet>
#include <QString>
#include <QStringList>
#include <tuple>

#include "scenePropertyValue.h"

namespace geometry
{

class ManipulatorTool;
typedef QSharedPointer<ManipulatorTool> PManipulatorTool;

enum ManipulatorToolMode
{
    Dragging    = 1, // draggin
    Clicking    = 2, // pure clicking
    DragToClick = 4  // dragging converts to [pair of clicks]
};

class ManipulatorTool : public QObject
{
    Q_OBJECT

protected:
    static QString helperTab;

public:
    Manipulator *manipulator_;
    Scene *      scene_;

    // набор команд еще не подтвержденных
    // нажатием мышки
    UndoList commandList_;

    // следующиий инструмент
    PManipulatorTool nextTool_;

    bool    joinToPrevious_;
    bool    abortPrevious_;
    int     supportModes_;
    QString cursorName_;
    bool    fastRedrawPossible_;

    bool preserveSelection_;

    ManipulatorTool(Manipulator *);
    ~ManipulatorTool();

    bool support(int) const;

    // вызывается во время установки инструмента в качестве текущего
    void setUp();

    // вызывается перед тем, как инструмент отключается от манипулятора
    void tearDown();

    // возвращает инструмент к нулевому состоянию, отменяя все действия
    // используется при cancel, moveout, setTool, undo/redo
    void rollback();

    // вызывается перед любым действием
    void rollbackAndPrepare();

    // передвижение мышки
    void move(point2d pt);
    // клик мышки
    void click(point2d pt);
    // двойной клик мыши
    void doubleClick(point2d pt);

    // в течение перетаскивания
    void drag(point2d pt, bool started = 0);
    // после окончания перетаскиваня
    void drop(point2d pt);

    // принять и обработать комманды
    void commit();

    void changeRotation(int);
    void changeSize(int sizeF, bool absolute);
    void toggleMode();

    bool isFastRedrawPossible() const;

    void takeToolProperty(SceneProperties &);
    bool updateToolProperties(ScenePropertyValue);

protected:
    virtual void do_setUp();
    virtual void do_tearDown();

    virtual void do_rollback();                     // cancel pressed
    virtual void do_prepare();                      // подготовка к выполнению
    virtual void do_move(point2d, PNeighbourhood);  // move without click
    virtual void do_click(point2d, PNeighbourhood); // click
    virtual void do_doubleClick(point2d, PNeighbourhood); // double click

    virtual void do_drag(point2d, PNeighbourhood, bool started = 0); // press-and-move
    virtual void do_drop(point2d, PNeighbourhood);                   // drop

    // действие подтверждено пользователем
    virtual void do_commit(); // commit

    // увеличение/уменьшение размера
    virtual void do_changeRotation(int d);
    virtual void do_changeSize(int sizeF, bool absolute);

    // переключение режима
    virtual void do_toggleMode();

    virtual void do_takeToolProperty(SceneProperties &);
    virtual bool do_updateToolProperties(ScenePropertyValue);

public:
    virtual QString helperText() const;

    UndoList takeCommandList();
    bool     joinToPrevious() const;
    bool     abortPrevious() const;

    void             clearNextTool();
    PManipulatorTool nextTool() const;

    void setJoinToPrevious(bool s);

    void setCursor(QString);

    // помощь в выборе направления
    QList<point3d>            generateDirections() const;
    point3d                   bestNormalDirection(point3d dir) const;
    std::tuple<point3d, bool> selectDirection(QList<point3d> variants,
                                              point3d        start,
                                              point2d        clicked,
                                              double         minimumSize);
};

//////

class ManipulatorLogger
{
    Manipulator * manip;
    Scene *       scene;
    QStringList   result;
    QSet<PObject> snapshot;

public:
    ManipulatorLogger(Manipulator *manip, Scene *scene);
    ManipulatorLogger &operator<<(QString text);
    void               makeLogRecord();
    void               reportTool(QObject *tool);
};

} // namespace geometry

#endif // MANIPULATORTOOLS_H
