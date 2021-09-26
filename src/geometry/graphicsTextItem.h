#ifndef GRAPHICSTEXTITEM_H
#define GRAPHICSTEXTITEM_H

#include "textInfo.h"

#include <QGraphicsTextItem>

namespace geometry
{

class GraphicsTextItem : public QGraphicsTextItem
{
    Q_OBJECT

    TextInfo info_;

public:
    GraphicsTextItem(TextInfo info);

    void setTextInfo(TextInfo info, bool force = false);
signals:

public slots:

protected:
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);

private:
    void checkForSpecialChars();
};

} // namespace geometry

#endif // GRAPHICSTEXTITEM_H
