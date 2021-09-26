#ifndef MANIPULATORMATCH_H
#define MANIPULATORMATCH_H

#include "manipulatorTools.h"
#include "neighbourhood.h"

#include <QMap>
#include <QSet>

namespace geometry
{

class MatchStyles : public ManipulatorTool
{
    Q_OBJECT
    QSet<PObjectToSelect> selObjects_;
    ScenePropertyMap      valueMap_;
    point2d               lt_, rb_;

public:
    MatchStyles(Manipulator *m);
    MatchStyles(Manipulator *m, ScenePropertyList l);

    bool shouldTakeSample();

    static PManipulatorTool create(Manipulator *m);

private:
    void makeSelection(PNeighbourhood nei);

public:
    void do_prepare();
    void do_click(point2d, PNeighbourhood);
    void do_drag(point2d, PNeighbourhood, bool started);
    void do_drop(point2d, PNeighbourhood);

    void do_changeSize(int sizeFactor, bool absolute);
    void do_takeToolProperty(SceneProperties &props);
    bool do_updateToolProperties(ScenePropertyValue v);

    void    do_commit();
    QString helperText() const;
};

} // namespace geometry

#endif // MANIPULATORMATCH_H
