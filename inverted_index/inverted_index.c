
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

#define MAX_LINE_LEN 257
#define MAX_WORD_LEN 20

// typedef char Word[MAX_WORD_LEN];
// typedef char Line[MAX_LINE_LEN];
typedef struct _Occurence Occurence;
typedef struct _IndexedWord IndexedWord;
typedef struct _IndexArray IndexArray;

/*!
 * \brief Occurence info of an indexed word
 */
struct _Occurence {
    int doc_id;  ///< the document id
    int line_id; ///< the line position of word occurence
    int word_id; ///<  the position of word in line
};

/*!
 * \brief indexed word
 */
struct _IndexedWord {
    char *chars;
    Occurence *occurs; ///< array storing occurence  info
    int n_occurs;      ///< the actual size of \p words
    int max_occurs;    ///< the capability of \p words
} DefaultIndexedWord = {NULL, NULL, 0, 0};

/*!
 * \brief Dynamic array storing indexed words.
 */
struct _IndexArray {
    IndexedWord *words;
    int size;
    int max_size;
} DefaultIndexArray = {NULL, 0, 0};

void initIndexArray(IndexArray *indices);
void addOccurToIndexedWord(int doc_id, int line_id, IndexedWord *word);
void addWordToIndexArray(const char *word,
                         int doc_id,
                         int line_id,
                         IndexArray *arr);
void destroyIndexArray(IndexArray *indices);
int comparaIndexedWordByOccurence(const void *a, const void *b);

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "%s", "Error: <Usage: 1 input file>\n");
        exit(EXIT_FAILURE);
    }
    char *fname_text = argv[1];

    FILE *fp_text = fopen(fname_text, "r");
    if (fp_text == NULL) {
        fprintf(stderr, "%s", "Error: <could not open file>\n");
        exit(EXIT_FAILURE);
    }

    IndexArray indices = DefaultIndexArray;

    // int doc_id  = 1;
    // int line_id = 1;
    // Word word = "a";
    //
    // int times = 10;
    // for (int i = 0; i < times; ++i) {
    //     word[i] = 'a';
    //     line_id = i + 1;
    //     addWordToIndexArray(word, doc_id, line_id, &indices);
    //     printf("%i words are indexed...\n", i+1);
    // }
    // printf("number of unique words = %i\n", indices.size);

    int doc_id    = 0;
    int line_id   = 0;
    int word_id   = 0;
    const int EOL = '\n';
    char *word    = (char *)malloc(MAX_WORD_LEN * sizeof(char));
    while (1) {
        int flag = fscanf(fp_text, "%s", word);

        if (EOL == flag) {
            line_id += 1;
            word_id = 0;
        } else if (EOF == flag) {
            break;
        }
        addWordToIndexArray(word, doc_id, line_id, &indices);
        ++word_id;
    }
    printf("number of unique words = %i\n", indices.size);

    IndexedWord *words = indices.words;
    qsort(words, indices.size, sizeof(*words), comparaIndexedWordByOccurence);

    for (int i = 0; i < indices.size; ++i) {
        IndexedWord *word = indices.words + i;
        printf("%i %s\n", word->n_occurs, word->chars);
    }

    destroyIndexArray(&indices);
    return 0;
}

/*!
 * \brief Add a word and its info into an inverted index.
 *
 * It's the user's responsibility to guarrantee the array
 * \var iindex is large enough.
 *
 * \param word
 * \param doc_id the id of the doc where \p word is
 * \param line_id the line number
 * \param head current length of \p iindex
 * \param indices the inverted index array to put \p word in
 *
 * \return void
 */
void addWordToIndexArray(const char *word_in,
                         int doc_id,
                         int line_id,
                         IndexArray *indices) {
    int word_found = 0;
    for (int i = 0; i < indices->size; ++i) {
        IndexedWord *word = &indices->words[i];
        // if we already have the word
        word_found = !strcmp(word->chars, word_in);
        if (word_found) {
            addOccurToIndexedWord(doc_id, line_id, word);
            return;
        }
    }

    // if the word is not indexed yet
    if (indices->max_size <= 0) {
        indices->max_size = 1;
        indices->size     = 0;
        indices->words =
                (IndexedWord *)malloc(indices->max_size * sizeof(IndexedWord));
        indices->words[0] = DefaultIndexedWord;
    } else if (indices->size >= indices->max_size - 1) {
        // check the capability of indices
        indices->max_size *= 2;
        IndexedWord *ptr = realloc(indices->words,
                                   indices->max_size * sizeof(IndexedWord));
        if (ptr == NULL) {
            fprintf(stderr, "%s", "Error: <unable to reallocate memory>\n");
            exit(EXIT_FAILURE);
        } else {
            indices->words = ptr;
            for (int i = indices->size; i < indices->max_size; ++i) {
                indices->words[i] = DefaultIndexedWord;
            }
        }
    }

    int end                   = indices->size;
    indices->words[end].chars = (char *)malloc(MAX_WORD_LEN * sizeof(char));
    addOccurToIndexedWord(doc_id, line_id, indices->words + end);
    strcpy(indices->words[end].chars, word_in);
    ++(indices->size);

    return;
}

/*!
 * \brief Add Occurence to indexed word.
 *
 * \param doc_id id of occurence doc
 * \param line_id id of occurence line
 * \param word indexed word
 */
void addOccurToIndexedWord(int doc_id, int line_id, IndexedWord *word) {
    if (word->max_occurs <= 0) {
        word->max_occurs = 1;
        word->n_occurs   = 0;
        word->occurs =
                (Occurence *)malloc(word->max_occurs * sizeof(Occurence));
    } else if (word->n_occurs >= word->max_occurs - 1) {
        // this deal with the initial case where max_docs = 0
        word->max_occurs *= 2;

        Occurence *ptr =
                realloc(word->occurs, word->max_occurs * sizeof(Occurence));

        if (ptr == NULL) {
            fprintf(stderr, "%s", "Error: <unable to reallocate memory>\n");
            exit(EXIT_FAILURE);
        } else {
            word->occurs = ptr;
        }
    }

    Occurence *end = word->occurs + word->n_occurs;
    end->doc_id    = doc_id;
    end->line_id   = line_id;

    ++(word->n_occurs);
    return;
}

/*!
 * \brief free the memory allocated in indices, both directly and indirectly.
 *
 * \param indices
 *
 * \return void
 */
void destroyIndexArray(IndexArray *indices) {
    for (int i = 0; i < indices->size; ++i) {
        free(indices->words[i].occurs);
    }

    free(indices->words);
    return;
}

/*!
 * \brief Compare two indexed words by their occurence.
 *
 * \param a 1st word
 * \param b 2nd word
 *
 * \return a.n_occurs - b.n_occurs
 */
int comparaIndexedWordByOccurence(const void *a, const void *b) {
    const IndexedWord *w1 = (IndexedWord *)a;
    const IndexedWord *w2 = (IndexedWord *)b;

    return w2->n_occurs - w1->n_occurs;
}
