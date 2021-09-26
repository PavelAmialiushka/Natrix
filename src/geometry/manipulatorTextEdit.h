#ifndef MANIPULATORTEXTEDIT_H
#define MANIPULATORTEXTEDIT_H

#include "manipulatorTools.h"

#include "TextInfo.h"
#include "TextLabel.h"
#include "textGrip.h"

namespace geometry
{

class TextEdit : public ManipulatorTool, public ITextEditor
{
    Q_OBJECT

    PTextLabel label_;
    TextInfo   previousInfo_;
    TextInfo   info_;

    bool selectAll_;
    bool returnToPreviousTool_;
    bool isAboutToCommit_;

    point2d point_;

public:
    TextEdit(Manipulator *m, PLabel p, point2d point);

    static PManipulatorTool create(Manipulator *m, PLabel p);
    static PManipulatorTool createOnExistingLabel(Manipulator *m,
                                                  PLabel       p,
                                                  point2d      point,
                                                  bool         selectAll    = false,
                                                  bool         returnToPrev = false);

private:
    void do_setUp() override;
    void do_tearDown() override;
    void do_rollback() override;
    void do_prepare() override;

private:
    void do_click(point2d, PNeighbourhood) override;
    void do_move(point2d, PNeighbourhood) override;
    void do_drag(point2d pt, PNeighbourhood nei, bool started) override;
    void do_drop(point2d pt, PNeighbourhood nei) override;

private:
    bool do_updateToolProperties(ScenePropertyValue) override;
    void do_takeToolProperty(SceneProperties &prop) override;

private:
    void do_toggleMode() override;
    void do_editFinished();

    QString helperText() const override;

    // ITextEditor
    TextInfo textInfo() const override;
    point2d  clickedPoint() const override;
    void     checkChanged() override;
    void     checkFocus() override;
    void     editFinished() override;
    void     do_changeSize(int delta, bool absolute) override;
    void     changeRotation(int delta) override;
    bool     eventInItem(int x, int y) override;
};

} // namespace geometry

#endif // MANIPULATORTEXTEDIT_H
