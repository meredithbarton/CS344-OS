#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

struct movie 
{
  char *title;
  int release_year;
  char *languages[5];  
  float rating;

  struct movie *next;
};

struct movie *createMovie(char *currLine)
{
    struct movie *currMovie = malloc(sizeof(struct movie));

    char *saveptr;

    // assign title
    char *token = strtok_r(currLine, ",", &saveptr);  
    currMovie->title = calloc(strlen(token)+1, sizeof(char));
    strcpy(currMovie->title, token);

    //assign release_year
    token = strtok_r(NULL, ",", &saveptr); 
    currMovie->release_year = atoi(token); 

    //assign languages
    token = strtok_r(NULL, ",", &saveptr); 
    // use the same tokenizing technique to process each language into an array
    char *saveptr_subtoken;
    char *subtoken = strtok_r(token, ";", &saveptr_subtoken); 
    int i = 0;
    int end_reached = 0;
    while (end_reached == 0)
    {
        if (subtoken[0] == '[')
        {   
            memmove(subtoken, subtoken+1, strlen(subtoken));     // remove opening bracket
        }
        else
        {
            subtoken = strtok_r(NULL, ";", &saveptr_subtoken);
        }
        int size = strlen(subtoken);
        if (subtoken[size-1] == ']')        // remove closing bracket and signal that there are no more languages 
        {
            subtoken[size-1] = '\0';
            end_reached = 1;
        }
        currMovie->languages[i] = calloc(strlen(subtoken), sizeof(char));
        strcpy(currMovie->languages[i], subtoken);
        i++;
    }
    
    // set any remaining array elements to NULL
    if (i != 5)
    {
        for (i; i < 5; i++)
        {
            currMovie->languages[i] = NULL;
        }
    }

    //assign rating
    token = strtok_r(NULL, ",", &saveptr); 
    currMovie->rating = atof(token); 

    currMovie->next = NULL;

    return currMovie;
}


struct movie *processFile(char *filePath, int *row_count)
/* creates linked list of all movies in file */
{
    FILE *movieFile = fopen(filePath, "r");

    /* init linked list */
    char *currLine = NULL;
    size_t len = 0;
    ssize_t nread;   
    char *token;

    struct movie *head = NULL;
    struct movie *tail = NULL;

    int i = 0;

    while ((nread = getline(&currLine, &len, movieFile)) != -1)
    {
        if (i == 0) //skips headers
        {
            i += 1;
            continue;
        }
        struct movie *newNode = createMovie(currLine);
        ++*(row_count);

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

void findYear(int year, struct movie *list)
/* Function for option 1. Prints all movie titles listed from desired year*/
{
    int movie_exists = 0;
    while (list != NULL)
    {
        if (list->release_year == year)
        {
            printf("%s\n", list->title);
            movie_exists = 1;
        }
        list = list->next;
    }
    if (movie_exists == 0)
    {
        printf("There are no listed movies from %d\n", year);
    }
}

void reportMaxRating(struct movie *list, int row_count, int pos)
{
    for (int p = 0; p < pos; p++)
    {
        list=list->next;
    }
    printf("%d %f %s\n", list->release_year, list->rating, list->title);
}

int searchMaxRatingInYear(struct movie *list, int check_year)
{
    int pos = 0;
    int pos_max = 0;
    while (list != NULL)
    {
        float max_rating = 0;
        if (list->release_year == check_year)
        {
            if (list->rating > max_rating)
            {
                max_rating = list->rating;
                pos_max = pos;
            }
        }
        list = list->next;
        pos++;
    }
    return pos_max;
}

void findHighRatings(struct movie *list, int row_count)
/* Function for option 3. Searches through movies for unchecked year, then checks each 
    occurance of year for the max rating in the subset*/
{
    int all_years_found = 0;  //bool to denote whether all years have been accounted for
    int years[row_count];  //one spot for each possible year
    int years_size = 0;
    while (list != NULL)
    {
        int year_already_found = 0;
        for (int x = 0; x < row_count; x++)
        {
            if (list->release_year == years[x])
            {
                year_already_found = 1;
            }
        }
        if (year_already_found == 0)  //max rating for this year not yet found
        {
            years[years_size] = list->release_year;
            years_size++;
            int check_year = list->release_year;
            int movie_to_report = searchMaxRatingInYear(list, check_year);  //get position in linked list of movie with best rating in year
            reportMaxRating(list, row_count, movie_to_report);
        }
        list = list->next;
    }
}

void findLanguages(char *lang, struct movie *list)
/* Function for option 2. Finds all movies provided in desired language*/
{
    int i;
    int encountered_lang = 0;
    while (list != NULL)
    {
        for (i = 0; i < 5; i++)
        {
            if (list->languages[i] != NULL)
            {
                if (strcmp(list->languages[i], lang) == 0)
                {
                    printf("%s\n", list->title);
                    encountered_lang = 1;
                }
            }
        }
        list = list->next;
    }
    if (encountered_lang == 0)
    {
        printf("There are no movies listed that are available in %s\n", lang);
    }   
}


int directOption(int option, struct movie *list, int row_count)
{
    if (option == 4)
    {
        return option;
    }
    if (option > 4 || option < 1)
    {
        printf("You've chosen an invalid option. Please try again.");
    }
    else if (option == 1)
    {
        int year;
        printf("Enter the year for which you want to see movies: ");
        scanf("%d", &year);
        findYear(year, list);
    }
    else if (option == 2)
    {
        findHighRatings(list, row_count);
    }
    else if (option == 3)
    {
        char lang[20];
        printf("Enter the language for which you want to see movies: ");
        scanf("%s", lang);
        findLanguages(lang, list);
    }
}

int takeUserInput(struct movie *list, int row_count)
{
    int option;
    printf("\n1. Show movies released in the specified year\n"
        "2. Show highest rated movie for each year\n"
        "3. Show the title and year of release of all movies in a specific language\n"
        "4. Exit from the program\n\n");
    printf("Enter a choice from 1 to 4: ");
    scanf("%d", &option);
    directOption(option, list, row_count);
    return option;
}

int main(int argc, char *argv[])
{
    if (argc < 2)       // no file provided
    {
        printf("You must provide the name of the file to process\n");
        printf("Example usage: ./movies movie_info1.txt\n");
        return EXIT_FAILURE;  
    }
    int row_count = 0;
    struct movie *list = processFile(argv[1], &row_count);      //process file into linked list
    //printMovieList(list);
    printf("Processed file %s and parsed data for %d movies\n", 
            argv[1], row_count);
    int option = 0;
    while (option != 4)
        {
            option = takeUserInput(list, row_count);
        }
    return EXIT_SUCCESS;
}







/* citations:
Adis Bari on Udemy for more information on pointers in functions, such as with incrementing while using pointer
https://www.tutorialspoint.com/how-can-we-return-multiple-values-from-a-function-in-c-cplusplus#:~:text=In%20C%20or%20C%2B%2B,values%20from%20a%20function%20directly.&text=We%20can%20return%20more%20than,will%20take%20pointer%20type%20data.
for extra examples on working with pointers in functions
*/

