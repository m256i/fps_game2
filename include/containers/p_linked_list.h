#ifndef P_LIST_H
#define P_LIST_H

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// Optional callbacks for non-POD types
// Destructor: called before freeing stored value
// Copier: returns a newly allocated copy of the value
typedef void  (*l_list_destructor_fn)(void *);
typedef void *(*l_list_copy_fn)(const void *);

/**
 * CREATE_LINKED_list_TYPE(TYPE)
 *  - Defines a doubly-linked list for TYPE
 *  - Supports push/pop front/back, insert_after/before, splice, reverse, find,
 * clear
 *  - O(1) size tracking
 */
#define CREATE_LINKED_LIST_TYPE(TYPE)                                           \
                                                                                \
  typedef struct TYPE##_node {                                                  \
    struct TYPE##_node *prev, *next;                                            \
    TYPE                value;                                                  \
  } TYPE##_node;                                                                \
                                                                                \
  typedef struct {                                                              \
    TYPE##_node         *head, *tail;                                           \
    size_t               size;                                                  \
    l_list_destructor_fn dtor;                                                  \
    l_list_copy_fn       copier;                                                \
  } l_list_##TYPE;                                                              \
                                                                                \
  /** Initialize to empty list **/                                              \
  static inline l_list_##TYPE l_list_##TYPE##_create(                           \
    l_list_copy_fn       cp,                                                    \
    l_list_destructor_fn dt                                                     \
  ) {                                                                           \
    return (l_list_##TYPE                                                       \
    ){.head = NULL, .tail = NULL, .size = 0, .copier = cp, .dtor = dt};         \
  }                                                                             \
                                                                                \
  /** Initialize to empty list **/                                              \
  static inline l_list_##TYPE l_list_##TYPE##_create_() {                       \
    return (l_list_##TYPE                                                       \
    ){.head = NULL, .tail = NULL, .size = 0, .copier = NULL, .dtor = NULL};     \
  }                                                                             \
                                                                                \
  /** Free all nodes and values **/                                             \
  static inline void l_list_##TYPE##_free(l_list_##TYPE *l) {                   \
    TYPE##_node *cur = l->head;                                                 \
    while (cur) {                                                               \
      TYPE##_node *next = cur->next;                                            \
      if (l->dtor) l->dtor(&cur->value);                                        \
      free(cur);                                                                \
      cur = next;                                                               \
    }                                                                           \
    l->head = l->tail = NULL;                                                   \
    l->size           = 0;                                                      \
  }                                                                             \
                                                                                \
  static inline int l_list_##TYPE##_empty(const l_list_##TYPE *l) {             \
    return l->size == 0;                                                        \
  }                                                                             \
  static inline size_t l_list_##TYPE##_size(const l_list_##TYPE *l) {           \
    return l->size;                                                             \
  }                                                                             \
                                                                                \
  /** Internal: allocate node, copy value **/                                   \
  static inline TYPE##_node *l_list_##TYPE##_make_node(                         \
    l_list_##TYPE *l,                                                           \
    const TYPE    *v                                                            \
  ) {                                                                           \
    TYPE##_node *node = (TYPE##_node *)malloc(sizeof(TYPE##_node));             \
    assert(node);                                                               \
    if (l->copier) {                                                            \
      TYPE *copy  = (TYPE *)l->copier(v);                                       \
      node->value = *copy;                                                      \
      free(copy);                                                               \
    } else {                                                                    \
      node->value = *v;                                                         \
    }                                                                           \
    node->prev = node->next = NULL;                                             \
    return node;                                                                \
  }                                                                             \
                                                                                \
  /** Push at back **/                                                          \
  static inline void l_list_##TYPE##_push_back(                                 \
    l_list_##TYPE *l,                                                           \
    const TYPE    *v                                                            \
  ) {                                                                           \
    TYPE##_node *n = l_list_##TYPE##_make_node(l, v);                           \
    n->prev        = l->tail;                                                   \
    if (l->tail) l->tail->next = n;                                             \
    else l->head = n;                                                           \
    l->tail = n;                                                                \
    l->size++;                                                                  \
  }                                                                             \
                                                                                \
  /** Push at front **/                                                         \
  static inline void l_list_##TYPE##_push_front(                                \
    l_list_##TYPE *l,                                                           \
    const TYPE    *v                                                            \
  ) {                                                                           \
    TYPE##_node *n = l_list_##TYPE##_make_node(l, v);                           \
    n->next        = l->head;                                                   \
    if (l->head) l->head->prev = n;                                             \
    else l->tail = n;                                                           \
    l->head = n;                                                                \
    l->size++;                                                                  \
  }                                                                             \
                                                                                \
  /** Pop front/back (returns 0 on success, -1 if empty) **/                    \
  static inline int l_list_##TYPE##_pop_front(l_list_##TYPE *l, TYPE *out) {    \
    if (!l->head) return -1;                                                    \
    TYPE##_node *n = l->head;                                                   \
    if (out) *out = n->value;                                                   \
    l->head = n->next;                                                          \
    if (l->head) l->head->prev = NULL;                                          \
    else l->tail = NULL;                                                        \
    if (l->dtor) l->dtor(&n->value);                                            \
    free(n);                                                                    \
    l->size--;                                                                  \
    return 0;                                                                   \
  }                                                                             \
  static inline int l_list_##TYPE##_pop_back(l_list_##TYPE *l, TYPE *out) {     \
    if (!l->tail) return -1;                                                    \
    TYPE##_node *n = l->tail;                                                   \
    if (out) *out = n->value;                                                   \
    l->tail = n->prev;                                                          \
    if (l->tail) l->tail->next = NULL;                                          \
    else l->head = NULL;                                                        \
    if (l->dtor) l->dtor(&n->value);                                            \
    free(n);                                                                    \
    l->size--;                                                                  \
    return 0;                                                                   \
  }                                                                             \
                                                                                \
  /** Insert after given node (pos must be non-NULL) **/                        \
  static inline int l_list_##TYPE##_insert_after(                               \
    l_list_##TYPE *l,                                                           \
    TYPE##_node   *pos,                                                         \
    const TYPE    *v                                                            \
  ) {                                                                           \
    if (!pos) return -1;                                                        \
    TYPE##_node *n = l_list_##TYPE##_make_node(l, v);                           \
    n->next        = pos->next;                                                 \
    n->prev        = pos;                                                       \
    if (pos->next) pos->next->prev = n;                                         \
    else l->tail = n;                                                           \
    pos->next = n;                                                              \
    l->size++;                                                                  \
    return 0;                                                                   \
  }                                                                             \
                                                                                \
  /** Insert before given node (pos must be non-NULL) **/                       \
  static inline int l_list_##TYPE##_insert_before(                              \
    l_list_##TYPE *l,                                                           \
    TYPE##_node   *pos,                                                         \
    const TYPE    *v                                                            \
  ) {                                                                           \
    if (!pos) return -1;                                                        \
    TYPE##_node *n = l_list_##TYPE##_make_node(l, v);                           \
    n->prev        = pos->prev;                                                 \
    n->next        = pos;                                                       \
    if (pos->prev) pos->prev->next = n;                                         \
    else l->head = n;                                                           \
    pos->prev = n;                                                              \
    l->size++;                                                                  \
    return 0;                                                                   \
  }                                                                             \
                                                                                \
  /** Remove arbitrary node (node must be non-NULL) **/                         \
  static inline int l_list_##TYPE##_remove_node(                                \
    l_list_##TYPE *l,                                                           \
    TYPE##_node   *n                                                            \
  ) {                                                                           \
    if (!n) return -1;                                                          \
    if (n->prev) n->prev->next = n->next;                                       \
    else l->head = n->next;                                                     \
    if (n->next) n->next->prev = n->prev;                                       \
    else l->tail = n->prev;                                                     \
    if (l->dtor) l->dtor(&n->value);                                            \
    free(n);                                                                    \
    l->size--;                                                                  \
    return 0;                                                                   \
  }                                                                             \
                                                                                \
  /** Find first node by value (memcmp, only for POD types) **/                 \
  static inline TYPE##_node *l_list_##TYPE##_find(                              \
    const l_list_##TYPE *l,                                                     \
    const TYPE          *v                                                      \
  ) {                                                                           \
    for (TYPE##_node *it = l->head; it; it = it->next) {                        \
      if (memcmp(&it->value, v, sizeof(TYPE)) == 0) return it;                  \
    }                                                                           \
    return NULL;                                                                \
  }                                                                             \
                                                                                \
  /* Splice entire other list into this before pos (pos==NULL => at end) **/    \
  static inline void l_list_##TYPE##_splice(                                    \
    l_list_##TYPE *l,                                                           \
    TYPE##_node   *pos,                                                         \
    l_list_##TYPE *other                                                        \
  ) {                                                                           \
    if (!other->head) return;                                                   \
    /*detach other*/                                                            \
    TYPE##_node *oh = other->head;                                              \
    TYPE##_node *ot = other->tail;                                              \
    if (!pos) {                                                                 \
      /* splice at end*/                                                        \
      ot->next = NULL;                                                          \
      oh->prev = l->tail;                                                       \
      if (l->tail) l->tail->next = oh;                                          \
      else l->head = oh;                                                        \
      l->tail = ot;                                                             \
    } else {                                                                    \
      /* splice before pos */                                                   \
      TYPE##_node *pp = pos->prev;                                              \
      ot->next        = pos;                                                    \
      pos->prev       = ot;                                                     \
      oh->prev        = pp;                                                     \
      if (pp) pp->next = oh;                                                    \
      else l->head = oh;                                                        \
    }                                                                           \
    l->size += other->size;                                                     \
    *other   = l_list_##TYPE##_create(other->copier, other->dtor);              \
  }                                                                             \
                                                                                \
  /** Reverse list in-place **/                                                 \
  static inline void l_list_##TYPE##_reverse(l_list_##TYPE *l) {                \
    TYPE##_node *cur = l->head;                                                 \
    TYPE##_node *tmp = NULL;                                                    \
    while (cur) {                                                               \
      tmp       = cur->prev;                                                    \
      cur->prev = cur->next;                                                    \
      cur->next = tmp;                                                          \
      cur       = cur->prev;                                                    \
    }                                                                           \
    if (tmp) { /* tmp is at old head's prev => new head is tmp->prev*/          \
      l->tail = l->head;                                                        \
      l->head = tmp->prev;                                                      \
    }                                                                           \
  }                                                                             \
  static inline void l_list_##TYPE##_for_each(                                  \
    const l_list_##TYPE *l,                                                     \
    void                 (*fn)(TYPE * elem, void *user_data),                   \
    void                *user_data                                              \
  ) {                                                                           \
    for (TYPE##_node *it = l->head; it; it = it->next)                          \
      fn(&it->value, user_data);                                                \
  }                                                                             \
                                                                                \
  static inline TYPE##_node *l_list_##TYPE##_find_if(                           \
    const l_list_##TYPE *l,                                                     \
    int                  (*predicate)(const TYPE *elem, void *user_data),       \
    void                *user_data                                              \
  ) {                                                                           \
    for (TYPE##_node *it = l->head; it; it = it->next)                          \
      if (predicate(&it->value, user_data)) return it;                          \
    return NULL;                                                                \
  }                                                                             \
                                                                                \
  /* Remove all elements for which `predicate(&value, user_data)` returns */    \
  /* non-zero. */                                                               \
  static inline void l_list_##TYPE##_remove_if(                                 \
    l_list_##TYPE *l,                                                           \
    int            (*predicate)(const TYPE *elem, void *user_data),             \
    void          *user_data                                                    \
  ) {                                                                           \
    TYPE##_node *it = l->head;                                                  \
    while (it) {                                                                \
      TYPE##_node *next = it->next;                                             \
      if (predicate(&it->value, user_data)) {                                   \
        l_list_##TYPE##_remove_node(l, it);                                     \
      }                                                                         \
      it = next;                                                                \
    }                                                                           \
  }                                                                             \
  /**                                                                           \
   * Insert value *v* at position idx (0 == before head, idx == size == at      \
   * end). Returns 0 on success, -1 if idx > size.                              \
   */                                                                           \
  static inline int l_list_##TYPE##_insert_at(                                  \
    l_list_##TYPE *l,                                                           \
    size_t         idx,                                                         \
    const TYPE    *v                                                            \
  ) {                                                                           \
    if (idx > l->size) return -1;                                               \
    if (idx == 0) {                                                             \
      l_list_##TYPE##_push_front(l, v);                                         \
      return 0;                                                                 \
    }                                                                           \
    if (idx == l->size) {                                                       \
      l_list_##TYPE##_push_back(l, v);                                          \
      return 0;                                                                 \
    }                                                                           \
    /* walk to the node currently at position idx */                            \
    TYPE##_node *cur = l->head;                                                 \
    for (size_t i = 0; i < idx; ++i)                                            \
      cur = cur->next;                                                          \
    /* insert before it */                                                      \
    return l_list_##TYPE##_insert_before(l, cur, v);                            \
  }                                                                             \
                                                                                \
  /**                                                                           \
   * Remove the node at position idx (0 == head, size-1 == tail),               \
   * optionally writing its value into *out. Returns 0 on success, -1 if empty  \
   * or idx >= size.                                                            \
   */                                                                           \
  static inline int l_list_##TYPE##_remove_at(                                  \
    l_list_##TYPE *l,                                                           \
    size_t         idx,                                                         \
    TYPE          *out                                                          \
  ) {                                                                           \
    if (idx >= l->size) return -1;                                              \
    /* walk to the node to remove */                                            \
    TYPE##_node *cur = l->head;                                                 \
    for (size_t i = 0; i < idx; ++i)                                            \
      cur = cur->next;                                                          \
    /* capture value if requested */                                            \
    if (out) *out = cur->value;                                                 \
    /* unlink & free */                                                         \
    return l_list_##TYPE##_remove_node(l, cur);                                 \
  }                                                                             \
  /*Return the node at zero - based index `idx`,                                \
      or NULL if out of bounds.*/                                               \
  static inline TYPE##_node *l_list_##TYPE##_node_at(                           \
    l_list_##TYPE *l,                                                           \
    size_t         idx                                                          \
  ) {                                                                           \
    TYPE##_node *cur = l->head;                                                 \
    while (cur && idx--)                                                        \
      cur = cur->next;                                                          \
    return cur;                                                                 \
  }                                                                             \
                                                                                \
  /**                                                                           \
   * Return a pointer to the element at `idx` (or NULL if out of bounds).       \
   */                                                                           \
  static inline TYPE *l_list_##TYPE##_at(l_list_##TYPE *l, size_t idx) {        \
    TYPE##_node *n = l_list_##TYPE##_node_at(l, idx);                           \
    return n ? &n->value : NULL;                                                \
  }                                                                             \
                                                                                \
  /**                                                                           \
   * Const‐qualified version of `at()`.                                       \
   */                                                                           \
  static inline const TYPE *l_list_##TYPE##_at_const(                           \
    const l_list_##TYPE *l,                                                     \
    size_t               idx                                                    \
  ) {                                                                           \
    /* cast away const to reuse node_at */                                      \
    return (const TYPE                                                          \
              *)(l_list_##TYPE##_node_at((l_list_##TYPE *)l, idx)               \
                   ? &l_list_##TYPE##_node_at((l_list_##TYPE *)l, idx)->value   \
                   : NULL);                                                     \
  }                                                                             \
                                                                                \
  /**                                                                           \
   * Copy the element at `idx` into *out and return 0, or –1 if idx>=size.    \
   */                                                                           \
  static inline int l_list_##TYPE##_get_at(                                     \
    const l_list_##TYPE *l,                                                     \
    size_t               idx,                                                   \
    TYPE                *out                                                    \
  ) {                                                                           \
    TYPE##_node *n = l_list_##TYPE##_node_at((l_list_##TYPE *)l, idx);          \
    if (!n) return -1;                                                          \
    if (out) *out = n->value;                                                   \
    return 0;                                                                   \
  }                                                                             \
                                                                                \
  /**                                                                           \
   * Peek at the front element (without removing). 0 on success, –1 if empty. \
   */                                                                           \
  static inline int l_list_##TYPE##_front(const l_list_##TYPE *l, TYPE *out) {  \
    if (!l->head) return -1;                                                    \
    if (out) *out = l->head->value;                                             \
    return 0;                                                                   \
  }                                                                             \
                                                                                \
  /**                                                                           \
   * Peek at the back element (without removing). 0 on success, –1 if empty.  \
   */                                                                           \
  static inline int l_list_##TYPE##_back(const l_list_##TYPE *l, TYPE *out) {   \
    if (!l->tail) return -1;                                                    \
    if (out) *out = l->tail->value;                                             \
    return 0;                                                                   \
  }

#endif
