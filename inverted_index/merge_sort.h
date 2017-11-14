#ifndef MERGE_SORT_H
#define MERGE_SORT_H

#include <stdlib.h>

void merge(void *base,
           size_t num,
           size_t el_size,
           size_t size,
           int (*compar)(const void *, const void *));

void merge_sort(void *base,
                size_t num,
                size_t el_size,
                int (*compar)(const void *, const void *));

#endif
