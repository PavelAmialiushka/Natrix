#ifndef MANIPULATORMOVECONTINUE_H
#define MANIPULATORMOVECONTINUE_H

#include "manipulatorTools.h"
#include "moveProcedures.h"

namespace geometry
{

class MoveContinue : public ManipulatorTool
{
    Q_OBJECT

    MoveDataInit moveParams_;

public:
    MoveContinue(Manipulator *m, MoveDataInit data);

    static PManipulatorTool create(Manipulator *m, MoveDataInit data);

public:
    void do_prepare() override;
    void do_tearDown() override;
    void do_click(point2d, PNeighbourhood) override;

    void    do_commit() override;
    QString helperText() const override;

    void do_changeSize(int sizeFactor, bool absolute) override;
    void do_takeToolProperty(SceneProperties &) override;
    bool do_updateToolProperties(ScenePropertyValue) override;

private:
    void simpleMove(point2d, PNeighbourhood);
    void modifyMove(point2d, PNeighbourhood);
};

} // namespace geometry

#endif // MANIPULATORMOVECONTINUE_H
