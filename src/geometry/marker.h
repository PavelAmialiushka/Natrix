#ifndef MARKER_H
#define MARKER_H

#include "baseObject.h"

namespace geometry
{

MAKESMART(Object);
MAKESMART(Marker);
MAKESMART(Label);

/*
 маркер - это сущность, которая отображается на бумаге и которая
позволяет привязываться к определенным точкам объектов
 - вспомогательные линии текстовых меток
 - линии размеров
 - точки вставки опор, подвесок, проходов через перекрытие/стену/землю
 -
*/

class Marker
//        : public QEnableSharedFromThis<Marker>
{
public:
    // зафиксированная точка, положение
    // который привязано к лидеру
    point3d point;

    // лидер (объект или метка),
    // смещая который смещается и маркер
    PObjectToSelect leader;

    // последователь (один из), который ведется
    // на изменение положения лидера
    PLabel follower;

    static PMarker create();
    static PMarker create(point3d, PObjectToSelect, PLabel);
};

} // namespace geometry

#endif // MARKER_H
