#ifndef MAKESMART_H
#define MAKESMART_H

#include <QSharedPointer>
#define MAKESMART(Object)                                                                          \
    class Object;                                                                                  \
    typedef QSharedPointer<Object> P##Object
#define MAKESMARTS(Object)                                                                         \
    struct Object;                                                                                 \
    typedef QSharedPointer<Object> P##Object

#endif // MAKESMART_H
