#ifndef GEOMETRY_LAYOUTMOVE_H
#define GEOMETRY_LAYOUTMOVE_H

#include "manipulator.h"
#include <QObject>

namespace geometry
{

class CanvasMove : public ManipulatorTool
{
    point2d          start_;
    PCanvasRectangle canvas_;
    QList<PGrip>     movingGrips_;

public:
    CanvasMove(Manipulator *m, point2d pt, PCanvasRectangle label);

    static PManipulatorTool create(Manipulator *m, point2d pt, PCanvasRectangle label);

public:
    void do_setUp() override;
    void do_rollback() override;
    void do_prepare() override;
    void do_click(point2d pt, PNeighbourhood n) override;
    void do_drag(point2d pt, PNeighbourhood, bool started = false) override;
    void do_drop(point2d pt, PNeighbourhood n) override;

    QString helperText() const override;
    void    pushCanvasGrips();
};

} // namespace geometry

#endif // GEOMETRY_LAYOUTMOVE_H
