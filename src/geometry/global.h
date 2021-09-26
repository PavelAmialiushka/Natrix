#ifndef GEOMETRYGLOBAL_H
#define GEOMETRYGLOBAL_H

namespace geometry
{
// objects
const int lineMinimumSize         = 16;
const int lineWidth               = 4;
const int initialCrossSpaceRadius = 10;
// elements
const int valveSize       = 40;
const int flangesWidth    = 4;
const int flangesHalfSize = 8;
// graphics
const int adornerBoxSize = 8;
// adorners
const int endCupStrokeHalfSize = 8;
const int weldStrokeHalfSize   = 4;
const int bendStrokeHalfSize   = weldStrokeHalfSize;
const int bendStrokeDistance   = 8;
const int teeStrokeHalfSize    = weldStrokeHalfSize;
const int teeStrokeDistance    = bendStrokeDistance;
const int blindCircleRadius    = 2;

const int TextNormalHeight = 13;

} // namespace geometry

#endif // GEOMETRYGLOBAL_H
