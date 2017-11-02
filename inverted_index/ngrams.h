
#ifndef NGRAMS_H
#define NGRAMS_H

#include <stdlib.h>

// #include "index.h"

// forward declaration
struct _IndexArray;
typedef struct _IndexArray IndexArray;

/*! \struct Bigram
 * \brief Stores two words and the number of occurence
 */
typedef struct _Bigram {
    char *word1;
    char *word2;
    int n_occurs;
} Bigram;

/*! \struct Trigram
 * \brief Stores two words and the number of occurence
 */
typedef struct _Trigram {
    char *word1;
    char *word2;
    char *word3;
    int n_occurs;
} Trigram;

/*! \struct BigramArray
 * \brief A dynamic array storing bigrams
 */
typedef struct _BigramArray {
    Bigram *bigrams;
    int size;
    int max_size;
} BigramArray;

/*! \struct TrigramArray
 * \brief A dynamic array storing trigrams
 */
typedef struct _TrigramArray {
    Trigram *trigrams;
    int size;
    int max_size;
} TrigramArray;

extern const Bigram DefaultBigram;
extern const Trigram DefaultTrigram;
extern const BigramArray DefaultBigramArray;
extern const TrigramArray DefaultTrigramArray;

void addBigram(const char *word1, const char *word2, BigramArray *bigrams);
void addTrigram(const char *word1,
                const char *word2,
                const char *word3,
                TrigramArray *trigrams);
void createNGrams(const char *fname,
                  const IndexArray *indices,
                  BigramArray *biarr,
                  TrigramArray *triarr);

int skipWordInNgram(const char *word, const IndexArray *indices, int top);
void outputBigrams(const char *fname, const BigramArray *arr, int top_n);
void outputTrigrams(const char *fname, const TrigramArray *arr, int top_n);

#endif
