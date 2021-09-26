#ifndef UTILITES_H
#define UTILITES_H

namespace utils
{

template <class PT, class P>
QSet<PT> filtered(QSet<PT> list, P pred)
{
    QSet<PT> result;
    foreach(PT const &p, list)
        if(pred(p))
            result << p;
    return result;
}

template <class PT, class P>
QList<PT> filtered(QList<PT> list, P pred)
{
    auto not_pred = [&](PT p) -> bool { return !pred(p); };
    list.erase(std::remove_if(list.begin(), list.end(), not_pred), list.end());
    return list;
}

template <class U, class T, class P>
QList<U> filter_transform(QSet<T> list, P trans)
{
    QList<U> result;
    foreach(T v, list)
        if(auto x = trans(v))
            result << x;
    return result;
}

template <class U, class T, class P>
QList<U> filter_transform(QList<T> list, P trans)
{
    QList<U> result;
    foreach(T v, list)
        if(auto x = trans(v))
            result << x;
    return result;
}

} // namespace utils
#endif // UTILITES_H
