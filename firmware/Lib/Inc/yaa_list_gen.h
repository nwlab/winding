/**
 * @file yaa_list_gen.h
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

#ifndef YAA_LIST_GEN_H
#define YAA_LIST_GEN_H

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <stdbool.h>
#include <stddef.h>

/* Core includes. */
#include <yaa_macro.h>

/* ============================================================================
 * Public Macro Definitions
 * ==========================================================================*/

// clang-format off

#define YAA_GENLIST_FOR_EACH_NODE(__lname, __l, __sn)         \
    for (__sn = yaa_##__lname##_peek_head(__l); __sn != NULL; \
         __sn = yaa_##__lname##_peek_next(__sn))

#define YAA_GENLIST_ITERATE_FROM_NODE(__lname, __l, __sn)       \
    for (__sn = __sn ? yaa_##__lname##_peek_next_no_check(__sn) \
                     : yaa_##__lname##_peek_head(__l);          \
         __sn != NULL;                                           \
         __sn = yaa_##__lname##_peek_next(__sn))

#define YAA_GENLIST_FOR_EACH_NODE_SAFE(__lname, __l, __sn, __sns) \
    for (__sn = yaa_##__lname##_peek_head(__l),                   \
        __sns = yaa_##__lname##_peek_next(__sn);                  \
         __sn != NULL; __sn = __sns,                               \
        __sns = yaa_##__lname##_peek_next(__sn))

#define YAA_GENLIST_CONTAINER(__ln, container_type, __n) \
    ((__ln) ? YAA_CONTAINER_OF((__ln), container_type, __n) : NULL)

#define YAA_GENLIST_PEEK_HEAD_CONTAINER(__lname, __l, container_type, __n) \
    YAA_GENLIST_CONTAINER(yaa_##__lname##_peek_head(__l), container_type, __n)

#define YAA_GENLIST_PEEK_TAIL_CONTAINER(__lname, __l, container_type, __n) \
    YAA_GENLIST_CONTAINER(yaa_##__lname##_peek_tail(__l), container_type, __n)

#define YAA_GENLIST_PEEK_NEXT_CONTAINER(__lname, container_type, __cn, __n) \
    ((__cn) ? YAA_GENLIST_CONTAINER(                                        \
                  yaa_##__lname##_peek_next(&((__cn)->__n)),                \
                  container_type, __n)                                       \
            : NULL)

#define YAA_GENLIST_FOR_EACH_CONTAINER(__lname, __l, container_type, __cn, __n) \
    for (__cn = YAA_GENLIST_PEEK_HEAD_CONTAINER(__lname, __l, container_type,   \
                                                 __n);                           \
         __cn != NULL;                                                           \
         __cn = YAA_GENLIST_PEEK_NEXT_CONTAINER(__lname, container_type, __cn, __n))

#define YAA_GENLIST_FOR_EACH_CONTAINER_SAFE(__lname, __l, container_type, __cn, __cns, __n) \
    for (__cn = YAA_GENLIST_PEEK_HEAD_CONTAINER(__lname, __l, container_type, __n),         \
        __cns = YAA_GENLIST_PEEK_NEXT_CONTAINER(__lname, container_type, __cn, __n);        \
         __cn != NULL; __cn = __cns,                                                         \
        __cns = YAA_GENLIST_PEEK_NEXT_CONTAINER(__lname, container_type, __cn, __n))

#define YAA_GENLIST_IS_EMPTY(__lname)                     \
    static inline bool                                     \
        yaa_##__lname##_is_empty(sys_##__lname##_t *list) \
    {                                                      \
        return (yaa_##__lname##_peek_head(list) == NULL); \
    }

#define YAA_GENLIST_PEEK_NEXT_NO_CHECK(__lname, __nname)            \
    static inline sys_##__nname##_t *                                \
        yaa_##__lname##_peek_next_no_check(sys_##__nname##_t *node) \
    {                                                                \
        return yaa_##__nname##_next_peek(node);                     \
    }

#define YAA_GENLIST_PEEK_NEXT(__lname, __nname)                                \
    static inline sys_##__nname##_t *                                           \
        yaa_##__lname##_peek_next(sys_##__nname##_t *node)                     \
    {                                                                           \
        return node != NULL ? yaa_##__lname##_peek_next_no_check(node) : NULL; \
    }

#define YAA_GENLIST_PREPEND(__lname, __nname)                           \
    static inline void                                                   \
        yaa_##__lname##_prepend(sys_##__lname##_t *list,                \
                                 sys_##__nname##_t *node)                \
    {                                                                    \
        yaa_##__nname##_next_set(node,                                  \
                                  yaa_##__lname##_peek_head(list));     \
        yaa_##__lname##_head_set(list, node);                           \
                                                                         \
        if (yaa_##__lname##_peek_tail(list) == NULL)                    \
        {                                                                \
            yaa_##__lname##_tail_set(list,                              \
                                      yaa_##__lname##_peek_head(list)); \
        }                                                                \
    }

#define YAA_GENLIST_APPEND(__lname, __nname)            \
    static inline void                                   \
        yaa_##__lname##_append(sys_##__lname##_t *list, \
                                sys_##__nname##_t *node) \
    {                                                    \
        yaa_##__nname##_next_set(node, NULL);           \
                                                         \
        if (yaa_##__lname##_peek_tail(list) == NULL)    \
        {                                                \
            yaa_##__lname##_tail_set(list, node);       \
            yaa_##__lname##_head_set(list, node);       \
        }                                                \
        else                                             \
        {                                                \
            yaa_##__nname##_next_set(                   \
                yaa_##__lname##_peek_tail(list),        \
                node);                                   \
            yaa_##__lname##_tail_set(list, node);       \
        }                                                \
    }

#define YAA_GENLIST_APPEND_LIST(__lname, __nname)                \
    static inline void                                            \
        yaa_##__lname##_append_list(sys_##__lname##_t *list,     \
                                     void *head, void *tail)      \
    {                                                             \
        if (yaa_##__lname##_peek_tail(list) == NULL)             \
        {                                                         \
            yaa_##__lname##_head_set(list,                       \
                                      (sys_##__nname##_t *)head); \
        }                                                         \
        else                                                      \
        {                                                         \
            yaa_##__nname##_next_set(                            \
                yaa_##__lname##_peek_tail(list),                 \
                (sys_##__nname##_t *)head);                       \
        }                                                         \
        yaa_##__lname##_tail_set(list,                           \
                                  (sys_##__nname##_t *)tail);     \
    }

#define YAA_GENLIST_MERGE_LIST(__lname, __nname)          \
    static inline void                                     \
        yaa_##__lname##_merge_##__lname(                  \
            sys_##__lname##_t *list,                       \
            sys_##__lname##_t *list_to_append)             \
    {                                                      \
        sys_##__nname##_t *head, *tail;                    \
        head = yaa_##__lname##_peek_head(list_to_append); \
        tail = yaa_##__lname##_peek_tail(list_to_append); \
        yaa_##__lname##_append_list(list, head, tail);    \
        yaa_##__lname##_init(list_to_append);             \
    }

#define YAA_GENLIST_INSERT(__lname, __nname)                            \
    static inline void                                                   \
        yaa_##__lname##_insert(sys_##__lname##_t *list,                 \
                                sys_##__nname##_t *prev,                 \
                                sys_##__nname##_t *node)                 \
    {                                                                    \
        if (prev == NULL)                                                \
        {                                                                \
            yaa_##__lname##_prepend(list, node);                        \
        }                                                                \
        else if (yaa_##__nname##_next_peek(prev) == NULL)               \
        {                                                                \
            yaa_##__lname##_append(list, node);                         \
        }                                                                \
        else                                                             \
        {                                                                \
            yaa_##__nname##_next_set(node,                              \
                                      yaa_##__nname##_next_peek(prev)); \
            yaa_##__nname##_next_set(prev, node);                       \
        }                                                                \
    }

#define YAA_GENLIST_GET_NOT_EMPTY(__lname, __nname)                     \
    static inline sys_##__nname##_t *                                    \
        yaa_##__lname##_get_not_empty(sys_##__lname##_t *list)          \
    {                                                                    \
        sys_##__nname##_t *node =                                        \
            yaa_##__lname##_peek_head(list);                            \
                                                                         \
        yaa_##__lname##_head_set(list,                                  \
                                  yaa_##__nname##_next_peek(node));     \
        if (yaa_##__lname##_peek_tail(list) == node)                    \
        {                                                                \
            yaa_##__lname##_tail_set(list,                              \
                                      yaa_##__lname##_peek_head(list)); \
        }                                                                \
                                                                         \
        return node;                                                     \
    }

#define YAA_GENLIST_GET(__lname, __nname)                                                    \
    static inline sys_##__nname##_t *                                                         \
        yaa_##__lname##_get(sys_##__lname##_t *list)                                         \
    {                                                                                         \
        return yaa_##__lname##_is_empty(list) ? NULL : yaa_##__lname##_get_not_empty(list); \
    }

#define YAA_GENLIST_REMOVE(__lname, __nname)                                \
    static inline void                                                       \
        yaa_##__lname##_remove(sys_##__lname##_t *list,                     \
                                sys_##__nname##_t *prev_node,                \
                                sys_##__nname##_t *node)                     \
    {                                                                        \
        if (prev_node == NULL)                                               \
        {                                                                    \
            yaa_##__lname##_head_set(list,                                  \
                                      yaa_##__nname##_next_peek(node));     \
                                                                             \
            /* Was node also the tail? */                                    \
            if (yaa_##__lname##_peek_tail(list) == node)                    \
            {                                                                \
                yaa_##__lname##_tail_set(list,                              \
                                          yaa_##__lname##_peek_head(list)); \
            }                                                                \
        }                                                                    \
        else                                                                 \
        {                                                                    \
            yaa_##__nname##_next_set(prev_node,                             \
                                      yaa_##__nname##_next_peek(node));     \
                                                                             \
            /* Was node the tail? */                                         \
            if (yaa_##__lname##_peek_tail(list) == node)                    \
            {                                                                \
                yaa_##__lname##_tail_set(list,                              \
                                          prev_node);                        \
            }                                                                \
        }                                                                    \
                                                                             \
        yaa_##__nname##_next_set(node, NULL);                               \
    }

#define YAA_GENLIST_FIND_AND_REMOVE(__lname, __nname)            \
    static inline bool                                            \
        yaa_##__lname##_find_and_remove(sys_##__lname##_t *list, \
                                         sys_##__nname##_t *node) \
    {                                                             \
        sys_##__nname##_t *prev = NULL;                           \
        sys_##__nname##_t *test;                                  \
                                                                  \
        YAA_GENLIST_FOR_EACH_NODE(__lname, list, test)           \
        {                                                         \
            if (test == node)                                     \
            {                                                     \
                yaa_##__lname##_remove(list, prev,               \
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

#endif // YAA_LIST_GEN_H
