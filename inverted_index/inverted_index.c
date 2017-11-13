#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "index.h"
#include "ngrams.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "%s", "Error: <Usage: 1 input file>\n");
        exit(EXIT_FAILURE);
    }

    clock_t start = clock();

    IndexArray indices = DefaultIndexArray;
    int doc_id         = 1;
    createInvertedIndex(argv[1], doc_id, &indices);

    BigramArray bigrams   = DefaultBigramArray;
    TrigramArray trigrams = DefaultTrigramArray;
    createNGrams(argv[1], &indices, &bigrams, &trigrams);

    outputInvertedIndex("words.dat", &indices, -1);
    outputBigrams("bigrams.dat", &bigrams, -1);
    outputTrigrams("trigrams.dat", &trigrams, -1);

    destroyIndexArray(&indices);

    clock_t diff = clock() - start;
    int msec     = diff * 1000 / CLOCKS_PER_SEC;
    printf("Time taken %d seconds %d milliseconds\n", msec / 1000, msec % 1000);
    return 0;
}
