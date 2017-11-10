#include "merge_sort.h"
void merge(void *base,
           size_t num,
           size_t el_size,
           size_t size,
           int (*compar)(const void *, const void *)) {
    size_t split = size * el_size;
    char *first  = malloc(split);
    char *p1     = memcpy(first, base, split);
    char *dest = base; 
    char *p2 = dest + split;
    size_t i = 0, j = size;
    while (i < size) {
        if (j == num || compar(p1, p2) <= 0) {
            for (size_t k = 0; k < el_size; k++)
                *dest++ = *p1++;
            i++;
        } else {
            for (size_t k = 0; k < el_size; k++)
                *dest++ = *p2++;
            j++;
        }
    }
    free(first);
}

void merge_sort(void *base,
                size_t num,
                size_t el_size,
                int (*compar)(const void *, const void *)) {
    if (num > 1) {
        size_t s        = (num + 1) / 2;
        char *char_base = base;
        merge_sort(char_base, s, el_size, compar);
        merge_sort(char_base + s * el_size, num - s, el_size, compar);
        merge(char_base, num, el_size, s, compar);
    }
}
