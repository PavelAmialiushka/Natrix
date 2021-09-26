#ifndef TOOLINFO_H
#define TOOLINFO_H

#include "baseObject.h"
#include <QSharedPointer>

namespace geometry
{

class Manipulator;

MAKESMART(ToolInfo);
MAKESMART(ManipulatorTool);

///////////////////////////////////////////////////////////////////////////////

class ToolInfo
{
public:
    template <class T>
    static PToolInfo create(QString rn, QString n, QString seq, T *pointer)
    {
        PToolInfo px(pointer);
        px->name     = n;
        px->regName  = rn;
        px->shortCut = seq;
        return px;
    }

public:
    QString regName;
    QString name;
    QString shortCut;

public:
    virtual ~ToolInfo();
    virtual PManipulatorTool createTool(Manipulator *) = 0;
};

///////////////////////////////////////////////////////////////////////////////

struct ToolSet
{
    QString          name;
    QList<PToolInfo> tools;
};

class ToolSetFactory
{
    QList<ToolSet> sets_;

    ToolSetFactory();

public:
    static ToolSetFactory &inst();

    QList<ToolSet> sets();
    PToolInfo      searchTool(Manipulator *manip, PManipulatorTool tool);

    PToolInfo toolByName(QString);
    PToolInfo toolSelect();
    PToolInfo toolLine();
    PToolInfo toolErase();
    PToolInfo toolMove();
    PToolInfo toolElement();
};

} // namespace geometry

#endif // TOOLINFO_H
