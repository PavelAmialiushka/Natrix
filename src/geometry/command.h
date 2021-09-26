#ifndef COMMAND_H
#define COMMAND_H

#include <QSharedPointer>

#include <functional>
using std::bind;
using std::function;

/////////////////////////////////////////////////////////////////////////////

namespace geometry
{

class Command
{
    QList<function<void()>> doitList_;
    QList<function<void()>> undoList_;

    QString name_;

public:
    Command();
    Command(QString name);

public:
    void clear();
    void doit();
    void undo();

    QString name() const;
    bool    empty() const;

public:
    Command &operator<<(Command const &other);
    Command &operator<<(function<void()> const &f);

    Command  reverted() const;
    Command &operator>>(function<void()> const &f);
};

class UndoList
{
    QList<Command> undoList_;
    QList<Command> redoList_;

public:
    UndoList();
    UndoList(Command);

    void clear();
         operator void *() const;

    // добавляет и выполняет комманду
    UndoList &addAndExecute(Command);
    Command   createCommand() const;

    // только добавляет список
    UndoList &append(UndoList);
    UndoList &appendAndJoinToLast(UndoList);
    void      removeLastCommand();

    // отмена операций
    bool      canUndo() const;
    UndoList &undo();
    UndoList &undoAll();

    // два действия в один ход
    void undoAllAndClear();

    // отмена отмены операций
    bool      canRedo() const;
    UndoList &redo();
};

} // namespace geometry
#endif // COMMAND_H
