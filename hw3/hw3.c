#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "comm.h"
#include "index.h"
#include "merge_sort.h"
#include "ngrams.h"

struct _InvertedIndex;
typedef struct _InvertedIndex IndexedIndex;
int readSurroundingLinesIntoWords(const char *fname,
                                  int line_id,
                                  int npre,
                                  int npost,
                                  char ***words_ptr);

#ifndef USE_SUBMITTY_MAIN
int main(int argc, char **argv)
{
    // if (argc != 3) {
    //     fprintf(stderr, "%s", "Error: <Usage: 2 input files>\n");
    //     exit(EXIT_FAILURE);
    // }

    FILE *fp = fopen("documents/contents.txt", "r");
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
    printf("number of files = %i\n", n_files);
    rewind(fp);

    char **fnames        = (char **)malloc(n_files * sizeof(char *));
    char **index_fnames  = (char **)malloc(n_files * sizeof(char *));
    const int len_prefix = 12;
    for (int i = 0; i < n_files; ++i) {
        fnames[i]       = createNewWord();
        index_fnames[i] = createNewWord();
        strncpy(fnames[i], "./documents/", len_prefix);
        strncpy(index_fnames[i], "./documents/", len_prefix);
    }

    // clock_t start = clock();

    int *doc_id = (int *)malloc(n_files * sizeof(int *));
    for (int i = 0; i < n_files; ++i) {
        int flag = fscanf(fp, "%i %s", doc_id + i, fnames[i] + len_prefix);
        if (flag != 2) {
            printf("%i\n", flag);
            fprintf(stderr, "%s", "Error: <wrong format in contents.txt>\n");
            exit(EXIT_FAILURE);
        }
        int len = strlen(fnames[i]);
        if (len == 0) {
            continue;
        }
        if (fnames[i][len - 1] == '\n') {
            fnames[i][len - 1] = '\0';
        }
        for (int j = len_prefix; j < len; ++j) {
            index_fnames[i][j] = fnames[i][j];
            if (strncmp(fnames[i] + j, ".txt", 4) == 0) {
                strncpy(index_fnames[i] + j, ".index", 6);
                break;
            }
        }

        printf("%i %s %s\n", doc_id[i], fnames[i], index_fnames[i]);
    }

    IndexArray indices = DefaultIndexArray;
    for (int i = 0; i < n_files; ++i) {
        createInvertedIndex(fnames[i], i + 1, &indices);
    }

    // IndexedWord *words = indices.words;
    merge_sort(indices.words,
               indices.size,
               sizeof(*(indices.words)),
               compareIndexedWordsByOccurence);

    BigramArray bigrams   = DefaultBigramArray;
    TrigramArray trigrams = DefaultTrigramArray;
    for (int i = 0; i < n_files; ++i) {
        createNGrams(fnames[i], i + 1, &indices, &bigrams, &trigrams);
    }

    // sort bigrams and trigrams
    // Bigram *bigrams_ptr = bigrams.grams;
    merge_sort(bigrams.grams,
               bigrams.size,
               sizeof(*(bigrams.grams)),
               compareBigramsByOccurence);

    // Trigram *trigrams_ptr = trigrams.grams;
    merge_sort(trigrams.grams,
               trigrams.size,
               sizeof(*(trigrams.grams)),
               compareTrigramsByOccurence);

    int *occuring_doc_id         = (int *)malloc(n_files * sizeof(int));
    int *occuring_line_id        = (int *)malloc(n_files * sizeof(int));
    const int NPRE               = 10;
    const int NPOST              = 10;
    const int STENCIL_LINES      = NPRE + NPOST + 1;
    const int MAX_WORDS_PER_LINE = 20;
    const int STENCIL_WORDS      = MAX_WORDS_PER_LINE * STENCIL_LINES;
    char **sur_words = (char **)malloc(STENCIL_WORDS * sizeof(char *));
    for (int i = 0; i < STENCIL_WORDS; ++i) {
        sur_words[i] = createNewWord();
    }

    // start searching words
    for (;;) {
        printf("SEARCH: ");
        char *input = createNewWord();
        fgets(input, MAX_WORD_LEN, stdin);
        if (strcmp(input, "EXIT\n") == 0) {
            return EXIT_SUCCESS;
        }
        printf("%s", input);

        char **query_words = NULL;
        int n_query_words  = parseLine(input, &query_words);
        // we only deal with up to 5 words
        const int max_words = 5;
        if (n_query_words > max_words) {
            n_query_words = max_words;
        }
        int *n_occurs = (int *)malloc(n_query_words * sizeof(int));
        // words occurence
        for (int i = 0; i < n_query_words; ++i) {
            n_occurs[i] = 0;
            int n_doc   = 0;
            for (int j = 0; j < indices.size; ++j) {
                if (strcmp(indices.words[j].chars, query_words[i])) {
                    continue;
                }
                int n_cur_occurs = indices.words[j].n_occurs;
                n_occurs[i] += n_cur_occurs;

                for (int l = 0; l < n_cur_occurs; ++l) {
                    int found = 0;
                    for (int k = 0; k < n_doc; ++k) {
                        if (occuring_doc_id[k] ==
                            indices.words[j].occurs[l].doc_id) {
                            found = 1;
                            break;
                        }
                    }
                    if (n_doc == n_files) {
                        break;
                    }
                    if (!found) {
                        occuring_doc_id[n_doc] =
                                indices.words[i].occurs[l].doc_id;
                        occuring_line_id[n_doc] =
                                indices.words[i].occurs[l].line_id;
                        ++n_doc;
                    }
                }
                break;
            }

            printf("Number of \"%s\" occurences is %i",
                   query_words[i],
                   n_occurs[i]);
            if (n_occurs[i] > 0) {
                printf(" [docID");
                for (int j = 0; j < n_doc; ++j) {
                    printf(" %i", occuring_doc_id[j]);
                }
                printf("]");
            }

            printf("\n");
        }
        for (int i = 0; i < n_query_words; ++i) {
            // int n_occurs       = 0;
            // int n_doc          = 0;
            // for (int i = 0; i < n_files; ++i) {
            //     occuring_doc_id[i] = -1;
            // }
            // for (int j = 0; j < indices.size; ++j) {
            //     if (strcmp(indices.words[j].chars, query_words[i])) {
            //         continue;
            //     }
            //     int n_cur_occurs = indices.words[j].n_occurs;
            //     n_occurs += n_cur_occurs;
            //     target_doc_id  = indices.words[j].occurs[0].doc_id;
            //     target_line_id = indices.words[j].occurs[0].line_id;
            //
            //     for (int l = 0; l < n_cur_occurs; ++l) {
            //         int found = 0;
            //         for (int k = 0; k < n_doc; ++k) {
            //             if (occuring_doc_id[k] ==
            //                 indices.words[j].occurs[l].doc_id) {
            //                 found = 1;
            //                 break;
            //             }
            //         }
            //         if (n_doc == n_files) {
            //             break;
            //         }
            //         if (!found) {
            //             occuring_doc_id[n_doc] =
            //                     indices.words[i].occurs[l].doc_id;
            //             ++n_doc;
            //         }
            //     }
            //     break;
            // }

            if (n_occurs[i] == 0) {
                continue;
            }

            int n_stencil_words = readSurroundingLinesIntoWords(
                    fnames[occuring_doc_id[0] - 1],
                    occuring_line_id[0],
                    NPRE,
                    NPOST,
                    &sur_words);
            // for (int i=0; i<n_stencil_words; ++i) {
            //     printf("%s ", sur_words[i]);
            // }
            // printf("\n");
            // loop over stencil words, find out the index of query word.
            int query_word_id    = -1;  // index of pass-the-end word in snippet
            int word_id0         = -1;  // index of first word in snippet
            int word_id1         = -1;  // index of last word in snippet
            int found_query_word = 0;
            char *tmp_word       = createNewWord();
            for (int j = 0; j < n_stencil_words; ++j) {
                char **sub_words = NULL;
                strcpy(tmp_word, sur_words[j]);
                // int n_sub_words  = parseLine(sur_words[j], &sub_words);
                int n_sub_words = parseLine(tmp_word, &sub_words);
                for (int k = 0; k < n_sub_words; ++k) {
                    if (!strcmp(sub_words[k], query_words[i])) {
                        // since we added into sur_words NPRE lines before and
                        // NPOST lines after the occuring line, these 2 indices
                        // propably are not negative.
                        query_word_id    = j;
                        found_query_word = 1;
                        break;
                    }
                }
                if (found_query_word) {
                    break;
                }
            }
            word_id0 = query_word_id;
            word_id1 = query_word_id;
            // find word_id0 and word_id1
            int n_tot_sub_words = 0;
            for (int j = 0; j < 100; j++) {
                char **sub_words = NULL;
                int id           = query_word_id - 1 - j;
                if (id < 0) {
                    break;
                }
                strcpy(tmp_word, sur_words[id]);
                int n_sub_words = parseLine(tmp_word, &sub_words);
                n_tot_sub_words += n_sub_words;
                word_id0 = id;
                if (n_tot_sub_words >= 5) {
                    break;
                }
            }
            n_tot_sub_words = 0;
            for (int j = 0; j < 100; j++) {
                char **sub_words = NULL;
                int id           = query_word_id + j + 1;
                if (id > n_stencil_words - 1) {
                    break;
                }
                strcpy(tmp_word, sur_words[id]);
                int n_sub_words = parseLine(tmp_word, &sub_words);
                n_tot_sub_words += n_sub_words;
                word_id1 += n_sub_words;
                word_id1 = id;
                if (n_tot_sub_words >= 5) {
                    break;
                }
            }
            if (word_id0 < 0) {
                word_id0 = 0;
            }
            if (word_id1 > n_stencil_words) {
                word_id1 = n_stencil_words;
            }
            printf("...");
            for (int j = word_id0; j < word_id1; ++j) {
                printf("%s ", sur_words[j]);
            }
            printf("%s...\n", sur_words[word_id1]);
        }
    }
    free(occuring_doc_id);
    free(line);

    destroyIndexArray(&indices);
    destroyBigramArray(&bigrams);
    destroyTrigramArray(&trigrams);

    for (int i = 0; i < STENCIL_WORDS; ++i) {
        free(sur_words[i]);
    }
    free(sur_words);
    for (int i = 0; i < n_files; ++i) {
        free(fnames[i]);
        free(index_fnames[i]);
    }
    free(fnames);
    free(index_fnames);
    return 0;
}

#endif

int readSurroundingLinesIntoWords(const char *fname,
                                  int line_id,
                                  int npre,
                                  int npost,
                                  char ***words_ptr)
{
    char **words = *words_ptr;
    // print out snippets
    int line_id0 = line_id - npre;
    int line_id1 = line_id + npost;
    if (line_id0 < 1) {
        line_id0 = 1;
    }

    FILE *fp_occ = fopen(fname, "r");
    if (fp_occ == NULL) {
        fprintf(stderr, "%s", "Error: <could not open file>\n");
        exit(EXIT_FAILURE);
    }

    char *cur_line = (char *)malloc(MAX_LINE_LEN * sizeof(char));
    for (int l = 1; l < line_id0; ++l) {
        char *fgets_ptr = fgets(cur_line, MAX_LINE_LEN, fp_occ);
        // char *fgets_ptr = fgets(cur_line, 2, fp_occ);

        if (fgets_ptr == NULL) {
            fprintf(stderr,
                    "%s",
                    "Error: <trying to read line that does not exist>\n");
            exit(EXIT_FAILURE);
        }
    }
    int word_cnt = 0;
    int char_cnt = 0;
    for (int l = line_id0; l <= line_id1; ++l) {
        // char *cur_line  = raw_lines[l - line_id0];
        char *fgets_ptr = fgets(cur_line, MAX_LINE_LEN, fp_occ);
        if (fgets_ptr == NULL) {
            break;
        }
        // printf("%s", cur_line);
        // copy all stencil lines into all_lines
        size_t cur_line_len = strlen(cur_line);
        if (cur_line_len == 0) {
            continue;
        }
        for (size_t k = 0; k < cur_line_len; ++k) {
            if (cur_line[k] == '\n' || isspace(cur_line[k])) {
                if (char_cnt == 0) {
                    continue;
                }
                else {
                    words[word_cnt][char_cnt] = '\0';
                    ++word_cnt;
                    char_cnt = 0;
                }
            }
            else {
                words[word_cnt][char_cnt++] = cur_line[k];
            }
        }
    }

    // for (int i=0; i<word_cnt; ++i) {
    //     printf("%s ", words[i]);
    // }
    // printf("\n");
    fclose(fp_occ);
    free(cur_line);
    return word_cnt;
}
