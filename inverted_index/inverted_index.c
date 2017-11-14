#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "comm.h"
#include "index.h"
#include "merge_sort.h"
#include "ngrams.h"

void outputStatistics(int n_docs,
                      const IndexArray *idx,
                      const BigramArray *biarr,
                      const TrigramArray *triarr);

void writeInvertedIndex(const char *fname,
                        int n_docs,
                        const IndexArray *idx,
                        const BigramArray *big,
                        const TrigramArray *trig);
struct _InvertedIndex;
typedef struct _InvertedIndex IndexedIndex;
int compareInvertedIndexByAlpha(const void *a, const void *b);

#ifndef USE_SUBMITTY_MAIN
int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "%s", "Error: <Usage: 2 input files>\n");
        exit(EXIT_FAILURE);
    }

    FILE *fp = fopen(argv[1], "r");
    if (fp == NULL) {
        fprintf(stderr, "%s", "Error: <could not open file>\n");
        exit(EXIT_FAILURE);
    }

    // first count the number of lines
    char *line  = createNewLine();
    int n_files = 0;
    while (NULL != fgets(line, MAX_LINE_LEN, fp)) {
        int len = strlen(line);
        if (len == 0) {
            continue;
        }
        ++n_files;
    }
    rewind(fp);

    char **fnames = (char **)malloc(n_files * sizeof(char *));
    for (int i = 0; i < n_files; ++i) {
        fnames[i] = createNewWord();
    }

    // clock_t start = clock();

    int doc_id = 0;
    while (NULL != fgets(fnames[doc_id], MAX_LINE_LEN, fp)) {
        int len = strlen(fnames[doc_id]);
        if (len == 0) {
            continue;
        }
        if (fnames[doc_id][len - 1] == '\n') {
            fnames[doc_id][len - 1] = '\0';
        }
        ++doc_id;
    }

    IndexArray indices = DefaultIndexArray;
    for (int i = 0; i < n_files; ++i) {
        createInvertedIndex(fnames[i], i + 1, &indices);
    }

    IndexedWord *words = indices.words;
    merge_sort(words,
               indices.size,
               sizeof(*words),
               compareIndexedWordsByOccurence);

    BigramArray bigrams   = DefaultBigramArray;
    TrigramArray trigrams = DefaultTrigramArray;
    for (int i = 0; i < n_files; ++i) {
        createNGrams(fnames[i], i + 1, &indices, &bigrams, &trigrams);
    }

    // sort bigrams and trigrams
    Bigram *bigrams_ptr = bigrams.grams;
    merge_sort(bigrams_ptr,
               bigrams.size,
               sizeof(*bigrams_ptr),
               compareBigramsByOccurence);

    Trigram *trigrams_ptr = trigrams.grams;
    merge_sort(trigrams_ptr,
               trigrams.size,
               sizeof(*trigrams_ptr),
               compareTrigramsByOccurence);

    outputStatistics(n_files, &indices, &bigrams, &trigrams);
    writeInvertedIndex(argv[2], n_files, &indices, &bigrams, &trigrams);
    destroyIndexArray(&indices);
    destroyBigramArray(&bigrams);
    destroyTrigramArray(&trigrams);

    return 0;
}

#endif

/*! \brief Print statistics, including a summary, top 50 words
 *         top 20 bigrams and top 10 trigrams.
 * 
 * \param n_docs number of documents
 * \param idx array of indexed words
 * \param biarr array of bigrams
 * \param triarr array of trigrams
 */
void outputStatistics(int n_docs,
                      const IndexArray *idx,
                      const BigramArray *biarr,
                      const TrigramArray *triarr) {

    int n_words = 0;

    for (int i = 0; i < idx->size; ++i) {
        IndexedWord *word = idx->words + i;
        n_words += word->n_occurs;
    }
    int n_bigrams = 0;
    for (int i = 0; i < biarr->size; ++i) {
        Bigram *gram = biarr->grams + i;
        n_bigrams += gram->n_occurs;
    }

    int n_trigrams = 0;

    for (int i = 0; i < triarr->size; ++i) {
        Trigram *gram = triarr->grams + i;
        n_trigrams += gram->n_occurs;
    }

    // summary
    printf("%s%i\n", "Total number of documents: ", n_docs);
    printf("%s%i\n", "Total number of words: ", n_words);
    printf("%s%i\n", "Total number of unique words: ", idx->size);
    printf("%s%i\n", "Total number of interesting bigrams: ", n_bigrams);
    printf("%s%i\n",
           "Total number of unique interesting bigrams: ",
           biarr->size);
    printf("%s%i\n", "Total number of interesting trigrams: ", n_trigrams);
    printf("%s%i\n",
           "Total number of unique interesting trigrams: ",
           triarr->size);

    // words
    const int n_out_words = 50;
    printf("\n%s%i%s\n", "Top ", n_out_words, " words:");
    for (int i = 0; i < n_out_words; ++i) {
        IndexedWord *word = idx->words + i;
        printf("%i %s\n", word->n_occurs, word->chars);
    }

    const int n_out_bigrams = 20;
    printf("\n%s%i%s\n", "Top ", n_out_bigrams, " interesting bigrams:");
    for (int i = 0; i < n_out_bigrams; ++i) {
        Bigram *gram = biarr->grams + i;
        printf("%i %s %s\n", gram->n_occurs, gram->word1, gram->word2);
    }

    const int n_out_trigrams = 10;
    printf("\n%s%i%s\n", "Top ", n_out_trigrams, " interesting trigrams:");
    for (int i = 0; i < n_out_trigrams; ++i) {
        Trigram *gram = triarr->grams + i;
        printf("%i %s %s %s\n",
               gram->n_occurs,
               gram->word1,
               gram->word2,
               gram->word3);
    }
    fflush(stdout);
    return;
}

typedef struct _InvertedIndex {
    char *chars;
    int *occurs;
} InvertedIndex;

int compareInvertedIndexByAlpha(const void *a, const void *b) {

    const InvertedIndex *w1 = (IndexedIndex *)a;
    const InvertedIndex *w2 = (IndexedIndex *)b;

    return strcmp(w1->chars, w2->chars);
}

/*! \brief Write inverted index into a file.
 * 
 * \param fname output file name
 * \param n_docs number of documents
 * \param idx array of indexed words
 * \param big array of bigrams
 * \param trig array of trigrams
 */
void writeInvertedIndex(const char *fname,
                        int n_docs,
                        const IndexArray *idx,
                        const BigramArray *big,
                        const TrigramArray *trig) {

    FILE *fp = fopen(fname, "w");
    if (fp == NULL) {
        fprintf(stderr, "%s", "Error: <could not open file>\n");
        exit(EXIT_FAILURE);
    }

    // the possible maximum length of a trigram
    const int LEN      = 3 * MAX_WORD_LEN + 2;
    // total grams
    int n_tot          = idx->size + big->size + trig->size;
    InvertedIndex *all = (InvertedIndex *)malloc(n_tot * sizeof(InvertedIndex));
    for (int i = 0; i < n_tot; ++i) {
        all[i].chars = (char *)malloc(LEN * sizeof(char));
        for (int j = 0; j < LEN; ++j) {
            all[i].chars[j] = '\0';
        }
        all[i].occurs = (int *)malloc(n_docs * sizeof(int));
        for (int j = 0; j < n_docs; ++j) {
            all[i].occurs[j] = 0;
        }
    }

    int cnt = 0;
    for (int i = 0; i < idx->size; ++i) {
        IndexedWord *word = idx->words + i;
        strcpy(all[cnt].chars, word->chars);

        for (int j = 0; j < word->n_occurs; ++j) {
            int doc_id = word->occurs[j].doc_id - 1;
            ++all[cnt].occurs[doc_id];
        }
        ++cnt;
    }

    for (int i = 0; i < big->size; ++i) {
        Bigram *gram = big->grams + i;
        int offset   = 0;
        strcpy(all[cnt].chars + offset, gram->word1);
        offset += strlen(gram->word1);
        all[cnt].chars[offset] = ' ';
        offset += 1;
        strcpy(all[cnt].chars + offset, gram->word2);

        for (int j = 0; j < gram->n_occurs; ++j) {
            int doc_id = gram->occurs[j].doc_id - 1;
            ++all[cnt].occurs[doc_id];
        }
        ++cnt;
    }

    for (int i = 0; i < trig->size; ++i) {
        Trigram *gram = trig->grams + i;
        int offset    = 0;
        strcpy(all[cnt].chars + offset, gram->word1);
        offset += strlen(gram->word1);
        all[cnt].chars[offset] = ' ';
        offset += 1;
        strcpy(all[cnt].chars + offset, gram->word2);
        offset += strlen(gram->word2);
        all[cnt].chars[offset] = ' ';
        offset += 1;
        strcpy(all[cnt].chars + offset, gram->word3);

        for (int j = 0; j < gram->n_occurs; ++j) {
            int doc_id = gram->occurs[j].doc_id - 1;
            ++all[cnt].occurs[doc_id];
        }
        ++cnt;
    }

    merge_sort(all, n_tot, sizeof(*all), compareInvertedIndexByAlpha);

    for (int i = 0; i < n_tot; ++i) {
        // count the number of docs in which this word occurs;
        int n_occurs = 0;
        for (int j = 0; j < n_docs; ++j) {
            if (all[i].occurs[j]) {
                ++n_occurs;
            }
        }
        if (!n_occurs) {
            printf("there is sth wrong\n");
        }

        fprintf(fp, "%s:", all[i].chars);
        int cnt = 0;
        for (int j = 0; j < n_docs; j++) {
            if (all[i].occurs[j] == 0) {
                continue;
            }
            char delimiter = ',';
            if (cnt == n_occurs - 1) {
                delimiter = '\n';
            }
            fprintf(fp, " [ %i, %i ]%c", j+1, all[i].occurs[j], delimiter);
            ++cnt;
        }
    }
    fclose(fp);
    
    for(int i=0; i<n_tot; ++i) {
        free(all[i].chars);
        free(all[i].occurs);
    }
    free(all);
    return;
}
