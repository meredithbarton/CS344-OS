#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <sys/types.h>
#include <time.h>


// ------------------------------ PARSE FILE --------------------//
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


struct movie *processFile(char *filePath)
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
        //start linked list if empty
        if (head == NULL)  
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

// ------------------------------- FILE PROCESSING --------------------------


void fileYear(int year, struct movie *list, char *dir)
/* Function for option 1. Prints all movie titles listed from desired year*/
{
    FILE *fd;
    char fn[256];
    sprintf(fn, "%s/%d.txt", dir, year);

    if ((fd = fopen(fn, "w+"))!=NULL)
    {
        chmod(fn, S_IRUSR || S_IWUSR || S_IRGRP);
        while (list != NULL)
        {
            if (list->release_year == year)
            {
                fprintf(fd, "%s\n", list->title);
            }
            list = list->next;
        }
        fn[0] = 0; 
        fclose(fd); 
    }
}

int listYears(struct movie *list, int *years)
{
    int years_size = 0;
    while(list != NULL)
    {
        bool year_already_found = 0;
        // create list of unique years
        for (int x = 0; x < 122; x++)
        {
            if (list->release_year == years[x])
            {
                year_already_found = 1;
            }
        }
        if (!year_already_found)
        {
            years[years_size] = list->release_year;
            years_size++;
        }
        list = list->next;
    }
    // fill remainder of years array with null value
    if (years_size < 122)
    while (years_size != 122)
    {
        years[years_size] = 0;
        years_size++;
    }
}

void process(char *fn)
{
    struct dirent *aDir;
    // make directory
    char *dir = malloc(23*sizeof(char));
    srandom(time(NULL));
    sprintf(dir, "bartonme.movies.%d", (random() % (100000)));
    mkdir(dir, 0750);

    // gather list of years
    struct movie *list = processFile(fn);
    int years[122];
    listYears(list, years);

    // fill directory
    DIR* currDir = opendir(dir);
    int index = 0;
    /*
    FILE *fd;
    while (list != NULL)
    {
        char fn[256];
        sprintf(fn, "%s/%d.txt", dir, list->release_year);

        if ((fd = fopen(fn, "w+"))!=NULL)
        {
            //chmod(fn, S_IRUSR || S_IWUSR || S_IRGRP);
            fprintf(fd, "%s\n", list->title);
            fn[0] = 0; 
            fclose(fd); 
        }
        list = list->next;
    }
    */
    while (years[index] != 0)
    {
        fileYear(years[index], list, dir);
        index++;
    }
    closedir(currDir);
}



// ------------------------------ SEARCH FOR FILE ---------------------------


char *searchLargestFile()
{
    char *fn = malloc(256*sizeof(char));
    DIR* currDir = opendir(".");    //citation: from Directories Exploration
    struct dirent *aDir;
    struct stat dirStat;
    struct file_details
    {
        char name[256];
        long int size; 
    };
    
    struct file_details longest_file;
    longest_file.size = 0;

    while((aDir = readdir(currDir)) != NULL)
    {
        if (strncmp("movies_", aDir->d_name, strlen("movies_")) == 0)
        {
            FILE* fd = fopen(aDir->d_name, "r");  // use FILE* instead of int fd so ftell can be used
            fseek(fd, 0L, SEEK_END);
            long int res = ftell(fd);
            char *dot = strchr(aDir->d_name, '.');
            bool isCSV = 0;
            /* citation: https://stackoverflow.com/questions/5309471/getting-file-extension-in-c
            The above page gave the idea to search for "." in filename */
            if (strcmp(dot+1, "csv") == 0)
            {
                isCSV = 1;
            }
            if (res > longest_file.size && isCSV)
            {
                strcpy(longest_file.name, aDir->d_name);
                longest_file.size = res - 1;
            }
            strcpy(fn, longest_file.name);
            fclose(fd);
        }
    }
    printf("\nThe largest file is %s. This file will now be processed.\n", longest_file.name);
    closedir(currDir);
    return fn;
}

char *searchSmallestFile()
{
    char *fn = malloc(256*sizeof(char));
    DIR* currDir = opendir(".");    //citation: from Directories Exploration
    struct dirent *aDir;
    struct stat dirStat;
    struct file_details
    {
        char name[256];
        long int size; 
    };
    
    struct file_details shortest_file;
    shortest_file.size = -1;

    while((aDir = readdir(currDir)) != NULL)
    {
        if (strncmp("movies_", aDir->d_name, strlen("movies_")) == 0)
        {
            FILE* fd = fopen(aDir->d_name, "r");  // use FILE* instead of int fd so ftell can be used
            fseek(fd, 0L, SEEK_END);
            long int res = ftell(fd);  // points to the last byte in file, returns int as byte's position (i.e. how many bytes in file)
            char *dot = strchr(aDir->d_name, '.');
            bool isCSV = 0;
            if (strcmp(dot+1, "csv") == 0)
            {
                isCSV = 1;
            }
            if (isCSV && (res < shortest_file.size || shortest_file.size == -1))
            {
                strcpy(shortest_file.name, aDir->d_name);
                shortest_file.size = res - 1;
            }
            strcpy(fn, shortest_file.name);
            fclose(fd);
        }
    }
    printf("\nThe smallest file is %s. This file will now be processed.\n", shortest_file.name);
    closedir(currDir);
    return fn;
}

char *searchFileByName()
{   
    char *fn = malloc(256*sizeof(char));
    DIR* currDir = opendir("."); 
    struct dirent *aDir;
    bool found = 0;
    

    printf("Please enter the desired file name: ");
    scanf("%s", &fn);

    while ((aDir = readdir(currDir)) != NULL)
    {
        if (strcmp(fn, aDir->d_name) == 0)
        {
            found = 1;
            printf("\nFound file with name %s. This file will now be processed.\n", fn);
            return fn;
        }
    }
    if (!found)
    {
        printf("\nFile %s does not appear in this directory\n", fn);
        return "NULL";
    }
}

// ---------------------------- MAIN AND USER INTERACTION ----------------------

void whichFile()
{
    int option;
    printf("\nWhich file should be processed?\n"
    "Enter 1 to pick the largest file\n"
    "Enter 2 to pick the smallest file\n"
    "Enter 3 to specify the name of the file\n");
    printf("Enter choice 1, 2, or 3: ");
    scanf("%d", &option);
    if (1 < option < 2)
    {
        char *fn = malloc(256*sizeof(char));

        if (option == 1)
        {
            fn = searchLargestFile();
        }
        else if (option == 2)
        {
            fn = searchSmallestFile();
        }
        else if (option == 3)
        {
            fn = searchFileByName();
        }
        process(fn);
    }
    else
    {
        printf("\nInvalid option chosen.\n");
    }
}

int directOption(int option)
{
    if (option == 2)
    {
        return option;
    }
    else if (option == 1)
    {
        whichFile();
    }
    else if (option < 1 || option > 2)
    {
        printf("You've chosen an invalid option. Please try again.");
    }
}


int takeUserInput()
{
    int option;
    printf("\n1. Select file to process\n"
            "2. Exit Program\n\n");
    printf("Enter choice 1 or 2: ");
    scanf("%d", &option);
    directOption(option);
    return option;
}

int main()
{
    int option = 0;
    while (option != 2)
        {
            option = takeUserInput();
        }
    return EXIT_SUCCESS;
}



