#include "command.h"

#include <algorithm>

namespace geometry
{

Command::Command()
{
}

Command::Command(QString name)
    : name_(name)
{
}

QString Command::name() const
{
    return name_;
}

bool Command::empty() const
{
    return doitList_.empty();
}

Command Command::reverted() const
{
    Command result;

    result.doitList_ = undoList_;
    std::reverse(result.doitList_.begin(), result.doitList_.end());

    result.undoList_ = doitList_;
    std::reverse(result.undoList_.begin(), result.undoList_.end());

    return result;
}

Command &Command::operator<<(function<void()> const &f)
{
    doitList_ << f;
    return *this;
}

Command &Command::operator<<(Command const &other)
{
    doitList_ << other.doitList_;
    undoList_ << other.undoList_;
    return *this;
}

Command &Command::operator>>(function<void()> const &f)
{
    undoList_.prepend(f);
    return *this;
}

void Command::clear()
{
    doitList_.clear();
    doitList_.clear();
}

void Command::doit()
{
    QListIterator<function<void()>> iter(doitList_);
    while(iter.hasNext())
    {
        auto foo = iter.next();
        foo();
    }
}

void Command::undo()
{
    QListIterator<function<void()>> iter(undoList_);
    iter.toBack();
    while(iter.hasPrevious())
    {
        auto foo = iter.previous();
        foo();
    }
}

//////////////////////////////////////////////////////////////

UndoList::UndoList()
{
}

UndoList::UndoList(Command cmd)
{
    addAndExecute(cmd);
}

void UndoList::clear()
{
    undoList_.clear();
    redoList_.clear();
}

UndoList::operator void *() const
{
    return undoList_.empty() && redoList_.empty() ? 0 : (void *)this;
}

UndoList &UndoList::addAndExecute(Command cmd)
{
    // executing command
    // if its throw, it will not be added
    // to the UndoList
    cmd.doit();

    undoList_ << cmd;
    redoList_.clear();

    return *this;
}

UndoList &UndoList::append(UndoList list)
{
    // command in list already executed

    redoList_.clear();
    Q_ASSERT(!list.undoList_.empty());

    Command cmd = list.createCommand();
    undoList_ << cmd;
    return *this;
}

Command UndoList::createCommand() const
{
    Command cmd{QString::fromUtf8("[набор действий]")};
    foreach(Command subc, undoList_)
        cmd << subc;
    return cmd;
}

UndoList &UndoList::appendAndJoinToLast(UndoList list)
{
    // NB command in list already executed

    redoList_.clear();
    Q_ASSERT(!list.undoList_.empty());
    Q_ASSERT(undoList_.size());

    if(undoList_.size())
    {
        Command &cmd = undoList_.last();
        cmd << list.createCommand();
    }

    return *this;
}

void UndoList::removeLastCommand()
{
    Q_ASSERT(undoList_.size());

    if(undoList_.size())
    {
        Command cmd = undoList_.takeLast();
        cmd.undo();
    }
}

bool UndoList::canUndo() const
{
    return !undoList_.empty();
}

UndoList &UndoList::undo()
{
    Q_ASSERT(canUndo());
    redoList_ << undoList_.takeLast();
    redoList_.last().undo();

    return *this;
}

bool UndoList::canRedo() const
{
    return !redoList_.empty();
}

UndoList &UndoList::redo()
{
    Q_ASSERT(canRedo());

    undoList_ << redoList_.takeLast();
    undoList_.last().doit();

    return *this;
}

void UndoList::undoAllAndClear()
{
    undoAll();
    clear();
}

UndoList &UndoList::undoAll()
{
    while(canUndo())
        undo();
    return *this;
}

} // namespace geometry
