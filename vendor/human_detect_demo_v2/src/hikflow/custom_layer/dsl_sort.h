#include <stdint.h>
#include <stdlib.h>

#ifdef DSL_SORT_SUFFIX
#ifdef DSL_SORT_VALUE_TYPE
#ifdef DSL_SORT_COMP_PTR_FUNC

#define DISTANCE_TYPE size_t
// #include <algorithm>
// #define DSL_SORT_MOVE(X) std::move(X)
#define DSL_SORT_MOVE(X) X

#define DSL_SORT_GETNAME_IMPL2(name, type, suffix) _dsl_ ##name ##_ ##type ##_ ##suffix
#define DSL_SORT_GETNAME_IMPL(name, type, suffix) DSL_SORT_GETNAME_IMPL2(name, type, suffix)
#define DSL_SORT_GETNAME(name)                                         \
    DSL_SORT_GETNAME_IMPL(name, DSL_SORT_VALUE_TYPE, DSL_SORT_SUFFIX)

#ifndef __DSL_SORT_COMMON_H__
#define __DSL_SORT_COMMON_H__

#if defined(__GNUC__) || defined(__clang__)
    //   stl_algobase.h  std::__lg
    // __builtin_clzl  unsigned long 0
    // size_t
    // gcc unsigned long 
    static inline unsigned long __dsl_lg(unsigned long __n)
    { return sizeof(unsigned long) * sizeof(char) * 8  - 1 - __builtin_clzl(__n); }
#else
    #error "Unsupport Platform"
#endif

#endif


#define DSL_S_threshold 16 

#define __dsl_iter_swap(__a, __b)           \
{                                           \
    DSL_SORT_VALUE_TYPE tmp = *__a; \
    *__a = *__b;                            \
    *__b = tmp;                             \
}

#define DSL_MOVE_BACKWARD(__first, __last, __result)\
    while ((__first) != (__last))   *(--(__result)) = *(--(__last));


static inline void DSL_SORT_GETNAME(push_heap)(
    DSL_SORT_VALUE_TYPE *__first, DISTANCE_TYPE __holeIndex,
    DISTANCE_TYPE __topIndex, DSL_SORT_VALUE_TYPE __value)
{
    DISTANCE_TYPE __parent = (__holeIndex - 1) / 2;
    while (__holeIndex > __topIndex &&
           DSL_SORT_COMP_PTR_FUNC((__first + __parent), (&__value)))
    {
        *(__first + __holeIndex) = DSL_SORT_MOVE(*(__first + __parent));
        __holeIndex = __parent;
        __parent = (__holeIndex - 1) / 2;
    }
    *(__first + __holeIndex) = DSL_SORT_MOVE(__value);
}

static inline void DSL_SORT_GETNAME(adjust_heap)(
    DSL_SORT_VALUE_TYPE *__first, DISTANCE_TYPE __holeIndex,
    DISTANCE_TYPE __len, DSL_SORT_VALUE_TYPE __value)
{
    const DISTANCE_TYPE __topIndex = __holeIndex;
    DISTANCE_TYPE __secondChild = __holeIndex;
    while (__secondChild < (__len - 1) / 2)
    {
        __secondChild = 2 * (__secondChild + 1);
        if (DSL_SORT_COMP_PTR_FUNC((__first + __secondChild),
                                       (__first + (__secondChild - 1))))
            __secondChild--;
        *(__first + __holeIndex) = DSL_SORT_MOVE(*(__first + __secondChild));
        __holeIndex = __secondChild;
    }
    if ((__len & 1) == 0 && __secondChild == (__len - 2) / 2)
    {
        __secondChild = 2 * (__secondChild + 1);
        *(__first + __holeIndex) = DSL_SORT_MOVE(*(__first + (__secondChild - 1)));
        __holeIndex = __secondChild - 1;
    }
    DSL_SORT_GETNAME(push_heap)
    (__first, __holeIndex, __topIndex, DSL_SORT_MOVE(__value));
}

static inline void
DSL_SORT_GETNAME(pop_heap)(DSL_SORT_VALUE_TYPE *__first,
                                     DSL_SORT_VALUE_TYPE *__last,
                                     DSL_SORT_VALUE_TYPE *__result)
{
    DSL_SORT_VALUE_TYPE __value = DSL_SORT_MOVE(*__result);
    *__result = DSL_SORT_MOVE(*__first);
    DSL_SORT_GETNAME(adjust_heap)
    (__first, (DISTANCE_TYPE)(0), (DISTANCE_TYPE)(__last - __first),
     DSL_SORT_MOVE(__value));
}

static inline void
DSL_SORT_GETNAME(make_heap)(DSL_SORT_VALUE_TYPE *__first,
                                      DSL_SORT_VALUE_TYPE *__last)
{

    if (__last - __first < 2)
        return;

    const DISTANCE_TYPE __len = __last - __first;
    DISTANCE_TYPE __parent = (__len - 2) / 2;
    while (1)
    {
        DSL_SORT_VALUE_TYPE __value = DSL_SORT_MOVE(*((__first) + __parent));
        DSL_SORT_GETNAME(adjust_heap)
        (__first, __parent, __len, DSL_SORT_MOVE(__value));
        if (__parent == 0)
            return;
        __parent--;
    }
}

static inline void DSL_SORT_GETNAME(heap_select)(DSL_SORT_VALUE_TYPE *__first,
                                 DSL_SORT_VALUE_TYPE *__middle,
                                 DSL_SORT_VALUE_TYPE *__last)
{
    DSL_SORT_GETNAME(make_heap)(__first, __middle);
    for (DSL_SORT_VALUE_TYPE *__i = __middle; __i < __last; ++__i)
        if (DSL_SORT_COMP_PTR_FUNC((__i), (__first)))
            DSL_SORT_GETNAME(pop_heap)(__first, __middle, __i);
}

static inline void
DSL_SORT_GETNAME(sort_heap)(DSL_SORT_VALUE_TYPE *__first,
                                      DSL_SORT_VALUE_TYPE *__last)
{
    while (__last - __first > 1)
    {
        --__last;
        DSL_SORT_GETNAME(pop_heap)(__first, __last, __last);
    }
}

static void
DSL_SORT_GETNAME(partial_sort)(DSL_SORT_VALUE_TYPE *__first,
                                       DSL_SORT_VALUE_TYPE *__middle,
                                       DSL_SORT_VALUE_TYPE *__last)
{
    DSL_SORT_GETNAME(heap_select)(__first, __middle, __last);
    DSL_SORT_GETNAME(sort_heap)(__first, __middle);
}


// nth_element =====================================================

static void DSL_SORT_GETNAME(move_median_to_first)(
    DSL_SORT_VALUE_TYPE * __result,
    DSL_SORT_VALUE_TYPE * __a,
    DSL_SORT_VALUE_TYPE * __b,
    DSL_SORT_VALUE_TYPE * __c)
{
    if (DSL_SORT_COMP_PTR_FUNC(__a, __b))
    {
        if (DSL_SORT_COMP_PTR_FUNC(__b, __c))
        {
            __dsl_iter_swap(__result, __b);
        }
        else if (DSL_SORT_COMP_PTR_FUNC(__a, __c))
        {
            __dsl_iter_swap(__result, __c);
        }
        else
        {
            __dsl_iter_swap(__result, __a);
        }
    }
    else if (DSL_SORT_COMP_PTR_FUNC(__a, __c))
    {
        __dsl_iter_swap(__result, __a);
    }
    else if (DSL_SORT_COMP_PTR_FUNC(__b, __c))
    {
        __dsl_iter_swap(__result, __c);
    }
    else
    {
        __dsl_iter_swap(__result, __b);
    }
}

static DSL_SORT_VALUE_TYPE *DSL_SORT_GETNAME(unguarded_partition)(
        DSL_SORT_VALUE_TYPE * __first,
        DSL_SORT_VALUE_TYPE * __last,
        DSL_SORT_VALUE_TYPE * __pivot)
{
    while (true)
    {
        while (DSL_SORT_COMP_PTR_FUNC(__first, __pivot))
            ++__first;
        --__last;
        while (DSL_SORT_COMP_PTR_FUNC(__pivot, __last))
            --__last;
        if (!(__first < __last))
            return __first;
        __dsl_iter_swap(__first, __last);
        ++__first;
    }
}

static inline DSL_SORT_VALUE_TYPE * DSL_SORT_GETNAME(unguarded_partition_pivot)(
        DSL_SORT_VALUE_TYPE * __first,
        DSL_SORT_VALUE_TYPE * __last)
{
    DSL_SORT_VALUE_TYPE * __mid = __first + (__last - __first) / 2;
    DSL_SORT_GETNAME(move_median_to_first)(__first, __first + 1, __mid, __last - 1);
    return DSL_SORT_GETNAME(unguarded_partition)(__first + 1, __last, __first);
}

static void DSL_SORT_GETNAME(unguarded_linear_insert)(DSL_SORT_VALUE_TYPE * __last)
{
    DSL_SORT_VALUE_TYPE __val = DSL_SORT_MOVE(*__last);
    DSL_SORT_VALUE_TYPE * __next = __last;
    --__next;
    while (DSL_SORT_COMP_PTR_FUNC((&__val), __next))
    {
        *__last = DSL_SORT_MOVE(*__next);
        __last = __next;
        --__next;
    }
    *__last = DSL_SORT_MOVE(__val);
}

static void DSL_SORT_GETNAME(insertion_sort)(
        DSL_SORT_VALUE_TYPE * __first,
        DSL_SORT_VALUE_TYPE * __last)
{
    if (__first == __last) return;

    for (DSL_SORT_VALUE_TYPE * __i = __first + 1; __i != __last; ++__i)
    {
        if (DSL_SORT_COMP_PTR_FUNC(__i, __first))
        {
            DSL_SORT_VALUE_TYPE __val = DSL_SORT_MOVE(*__i);
            DSL_SORT_VALUE_TYPE *__ii = __i + 1;
            DSL_MOVE_BACKWARD(__first, __i, __ii);
            *__first = DSL_SORT_MOVE(__val);
        }
        else DSL_SORT_GETNAME(unguarded_linear_insert)(__i);
    }
}

static void DSL_SORT_GETNAME(introselect)(
    DSL_SORT_VALUE_TYPE *__first,
    DSL_SORT_VALUE_TYPE *__nth,
    DSL_SORT_VALUE_TYPE * __last,
    DISTANCE_TYPE __depth_limit)
{
    while (__last - __first > 3)
    {
    if (__depth_limit == 0)
        {
            DSL_SORT_GETNAME(heap_select)(__first, __nth + 1, __last);
            // Place the nth largest element in its final position.
            __dsl_iter_swap(__first, __nth);
            return;
        }
    --__depth_limit;
    DSL_SORT_VALUE_TYPE * __cut =
        DSL_SORT_GETNAME(unguarded_partition_pivot)(__first, __last);
    if (__cut <= __nth)
        __first = __cut;
    else
        __last = __cut;
    }
    DSL_SORT_GETNAME(insertion_sort)(__first, __last);
}

static void DSL_SORT_GETNAME(nth_element)(DSL_SORT_VALUE_TYPE *__first, 
                                                  DSL_SORT_VALUE_TYPE *__nth,
                                                  DSL_SORT_VALUE_TYPE *__last)
{
    if (__first == __last || __nth == __last)
        return;
    DSL_SORT_GETNAME(introselect)(__first, __nth, __last,
            __dsl_lg(__last - __first) * 2);
}



// sort =====================================================


static void DSL_SORT_GETNAME(introsort_loop)(
        DSL_SORT_VALUE_TYPE * __first,
        DSL_SORT_VALUE_TYPE * __last,
        DISTANCE_TYPE __depth_limit)
{
    while (__last - __first > DSL_S_threshold)
    {
        if (__depth_limit == 0)
        {
            DSL_SORT_GETNAME(partial_sort)(__first, __last, __last);
            return;
        }
        --__depth_limit;
        DSL_SORT_VALUE_TYPE * __cut =
            DSL_SORT_GETNAME(unguarded_partition_pivot)(__first, __last);
        DSL_SORT_GETNAME(introsort_loop)(__cut, __last, __depth_limit);
        __last = __cut;
    }
}


static inline void DSL_SORT_GETNAME(unguarded_insertion_sort)(
        DSL_SORT_VALUE_TYPE * __first,
        DSL_SORT_VALUE_TYPE * __last)
{
    for (DSL_SORT_VALUE_TYPE * __i = __first; __i != __last; ++__i)
        DSL_SORT_GETNAME(unguarded_linear_insert)(__i);
}


static void DSL_SORT_GETNAME(final_insertion_sort)(
        DSL_SORT_VALUE_TYPE * __first,
        DSL_SORT_VALUE_TYPE * __last)
{
    if (__last - __first > DSL_S_threshold)
    {
        DSL_SORT_GETNAME(insertion_sort)(__first, __first + DSL_S_threshold);
        DSL_SORT_GETNAME(unguarded_insertion_sort)(__first + DSL_S_threshold, __last);
    }
    else
        DSL_SORT_GETNAME(insertion_sort)(__first, __last);
}



static void DSL_SORT_GETNAME(sort)(
        DSL_SORT_VALUE_TYPE * __first,
        DSL_SORT_VALUE_TYPE * __last)
{
    if (__first != __last)
    {
        DSL_SORT_GETNAME(introsort_loop)(__first, __last,
            __dsl_lg(__last - __first) * 2);
        DSL_SORT_GETNAME(final_insertion_sort)(__first, __last);
    }
}

#undef  DSL_S_threshold
#undef DSL_MOVE_BACKWARD
#undef __dsl_iter_swap

#undef DISTANCE_TYPE
#undef DSL_SORT_MOVE
#undef DSL_SORT_GETNAME_IMPL2
#undef DSL_SORT_GETNAME_IMPL
#undef DSL_SORT_GETNAME

#undef DSL_SORT_SUFFIX
#undef DSL_SORT_VALUE_TYPE
#undef DSL_SORT_COMP_PTR_FUNC

#endif

#endif

#endif