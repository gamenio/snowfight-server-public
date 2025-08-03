#ifndef __TYPE_LIST_H__
#define __TYPE_LIST_H__


class TypeNull;

template<typename H, typename T>
class TypeList;


#define TYPELIST_1(T1)						TypeList<T1, TypeNull>
#define TYPELIST_2(T1, T2)					TypeList<T1, TYPELIST_1(T2)>
#define TYPELIST_3(T1, T2, T3)				TypeList<T1, TYPELIST_2(T2, T3)>
#define TYPELIST_4(T1, T2, T3, T4)			TypeList<T1, TYPELIST_3(T2, T3, T4)>
#define TYPELIST_5(T1, T2, T3, T4, T5)		TypeList<T1, TYPELIST_4(T2, T3, T4, T5)>

#endif // __TYPE_LIST_H__
