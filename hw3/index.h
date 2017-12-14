
#ifndef INDEX_H
#define INDEX_H

#include <stdlib.h>

// typedef struct _Occurence Occurence;
// typedef struct _IndexedWord IndexedWord;
// typedef struct _IndexArray IndexArray;

/*!
 * \brief Occurence info of an indexed word
 */
typedef struct _Occurence {
    int doc_id;  ///< the document id
    int line_id; ///< the line position of word occurence
    int word_id; ///<  the position of word in line
} Occurence;

/*!
 * \brief indexed word
 */
typedef struct _IndexedWord {
    char *chars;
    Occurence *occurs; ///< array storing occurence  info
    int n_occurs;      ///< the actual size of \p words
    int max_occurs;    ///< the capability of \p words
} IndexedWord;

/*! \struct IndexArray
 * \brief Dynamic array storing indexed words.
 */
typedef struct _IndexArray {
    IndexedWord *words;
    int size;
    int max_size;
} IndexArray;

extern const IndexedWord DefaultIndexedWord;
extern const IndexArray DefaultIndexArray;

void initIndexArray(IndexArray *indices);
void addOccurToIndexedWord(int doc_id,
                           int line_id,
                           int word_id,
                           IndexedWord *word);
void addWordToIndexArray(const char *word,
                         int doc_id,
                         int line_id,
                         int word_id,
                         IndexArray *arr);
void createInvertedIndex(const char *fname, int doc_id, IndexArray *indices);
int compareIndexedWordsByOccurence(const void *a, const void *b);
int compareBigramsByOccurence(const void *a, const void *b);
int compareTrigramsByOccurence(const void *a, const void *b);
void outputInvertedIndex(const char *fname,
                         const IndexArray *indices,
                         int top_n);
void destroyIndexArray(IndexArray *indices);

#endif
