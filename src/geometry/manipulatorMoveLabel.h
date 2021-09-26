#ifndef MANIPULATORMOVELABEL_H
#define MANIPULATORMOVELABEL_H

#include "manipulatorTools.h"

namespace geometry
{

class MoveLabel : public ManipulatorTool
{
    Q_OBJECT

    point2d start_point_;
    PLabel  label_, newbyLabel_;

    Command cmdMoveLabel(Scene *        scene,
                         PNeighbourhood nei,
                         PLabel         label,
                         point2d        pt,
                         point2d        delta,
                         bool           copyLabel,
                         bool           dropMarkers,
                         bool           dontStick);

public:
    MoveLabel(Manipulator *m, point2d, PLabel);
    static PManipulatorTool create(Manipulator *m, point2d, PLabel);

    void do_prepare();
    void do_click(point2d pt, PNeighbourhood);
    void do_drag(point2d pt, PNeighbourhood, bool started = 0);
    void do_drop(point2d pt, PNeighbourhood);
    void do_tearDown();

    QString helperText() const;
};

} // namespace geometry

#endif // MANIPULATORMOVELABEL_H
