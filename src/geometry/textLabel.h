#ifndef TEXTLABEL_H
#define TEXTLABEL_H

#include "label.h"
#include "marker.h"

#include <QRectF>
#include <QString>

class QDomElement;

namespace geometry
{

class Scene;
class GraphicsTextItem;

class TextLabel;

inline auto isText = [](PLabel a) { return a.dynamicCast<TextLabel>(); };
inline auto toText = [](PLabel a) { return a.dynamicCast<TextLabel>(); };

struct TextLayout
{
    PObject leaderElement;
    point3d leaderPoint;
    QPointF stickyDelta;

    bool stickyTextMode;
};

///////////////////////////////////////////////////////////////////

class TextLabel : public Label
{
    TextInfo                       info_;
    QWeakPointer<GraphicsTextItem> item_;
    double                         height_, width_;

public:
    TextLabel(Scene *, TextInfo info);
    ~TextLabel();
    static PLabel create(Scene *, point3d point, TextInfo info);
    static PLabel create(Scene *, TextInfo info);

    // serialization
    static PLabel createFrom(Scene *, class QDomElement);
    virtual void  saveLabel(QDomElement);
    virtual bool  loadLabel(QDomElement);

    virtual void   draw(GraphicsScene *gscene, GItems &gitems, int level);
    virtual PLabel clone(Scene *, point2d delta) const;

public:
    void     updateText(QString);
    TextInfo info() const;
    void     setInfo(TextInfo);

    bool              applyStyle(ScenePropertyValue);
    ScenePropertyList getStyles();

    // выполняется во время редактирования
    void selectAll();

    QSharedPointer<GraphicsTextItem> item() const;

private:
    QList<point2d> getCornerPoints();
    void           updateStickyMode(QList<PMarker> markers, struct TextLayout &self) const;
    void           setActualSize(double w, double h);

public:
    virtual double distance(const point2d &pt);
    virtual PLabel cloneMoveRotate(Scene * scene,
                                   point3d delta,
                                   point3d center,
                                   double  angle,
                                   MoveRotateContext &) const;
};

typedef QSharedPointer<TextLabel> PTextLabel;
void attachTextLabelTo(Scene *scene, PTextLabel text, point3d point, point3d direction);

} // namespace geometry
#endif // TEXTLABEL_H
