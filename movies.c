#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct movie 
{
  char *title;
  int *release_year;
  char *languages[5];     /* an array */
  float *rating;
}

struct movie *createMovie(char *currLine)
{
    struct movie *currMovie = malloc(sizeof(struct movie));
    char *saveptr;

    // assign title
    char *token = strtok_r(currLine, ",", &saveptr);  //split csv by entries
    currMovie->title = calloc(strlen(token)+1, sizeof(char));
    strcpy(currMovie->title, token);

    //assign release_year
    token = strtok_r(NULL, ",", &saveptr); 
    currMovie->release_year = calloc(strlen(token) + 1, sizeof(char));
    strcpy(currMovie->release_year, token);

    //assign languages
    token = strtok_r(NULL, ",", &saveptr); 
    currMovie->languages = calloc(strlen(token) + 1, sizeof(char));
    strcpy(currMovie->languages, token);

    //assign rating
    token = strtok_r(NULL, ",", &saveptr); 
    currMovie->rating = calloc(strlen(token) + 1, sizeof(char));
    strcpy(currMovie->rating, token);
    
}

struct movie *processFile(char *filePath)
{
    FILE *movieFile = fopen(filePath, "r");

    char *currLine = NULL;
    size_t len = 0;
    ssize_t nread;    //signed size_t
    char *token;

    struct movie *head = NULL;
    struct movie *tail = NULL;

    while ((nread = getline(&currLine, &len, movieFile)) != -1)
    {
        struct movie *newNode = createMovie(currLine);

        if (head == NULL)  //start linked list if empty
        {
            head = newNode;
            tail = newNode;
        }
        else
        {
            tail->next = newNode;
            tail = newNode;
        }
    }
    free(currLine);
    fclose(movieFile);
    return head;
}

void printMovie(struct movie *aMovie)
{
    printf("%s, %d, %s, %f\n", 
            aMovie->title, 
            aMovie->release_year,
            aMovie->languages,
            aMovie->rating)
}

void printMovieList(struct movie *list)
{
    while (list != NULL)
    {
        printMovie(list);
        list = list->next;
    }
}

int main(char *argv[])
{
    struct movie *list = processFile(argv[1]);
    printMovieList(list);
    return EXIT_SUCCESS;
}