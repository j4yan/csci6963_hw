
#ifndef NGRAMS_H
#define NGRAMS_H

#include <stdlib.h>

// #include "index.h"

// TODO: Actually we can combine all the words in a ngram
// into a single one, then IndexedWord, Bigram and Trigram 
// can be unified, and we can get rid of many lines of code.
// 
// forward declaration
struct _IndexArray;
typedef struct _IndexArray IndexArray;
struct _Occurence;
typedef struct _Occurence Occurence;

/*! \struct Bigram
 * \brief Stores two words and the number of occurence
 */
typedef struct _Bigram {
    char *word1;
    char *word2;
    int n_occurs;
    int max_occurs;
    Occurence *occurs;
} Bigram;

/*! \struct Trigram
 * \brief Stores two words and the number of occurence
 */
typedef struct _Trigram {
    char *word1;
    char *word2;
    char *word3;
    int n_occurs;
    int max_occurs;
    Occurence *occurs;
} Trigram;

/*! \struct BigramArray
 * \brief A dynamic array storing bigrams
 */
typedef struct _BigramArray {
    Bigram *grams;
    int size;
    int max_size;
} BigramArray;

/*! \struct TrigramArray
 * \brief A dynamic array storing trigrams
 */
typedef struct _TrigramArray {
    Trigram *grams;
    int size;
    int max_size;
} TrigramArray;

extern const Bigram DefaultBigram;
extern const Trigram DefaultTrigram;
extern const BigramArray DefaultBigramArray;
extern const TrigramArray DefaultTrigramArray;

void addOccurToBigram(int doc_id, int line_id, Bigram *gram);
void addOccurToTrigram(int doc_id, int line_id, Trigram *gram);
void addBigram(const char *word1,
               const char *word2,
               int doc_id,
               int line_id,
               BigramArray *bigrams);
void addTrigram(const char *word1,
                const char *word2,
                const char *word3,
                int doc_id,
                int line_id,
                TrigramArray *trigrams);
void createNGrams(const char *fname,
                  int doc_id,
                  const IndexArray *indices,
                  BigramArray *biarr,
                  TrigramArray *triarr);

void destroyBigramArray(BigramArray* big);
void destroyTrigramArray(TrigramArray* trig);

int skipWordInNgram(const char *word, const IndexArray *indices, int top);
void outputBigrams(const char *fname, const BigramArray *arr, int top_n);
void outputTrigrams(const char *fname, const TrigramArray *arr, int top_n);

#endif
