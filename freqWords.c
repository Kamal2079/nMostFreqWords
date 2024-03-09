#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#define MAXC   32   /* max word length is 28-char, 29-char is sufficient */
#define MAXW  256   /* initial maximum number of words to allocate */

typedef struct {
    char word[MAXC];    /* create a struct named "words_t" to hold individual words */
    size_t ninst;       /* and the number of times they occur */
} words_t;

/*  function prototypes */
void *addword (words_t *words, const char *word, size_t *wc, size_t *maxw);
void *xrealloc (void *ptr, size_t psz, size_t *nelem);

/* qsort compare function. Takes in words_t structs and sorts them alphabetically.
output will be positive is a is larger, negative if b larger, 0 if equal. */
int cmpwrds (const void *a, const void *b)
{
    return strcmp (((words_t *)a)->word, ((words_t *)b)->word);
}

/* another qsort compare function. Takes in words_t structs and sorts by number of
occurences (descending) and alphabetically (ascending) if the number of occurrences are equal.
 */
int cmpinst (const void *a, const void *b)
{
    int ndiff =  (((words_t *)a)->ninst < ((words_t *)b)->ninst) -
                (((words_t *)a)->ninst > ((words_t *)b)->ninst);

    if (ndiff)
        return ndiff;

    return strcmp (((words_t *)a)->word, ((words_t *)b)->word);
}

int main (int argc, char **argv) {

    int c = 0, nc = 0, prev = ' ', total = 0;
    size_t maxw = MAXW, wc = 0, n = 100;
    char buf[MAXC] = "";
    words_t *words = NULL;

    // Get the file path from the first command line argument

    FILE *fp = fopen (argv[1], "r");

    if (!fp) {  /* validate file open for reading */
        fprintf (stderr, "error: file open failed '%s'.\n", argv[1]);
        return 1;
    }

    // Get the value of n from the second command line argument

    if (argc > 2) { /* Check if there are more than 2 command-line arguments, if so collect n value */
    char *p = argv[2];
    char *endptr;
    long tmp = strtol(argv[2], &endptr, 10); /* Convert string to long integer */

    if (endptr != argv[2] && *endptr == '\0' && tmp >= 0 && tmp <= SIZE_MAX) {
        n = (size_t)tmp; /* Conversion successful, assign to 'n' */
    } else {
        fprintf(stderr, "Invalid argument for number of n words: %s\n", argv[2]);
        return 1;
    }
    } // end of if (argc > 2)

    /* allocate memory for the array of words_t structures. perror handles the case where memory allocation fails. */
    if (!(words = calloc (maxw, sizeof *words))) {
        perror ("calloc-words");
        return 1;
    }


    // parses the whole input file. figures out where words start and end. Adds words to the array of words_t accordingly.
    while ((c = fgetc(fp)) != EOF) {        /* read each character in file, until end */
        if (c != '-' && (isspace (c) || ispunct (c))) { /* word-end found */
            if (!isspace (prev) && !ispunct (prev) &&   /* multiple ws/punct */
                !(prev == 's' && nc == 1) &&           /* exclude "'s" */
                !(prev == 'd' && nc == 1)) {            /* exclude "'d" */
                buf[nc] = 0;                            /* nul-terminate */
                words = addword (words, buf, &wc, &maxw);   /* add word */
                nc = 0;     /* reset char count */
            }
        }
        else if (nc < MAXC - 1) {   /* add char to buf */
            buf[nc++] = c;
        }
        else {  /* chars exceed MAXC - 1; storage capability of struct (word too long) */
            fprintf (stderr, "error: characters exceed %d.\n", MAXC);
            return 1;
        }
        prev = c;   /* save previous char */
    } // end of while

    if (!isspace (prev) && !ispunct (prev))     /* handle non-POSIX end */
        words = addword (words, buf, &wc, &maxw);

    if (fp != stdin) fclose (fp);   /* close file if not stdin */

    qsort (words, wc, sizeof *words, cmpinst);  /* sort words by frequency */

    printf ("'%s' contained '%zu' words.\n\n",  /* output total No. words */
            fp == stdin ? "stdin" : argv[1], wc);

    /* output n words (or all words in descending order if n not given) */
    for (size_t i = 0; i < (n != 0 ? n : wc); i++) {
        printf ("  %-28s    %5zu\n", words[i].word, words[i].ninst);
        total += words[i].ninst;
    }
    printf ("%33s------\n%34s%5d\n", " ", "Total: ", total);

    free (words);

    return 0;
}

/** add word to words, updating pointer to word-count 'wc' and
 *  the maximum words allocated 'maxw' as needed. returns pointer
 *  to words (which must be assigned back in the caller).
 */
void *addword (words_t *words, const char *word, size_t *wc, size_t *maxw)
{
    size_t i;

    for (i = 0; i < *wc; i++)
        if (strcasecmp (words[i].word, word) == 0) { //strcasecmp is case insensitive
            words[i].ninst++;
            return words;
        }

    if (*wc == *maxw)
        words = xrealloc (words, sizeof *words, maxw);

    strcpy (words[*wc].word, word);
    words[(*wc)++].ninst++;

    return words;
}

/** realloc 'ptr' of 'nelem' of 'psz' to 'nelem * 2' of 'psz'.
 *  returns pointer to reallocated block of memory with new
 *  memory initialized to 0/NULL. return must be assigned to
 *  original pointer in caller.
 */
void *xrealloc (void *ptr, size_t psz, size_t *nelem)
{   void *memptr = realloc ((char *)ptr, *nelem * 2 * psz);
    if (!memptr) {
        perror ("realloc(): virtual memory exhausted.");
        exit (EXIT_FAILURE);
    }   /* zero new memory (optional) */
    memset ((char *)memptr + *nelem * psz, 0, *nelem * psz);
    *nelem *= 2;
    return memptr;
}
