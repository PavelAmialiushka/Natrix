#ifndef GEOMETRY_MANIPULATORWELDMOVE_H
#define GEOMETRY_MANIPULATORWELDMOVE_H

#include "manipulatorTools.h"
#include "weldProcedures.h"

namespace geometry
{

MAKESMART(WeldJoiner);
MAKESMART(BendJoiner);
MAKESMART(TeeJoiner);

class WeldMove : public ManipulatorTool
{
    Q_OBJECT

    QList<WeldInfo> infos_;

public:
    explicit WeldMove(Manipulator *m, QList<WeldInfo> infos);
    static PManipulatorTool create(Manipulator *m, QList<WeldInfo> infos);

public:
    void    do_click(point2d, PNeighbourhood);
    QString helperText() const;

private:
    Command cmdRemoveWeldJoiner(PObject weldObject);

    Command moveWeld(point2d point, PNeighbourhood nei);
    void    do_prepare();
};

} // namespace geometry

#endif // GEOMETRY_MANIPULATORWELDMOVE_H
