#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "comm.h"
#include "index.h"

const IndexedWord DefaultIndexedWord = {NULL, NULL, 0, 0};
const IndexArray DefaultIndexArray   = {NULL, 0, 0};

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
 * \brief This function creates inverted index from a doc,
 *        and put it into index array \p indices.
 *
 * \param fname file name of document to be indexed
 * \param doc_id the documentation id
 * \param indices the index array
 *
 * \return void
 */
void createInvertedIndex(const char *fname, int doc_id, IndexArray *indices) {

    FILE *fp = fopen(fname, "r");
    if (fp == NULL) {
        fprintf(stderr, "%s", "Error: <could not open file>\n");
        exit(EXIT_FAILURE);
    }
    int line_id = 0;
    char *line  = (char *)malloc(MAX_LINE_LEN * sizeof(char));

    while (NULL != fgets(line, MAX_LINE_LEN, fp)) {
        ++line_id;
        // printf("%i %s", line_id, line);
        char **words = NULL;
        int n_words  = parseLine(line, &words);
        for (int i = 0; i < n_words; ++i) {
            addWordToIndexArray(words[i], doc_id, line_id, i + 1, indices);
        }
    }

    // sort index
    {
        IndexedWord *words = indices->words;
        qsort(words,
              indices->size,
              sizeof(*words),
              compareIndexedWordsByOccurence);
    }

    free(line);
    fclose(fp);
    return;
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
void addWordToIndexArray(const char *line,
                         int doc_id,
                         int line_id,
                         int word_id,
                         IndexArray *indices) {
    int word_found = 0;
    for (int i = 0; i < indices->size; ++i) {
        IndexedWord *word = &indices->words[i];
        // if we already have the word
        word_found = !strcmp(word->chars, line);
        if (word_found) {
            addOccurToIndexedWord(doc_id, line_id, word_id, word);
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
            // printf("IndexArray: memory reallocated\n");
        }
    }

    int end = indices->size;
    // indices->words[end].chars = (char *)malloc(MAX_WORD_LEN * sizeof(char));
    indices->words[end].chars = createNewWord();
    addOccurToIndexedWord(doc_id, line_id, word_id, indices->words + end);
    strcpy(indices->words[end].chars, line);
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
void addOccurToIndexedWord(int doc_id,
                           int line_id,
                           int word_id,
                           IndexedWord *word) {
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
            // printf("WordOccurence: memory reallocated\n");
        }
    }

    Occurence *end = word->occurs + word->n_occurs;
    end->doc_id    = doc_id;
    end->line_id   = line_id;
    end->word_id   = word_id;

    ++(word->n_occurs);
    return;
}

/*!
 * \brief Compare two indexed words by their occurence.
 *
 * To stabilize, if the occurences are the same, we compare
 * them by their address.
 *
 * \param a 1st word
 * \param b 2nd word
 *
 * \return a.n_occurs - b.n_occurs
 */
int compareIndexedWordsByOccurence(const void *a, const void *b) {
    const IndexedWord *w1 = (IndexedWord *)a;
    const IndexedWord *w2 = (IndexedWord *)b;

    int diff = w2->n_occurs - w1->n_occurs;
    // return w2->n_occurs - w1->n_occurs;
    return diff == 0 ? (int)(w1 - w2) : diff;
}

/*!
 * \brief output the top \p top_n words with most occurence and its inverted
 * index info.
 *
 * \param fname output file name
 * \param arr trigram array
 * \param top_n number of grams to be output
 * \return void
 */
void outputInvertedIndex(const char *fname,
                         const IndexArray *indices,
                         int top_n) {

    assert(top_n <= indices->size);
    FILE *fp = fopen(fname, "w");
    if (fp == NULL) {
        fprintf(stderr, "%s", "Error: <could not open file>\n");
        exit(EXIT_FAILURE);
    }

    int n_output      = top_n < 0 ? indices->size : top_n;
    int n_total_words = 0;

    for (int i = 0; i < indices->size; ++i) {
        IndexedWord *word = indices->words + i;
        n_total_words += word->n_occurs;
    }
    fprintf(fp, "TOTAL WORDS: %i\n", n_total_words);
    fprintf(fp, "UNIQUE WORDS: %i\n", indices->size);
    for (int i = 0; i < n_output; ++i) {
        IndexedWord *word = indices->words + i;
        n_total_words += word->n_occurs;
        fprintf(fp, "%i %s\n", word->n_occurs, word->chars);
    }

    fclose(fp);

    return;
}
