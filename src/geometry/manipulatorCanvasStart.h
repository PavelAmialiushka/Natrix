#ifndef GEOMETRY_LAYOUTSTART_H
#define GEOMETRY_LAYOUTSTART_H

#include <QObject>

#include "manipulator.h"

namespace geometry
{

class CanvasStart : public ManipulatorTool
{
public:
    CanvasStart(Manipulator *m);

    static PManipulatorTool create(Manipulator *m);

public:
    void    do_setUp() override;
    void    do_prepare() override;
    void    do_rollback() override;
    void    do_click(point2d, PNeighbourhood nei) override;
    QString helperText() const override;

    void drawCanvasGrips();
};

} // namespace geometry

#endif // GEOMETRY_LAYOUTSTART_H
