#ifndef MAIPULATORTEXTSTART_H
#define MAIPULATORTEXTSTART_H

#include "manipulator.h"
#include "neighbourhood.h"

#include "textGrip.h"

namespace geometry
{

class TextStart : public ManipulatorTool
{
    Q_OBJECT

public:
    TextStart(Manipulator *m);
    static PManipulatorTool create(Manipulator *m);

public:
    void do_setUp() override;
    void do_prepare() override;
    void do_rollback() override;
    void do_move(point2d, PNeighbourhood) override;
    void do_click(point2d, PNeighbourhood) override;

    void moveGrip(point2d pt, PNeighbourhood nei);
    void do_drop(point2d pt, PNeighbourhood nei) override;
    void do_drag(point2d pt, PNeighbourhood, bool started = 0) override;

    QString helperText() const override;
};

} // namespace geometry

#endif // MAIPULATORTEXTSTART_H
