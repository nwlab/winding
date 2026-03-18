/**
 * @file rdnx_list_gen.h
 * @author Software development team
 * @brief Generic linked list operations
 * @version 1.0
 * @date 2024-09-09
 */

/*
 * Based on code from Zephyr project
 * zephyr/include/sys/list_gen.h
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RDNX_LIST_GEN_H
#define RDNX_LIST_GEN_H

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <stdbool.h>
#include <stddef.h>

/* Core includes. */
#include <rdnx_macro.h>

/* ============================================================================
 * Public Macro Definitions
 * ==========================================================================*/

// clang-format off

#define RDNX_GENLIST_FOR_EACH_NODE(__lname, __l, __sn)         \
    for (__sn = rdnx_##__lname##_peek_head(__l); __sn != NULL; \
         __sn = rdnx_##__lname##_peek_next(__sn))

#define RDNX_GENLIST_ITERATE_FROM_NODE(__lname, __l, __sn)       \
    for (__sn = __sn ? rdnx_##__lname##_peek_next_no_check(__sn) \
                     : rdnx_##__lname##_peek_head(__l);          \
         __sn != NULL;                                           \
         __sn = rdnx_##__lname##_peek_next(__sn))

#define RDNX_GENLIST_FOR_EACH_NODE_SAFE(__lname, __l, __sn, __sns) \
    for (__sn = rdnx_##__lname##_peek_head(__l),                   \
        __sns = rdnx_##__lname##_peek_next(__sn);                  \
         __sn != NULL; __sn = __sns,                               \
        __sns = rdnx_##__lname##_peek_next(__sn))

#define RDNX_GENLIST_CONTAINER(__ln, container_type, __n) \
    ((__ln) ? RDNX_CONTAINER_OF((__ln), container_type, __n) : NULL)

#define RDNX_GENLIST_PEEK_HEAD_CONTAINER(__lname, __l, container_type, __n) \
    RDNX_GENLIST_CONTAINER(rdnx_##__lname##_peek_head(__l), container_type, __n)

#define RDNX_GENLIST_PEEK_TAIL_CONTAINER(__lname, __l, container_type, __n) \
    RDNX_GENLIST_CONTAINER(rdnx_##__lname##_peek_tail(__l), container_type, __n)

#define RDNX_GENLIST_PEEK_NEXT_CONTAINER(__lname, container_type, __cn, __n) \
    ((__cn) ? RDNX_GENLIST_CONTAINER(                                        \
                  rdnx_##__lname##_peek_next(&((__cn)->__n)),                \
                  container_type, __n)                                       \
            : NULL)

#define RDNX_GENLIST_FOR_EACH_CONTAINER(__lname, __l, container_type, __cn, __n) \
    for (__cn = RDNX_GENLIST_PEEK_HEAD_CONTAINER(__lname, __l, container_type,   \
                                                 __n);                           \
         __cn != NULL;                                                           \
         __cn = RDNX_GENLIST_PEEK_NEXT_CONTAINER(__lname, container_type, __cn, __n))

#define RDNX_GENLIST_FOR_EACH_CONTAINER_SAFE(__lname, __l, container_type, __cn, __cns, __n) \
    for (__cn = RDNX_GENLIST_PEEK_HEAD_CONTAINER(__lname, __l, container_type, __n),         \
        __cns = RDNX_GENLIST_PEEK_NEXT_CONTAINER(__lname, container_type, __cn, __n);        \
         __cn != NULL; __cn = __cns,                                                         \
        __cns = RDNX_GENLIST_PEEK_NEXT_CONTAINER(__lname, container_type, __cn, __n))

#define RDNX_GENLIST_IS_EMPTY(__lname)                     \
    static inline bool                                     \
        rdnx_##__lname##_is_empty(sys_##__lname##_t *list) \
    {                                                      \
        return (rdnx_##__lname##_peek_head(list) == NULL); \
    }

#define RDNX_GENLIST_PEEK_NEXT_NO_CHECK(__lname, __nname)            \
    static inline sys_##__nname##_t *                                \
        rdnx_##__lname##_peek_next_no_check(sys_##__nname##_t *node) \
    {                                                                \
        return rdnx_##__nname##_next_peek(node);                     \
    }

#define RDNX_GENLIST_PEEK_NEXT(__lname, __nname)                                \
    static inline sys_##__nname##_t *                                           \
        rdnx_##__lname##_peek_next(sys_##__nname##_t *node)                     \
    {                                                                           \
        return node != NULL ? rdnx_##__lname##_peek_next_no_check(node) : NULL; \
    }

#define RDNX_GENLIST_PREPEND(__lname, __nname)                           \
    static inline void                                                   \
        rdnx_##__lname##_prepend(sys_##__lname##_t *list,                \
                                 sys_##__nname##_t *node)                \
    {                                                                    \
        rdnx_##__nname##_next_set(node,                                  \
                                  rdnx_##__lname##_peek_head(list));     \
        rdnx_##__lname##_head_set(list, node);                           \
                                                                         \
        if (rdnx_##__lname##_peek_tail(list) == NULL)                    \
        {                                                                \
            rdnx_##__lname##_tail_set(list,                              \
                                      rdnx_##__lname##_peek_head(list)); \
        }                                                                \
    }

#define RDNX_GENLIST_APPEND(__lname, __nname)            \
    static inline void                                   \
        rdnx_##__lname##_append(sys_##__lname##_t *list, \
                                sys_##__nname##_t *node) \
    {                                                    \
        rdnx_##__nname##_next_set(node, NULL);           \
                                                         \
        if (rdnx_##__lname##_peek_tail(list) == NULL)    \
        {                                                \
            rdnx_##__lname##_tail_set(list, node);       \
            rdnx_##__lname##_head_set(list, node);       \
        }                                                \
        else                                             \
        {                                                \
            rdnx_##__nname##_next_set(                   \
                rdnx_##__lname##_peek_tail(list),        \
                node);                                   \
            rdnx_##__lname##_tail_set(list, node);       \
        }                                                \
    }

#define RDNX_GENLIST_APPEND_LIST(__lname, __nname)                \
    static inline void                                            \
        rdnx_##__lname##_append_list(sys_##__lname##_t *list,     \
                                     void *head, void *tail)      \
    {                                                             \
        if (rdnx_##__lname##_peek_tail(list) == NULL)             \
        {                                                         \
            rdnx_##__lname##_head_set(list,                       \
                                      (sys_##__nname##_t *)head); \
        }                                                         \
        else                                                      \
        {                                                         \
            rdnx_##__nname##_next_set(                            \
                rdnx_##__lname##_peek_tail(list),                 \
                (sys_##__nname##_t *)head);                       \
        }                                                         \
        rdnx_##__lname##_tail_set(list,                           \
                                  (sys_##__nname##_t *)tail);     \
    }

#define RDNX_GENLIST_MERGE_LIST(__lname, __nname)          \
    static inline void                                     \
        rdnx_##__lname##_merge_##__lname(                  \
            sys_##__lname##_t *list,                       \
            sys_##__lname##_t *list_to_append)             \
    {                                                      \
        sys_##__nname##_t *head, *tail;                    \
        head = rdnx_##__lname##_peek_head(list_to_append); \
        tail = rdnx_##__lname##_peek_tail(list_to_append); \
        rdnx_##__lname##_append_list(list, head, tail);    \
        rdnx_##__lname##_init(list_to_append);             \
    }

#define RDNX_GENLIST_INSERT(__lname, __nname)                            \
    static inline void                                                   \
        rdnx_##__lname##_insert(sys_##__lname##_t *list,                 \
                                sys_##__nname##_t *prev,                 \
                                sys_##__nname##_t *node)                 \
    {                                                                    \
        if (prev == NULL)                                                \
        {                                                                \
            rdnx_##__lname##_prepend(list, node);                        \
        }                                                                \
        else if (rdnx_##__nname##_next_peek(prev) == NULL)               \
        {                                                                \
            rdnx_##__lname##_append(list, node);                         \
        }                                                                \
        else                                                             \
        {                                                                \
            rdnx_##__nname##_next_set(node,                              \
                                      rdnx_##__nname##_next_peek(prev)); \
            rdnx_##__nname##_next_set(prev, node);                       \
        }                                                                \
    }

#define RDNX_GENLIST_GET_NOT_EMPTY(__lname, __nname)                     \
    static inline sys_##__nname##_t *                                    \
        rdnx_##__lname##_get_not_empty(sys_##__lname##_t *list)          \
    {                                                                    \
        sys_##__nname##_t *node =                                        \
            rdnx_##__lname##_peek_head(list);                            \
                                                                         \
        rdnx_##__lname##_head_set(list,                                  \
                                  rdnx_##__nname##_next_peek(node));     \
        if (rdnx_##__lname##_peek_tail(list) == node)                    \
        {                                                                \
            rdnx_##__lname##_tail_set(list,                              \
                                      rdnx_##__lname##_peek_head(list)); \
        }                                                                \
                                                                         \
        return node;                                                     \
    }

#define RDNX_GENLIST_GET(__lname, __nname)                                                    \
    static inline sys_##__nname##_t *                                                         \
        rdnx_##__lname##_get(sys_##__lname##_t *list)                                         \
    {                                                                                         \
        return rdnx_##__lname##_is_empty(list) ? NULL : rdnx_##__lname##_get_not_empty(list); \
    }

#define RDNX_GENLIST_REMOVE(__lname, __nname)                                \
    static inline void                                                       \
        rdnx_##__lname##_remove(sys_##__lname##_t *list,                     \
                                sys_##__nname##_t *prev_node,                \
                                sys_##__nname##_t *node)                     \
    {                                                                        \
        if (prev_node == NULL)                                               \
        {                                                                    \
            rdnx_##__lname##_head_set(list,                                  \
                                      rdnx_##__nname##_next_peek(node));     \
                                                                             \
            /* Was node also the tail? */                                    \
            if (rdnx_##__lname##_peek_tail(list) == node)                    \
            {                                                                \
                rdnx_##__lname##_tail_set(list,                              \
                                          rdnx_##__lname##_peek_head(list)); \
            }                                                                \
        }                                                                    \
        else                                                                 \
        {                                                                    \
            rdnx_##__nname##_next_set(prev_node,                             \
                                      rdnx_##__nname##_next_peek(node));     \
                                                                             \
            /* Was node the tail? */                                         \
            if (rdnx_##__lname##_peek_tail(list) == node)                    \
            {                                                                \
                rdnx_##__lname##_tail_set(list,                              \
                                          prev_node);                        \
            }                                                                \
        }                                                                    \
                                                                             \
        rdnx_##__nname##_next_set(node, NULL);                               \
    }

#define RDNX_GENLIST_FIND_AND_REMOVE(__lname, __nname)            \
    static inline bool                                            \
        rdnx_##__lname##_find_and_remove(sys_##__lname##_t *list, \
                                         sys_##__nname##_t *node) \
    {                                                             \
        sys_##__nname##_t *prev = NULL;                           \
        sys_##__nname##_t *test;                                  \
                                                                  \
        RDNX_GENLIST_FOR_EACH_NODE(__lname, list, test)           \
        {                                                         \
            if (test == node)                                     \
            {                                                     \
                rdnx_##__lname##_remove(list, prev,               \
                                        node);                    \
                return true;                                      \
            }                                                     \
                                                                  \
            prev = test;                                          \
        }                                                         \
                                                                  \
        return false;                                             \
    }

// clang-format on

#endif // RDNX_LIST_GEN_H
