#ifndef PRIMITIVE_H
#define PRIMITIVE_H

#include "point.h"
#include "textInfo.h"

#include <QSharedPointer>
#include <QString>

namespace geometry
{

//////////////////////////////////////////////////////////////////

class Primitive;
typedef QSharedPointer<Primitive> PPrimitive;

//////////////////////////////////////////////////////////////////

enum PrimitiveStyle
{
    NoStyle,
    StyleLine,        // линия
    StyleText,        // текст
    StyleElement,     // задвижки и проч
    StyleDashDot,     // штрихпунктир
    StyleBlackFilled, // заливка черным цветом
    StyleViewport, // линии, изображающие границы области рисования

    // adorners
    StyleAdornerCursorLine, // line of cursor point

    //
    StyleStraightLineAdorner, // вспомогательные линии
    StyleMoveLineAdorner,
    StyleGhostAdorner,

    // nodepointadorners
    StyleNodePointAdorner = 100, // здесь должно хватить номеров, чтобы
                                 // вместились все стили NodePointAdorner

    // not to setup
    PrivateStyleHover = 500,
    PrivateStyleNewby,
    PrivateStyleTextHover
};

} // namespace geometry

#endif // PRIMITIVE_H
