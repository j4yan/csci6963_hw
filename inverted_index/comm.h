#ifndef COMM_H
#define COMM_H 

#define MAX_LINE_LEN 257
#define MAX_WORD_LEN 30

char *createNewWord();
int parseLine(char *line, char ***words_out_ptr);

#endif
