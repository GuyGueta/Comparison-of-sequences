
/**
 * @file CompareSequences.c
 * @author  guy gueta <guy.gueta@mail.huji.ac.il>
 * @version 1.0
 * @date 13 nov 2018
 *
 * @brief System to pars a file of words and analyze info out of him
 * @section LICENSE
 * This program is not a free software;
 * @section DESCRIPTION
 * the system pars a file , and with the func calculateMax its checks the longest match substring
 * between every 2 words in the given file and prints it out  .
 * Input  : a path of  file and 3  int values gap mismatch and match .
 * Process: parsing the file and calculating for every word we pars,
 * from the file the longest match substring between her and all the other words .
 * Output : prints  longest match substring between every 2 words in the given file .
 */

/**************************************includes**************************************************/
#include <stdio.h>

#include <stdlib.h>

#include <string.h>

#include <errno.h>


/**************************************defines***************************************************/

/** read file command*/
#define READ "r"

/** the max number of chars can be in one line*/
#define LINE_SIZE 100

/** the max number of sequences allowed*/
#define MAX_NUM_OF_SEQUENCES 100

/** the sign that Symbolizes the start of a header line*/
#define HEADER_SIGN ">"

/** the number of chars to check if there is a header sign*/
#define HEADER_CHECK 1

#define SCORE_MSG "Score for alignment of %s to %s is %d\n"

/* add of the int 1*/
#define CHANGE 1

#define MIN_NUMBER_OF_SEQ 2

/** args numbers */
#define ARG1 1
#define ARG2 2
#define ARG3 3
#define ARG4 4
#define ARG_NUM 5

/** error msg */
#define  NUM_OF_ARGS_ERROR "Usage: CompareSequences <file.txt> <m> <s> <g> ...\n"

#define OPEN_FILE_ERROR  "Usage: CompareSequences <file.txt> <m> <s> <g> ...\n"

#define INVAILD_ARGS_ERROR "Error: %s is invalid input\n"

#define MEMORY_ALLOCATE_FAIL "Error: Allocating memory is failed.\n"

#define ERROR_AMOUNT_OF_SEQ "Error: File should contain at least two sequences.\n"



/**************************************funcs*****************************************************/

/**
 * the funcks free a 2D malloc array from a given index and close the program, the free process is
 * different to a 2 d malloc array , and for a 2d array that only her inner arrays are malloc
 * @param twoDArray 2D malloc array who need to get closed
 * @param index the given index that from him we starts to free the inner malloc arrays
 */
void twoDemArrayFreeFail(char* twoDArray[MAX_NUM_OF_SEQUENCES], int index)
{
    int i;
    for (i = index ; i >= 0 ; i--)
    {
        free(twoDArray[i]);
    }
    fprintf(stderr, MEMORY_ALLOCATE_FAIL);
    exit(EXIT_FAILURE);
}


/**
 * the func checks if the memory allocation was good and if not its calling twoDemArrayFreeFail
 * @param wordsArr the array that we free at twoDemArrayFreeFail if the memory allocation was not
 * good
 * @param index the index in the array the allocation failed
 * @param arrToCheck the malloc that his allocation we check.
 */
void mallockCheck(char* wordsArr[MAX_NUM_OF_SEQUENCES] , int index, char* arrToCheck)
{
    if(arrToCheck == NULL)
    {
        free(arrToCheck);
        twoDemArrayFreeFail(wordsArr, index);
    }
}

/**
 * the funcs add a word that was pars from the file to the words arr.
 * @param wordsArr the array that contains the word that were pars from the file
 * @param counter a counter to the number of words that were add to the wordsArr
 * @param tempLine the current marge of past lines, that were marge to one string that we parsing.
 */
void addSWordTo(char* wordsArr[MAX_NUM_OF_SEQUENCES], int counter, char* tempLine)
{
    wordsArr[counter] = (char*)malloc(sizeof(char) * (strlen(tempLine) + sizeof(char)));
    int len = (int)strlen(tempLine);
    int i;
    for (i = 0; i < len + CHANGE ; i++)
    {
        wordsArr[counter][i] = '\0';
    }
    mallockCheck(wordsArr, counter, wordsArr[counter]);
    strcat(wordsArr[counter], tempLine);
    free(tempLine);
}

/**
 * the func is Process the current line in to a word , that we are adding to the words array
 * @param wordsArr the array that contains the word that were pars from the file
 * @param lineBuffer the current line that we parsing.
 * @param tempLine the current marge of past lines, that were marge to one string that we parsing.
 * @param counter a counter to the number of words that were add to the wordsArr
 * @return the word that we processed.
 */
char* createWord(char* wordsArr[MAX_NUM_OF_SEQUENCES], char* lineBuffer, char* tempLine,
                 int counter)
{
    unsigned long currentLineLen = strlen(lineBuffer);
    if (tempLine == NULL)
    {
        tempLine = (char*)malloc(sizeof(char) * (currentLineLen + sizeof(char)));
        size_t i;
        for (i = 0; i < currentLineLen; i++)
        {
            tempLine[i] = '\0';
        }
        mallockCheck(wordsArr, counter, tempLine);
    }
    else
    {
        unsigned long tempLineLen = strlen(tempLine);
        unsigned long reAllocLen = tempLineLen + currentLineLen;
        tempLine = realloc(tempLine, reAllocLen + sizeof(char));
        mallockCheck(wordsArr, counter, tempLine);
    }
    return tempLine;
}

/**
 * the func is Process the current nameline and add her to the name of seq array , that we are adding to the words
 * array
 * @param seqList the arr that we enter the the name of the words to .
 * @param nameLine the current name line we process.
 * @param counter the counter of the number of seq names we add
 */
void updateSeqList (char* seqList[MAX_NUM_OF_SEQUENCES], char* nameLine, int counter )
{
    nameLine = nameLine + CHANGE;
    seqList[counter] = (char*)malloc(sizeof(char) * (strlen(nameLine) + sizeof(char)));
    int len = (int)strlen(nameLine);
    int i;
    for (i = 0; i < len + CHANGE ; i++)
    {
        seqList[counter][i] = '\0';
    }
    mallockCheck(seqList, counter, seqList[counter]);
    strcat(seqList[counter], nameLine);
}


/**
 * the func pars the file
 * @param myFile the name of the file we pars
 * @param wordsArr the arr we put all the given words into
 * @param counter a counter to the number of patterns that were found in the file
 * @param seqList the arr that we enter the the name of the words to .
 * @return
 */
int parsFile(FILE* myFile , char* wordsArr[MAX_NUM_OF_SEQUENCES], int counter,
             char* seqList[MAX_NUM_OF_SEQUENCES])
{
    char lineBuffer[LINE_SIZE + CHANGE];
    char* tempLine = NULL;
    int headerCounter = 0;
    while(fgets(lineBuffer, LINE_SIZE + CHANGE, myFile) != NULL && counter < MAX_NUM_OF_SEQUENCES)
    {
        lineBuffer[strcspn(lineBuffer, "\r")] = '\0';
        lineBuffer[strcspn(lineBuffer, "\n")] = '\0';
        if(strncmp(lineBuffer, HEADER_SIGN, HEADER_CHECK) == 0)
        {
            updateSeqList(seqList, lineBuffer, headerCounter);
            if(tempLine == NULL)
            {
                headerCounter++;
                continue; // if this is the first header
            }
            addSWordTo(wordsArr, counter, tempLine);
            tempLine = NULL;
            headerCounter++;
            counter ++;
            continue;
        }
        else
        {
            tempLine = createWord(wordsArr, lineBuffer, tempLine, counter);
            tempLine = strcat(tempLine, lineBuffer);
            continue;
        }
    }
    addSWordTo(wordsArr, counter, tempLine);
    tempLine = NULL;
    counter++;
    return counter;
}

/**
 * the func free the all the mallocs of the given array
 * @param wordsArr the array that contains the word that were pars from the file
 * @param counter a counter to the number of words that were add to the wordsArr
 */
void freeTwoDArray(char** wordsArr, int counter)
{
    int i;
    for (i = 0; i < counter; i++)
    {
        free(wordsArr[i]);
        wordsArr[i] = NULL;
    }
}

/**
 * init a 2 d int array with mallocs, that we use in the calculate max.
 * @param arrayToInit the 2 d array that we are Initializing
 * @param firstLen the numbers of rows , its the first string len
 * @param secondLen the number of Columns, its the second string len.
 * @return 2 d int array
 */
int ** initTable(int** arrayToInit, int firstLen, int secondLen)
{

    arrayToInit = (int**)calloc(secondLen * sizeof(int*) + sizeof(char), secondLen * sizeof(int*) + sizeof(char));
    if(arrayToInit == NULL)
    {
        free(arrayToInit);
        fprintf(stderr, MEMORY_ALLOCATE_FAIL);
        exit(EXIT_FAILURE);
    }
    int i;
    for( i = 0; i < firstLen ; i++)
    {

        arrayToInit[i] = (int *)calloc(sizeof(int) * secondLen, sizeof(int) * secondLen);
        if(arrayToInit[i] == NULL)
        {
            free(arrayToInit);
            fprintf(stderr, MEMORY_ALLOCATE_FAIL);
            exit(EXIT_FAILURE);
        }
    }
    return arrayToInit;
}

/**
 * add to the 2 d array that represents the table we work with at calculateMax, the value at the
 * bounds
 * @param tableArray 2 d array that represents the table we work with at calculateMax
 * @param firstLen the numbers of rows , its the first string len
 * @param second Len the number of Columns, its the second string len.
 * @param gap a value given by the user, used in the calculate max Formula.
 */
void addBoundsOfTable(int **tableArray, int firstLen, int secondLen, int gap)
{
    int i;
    for ( i = 0; i < firstLen; i++)
    {
        tableArray[i][0] = gap * i;
    }
    int j;
    for (j = 0; j < secondLen; j++)
    {
        tableArray[0][j] = gap * j;
    }
}

/**
 * check if 2 chars from the 2 words we are checking are match , sum the value at the table index
 * according to the calculate max Formula.
 * @param tableArray the table that we use to calculate the max
 * @param firstChar the first char to compere
 * @param secondChar the second char to compere
 * @param match a value given by the user, used in the calculate max Formula.
 * @param misMatch a value given by the user, used in the calculate max Formula.
 * @param i the row index
 * @param j the collum index
 * @return the value of the given index according to the calculate max Formula.
 */
int checkMatch(int ** tableArray, char firstChar, char secondChar, int match , int misMatch, int i,
               int j)
{
    int sum = 0;
    if( firstChar == secondChar )
    {
        sum = tableArray[i - CHANGE][j - CHANGE] + match;
    }
    else
    {
        sum = tableArray[i - CHANGE][j- CHANGE] + misMatch;
    }
    return sum;
}
 /** a help func to calculate max ,find the best value for every index in the table and fill
 * the table
 * @param firstLen the first word len .
 * @param secondLen the second word len.
 * @param tableArray the table that we use to calculate the max
 * @param firstChar the first char to compere
 * @param secondChar the second char to compere
 * @param match a value given by the user, used in the calculate max Formula.
 * @param misMatch a value given by the user, used in the calculate max Formula.
  *@param firstWord the first word to match
  *@param secondWord the second word to match
  */
void calculateTable(int firstLen, int secondLen, int** tableArray, int match, int misMatch,
                    int gap, char* firstWord, char* secondWord)
{
    int i, j;
    for (i = 1 ; i < firstLen  ; i++)
    {
        for( j = 1 ; j < secondLen  ; j++)
        {
            int op1 = 0, op2 = 0 , op3 = 0;
            op1 = checkMatch(tableArray, firstWord[i -CHANGE], secondWord[j -CHANGE] , match, misMatch, i, j);
            op2 = tableArray[i][j -CHANGE] + gap;
            op3 = tableArray[i-CHANGE][j] + gap;
            int max1 = ((op1 >= op2) ? op1 : op2);
            int max2 = ((op2 >= op3) ? op2 : op3);
            int max = ((max1 >= max2) ? max1 : max2);
            tableArray[i][j] = max;
        }
    }
}

/**
 * the func calculate size of the longest substring that natch between the given 2 words
 * @param firstWord the first word we compere
 * @param secondWord the second word we compere
 * @param match a value given by the user, used in the calculate max Formula.
 * @param misMatch a value given by the user, used in the calculate max Formula.
 * @param gap a value given by the user, used in the calculate max Formula.
 * @return the size of the longest substring that natch between the given 2 words
 */
int calculateMax(char* firstWord, char* secondWord ,  int match, int misMatch, int gap)
{
    int firstLen = (int)strlen(firstWord) + CHANGE;
    int secondLen = (int)strlen(secondWord) + CHANGE;
    int ** tableArray = NULL;
    tableArray = initTable(tableArray, firstLen, secondLen);
    addBoundsOfTable(tableArray, firstLen, secondLen, gap);
    calculateTable(firstLen, secondLen, tableArray, match, misMatch, gap, firstWord, secondWord);
    int maxNum =  tableArray[firstLen - CHANGE][secondLen - CHANGE];
    int k ;
    for(k = 0; k < firstLen ; k++)
    {
        free(tableArray[k]);
    }
    free(tableArray);
    return maxNum;
}

/**
 * the func gets a string that is a path of a file , open it and calles parsFile if the path is
 * good ,and sends errors
 * otherwise
 * @param fileName the path of the file
 */
FILE* openFile(const char* fileName)
{
    FILE *fp = NULL;
    fp = fopen(fileName, READ);
    if(fp == NULL)
    {
        fprintf(stderr, OPEN_FILE_ERROR);
        exit(EXIT_FAILURE);
    }
    return fp;
}

/**
 * compers all the string word we pars from the file , and for each 2 prints the
 * size of the longest substring that natch between the given 2 words
 * @param fileName the name of the file we parsing
 * @param match a value given by the user, used in the calculate max Formula.
 * @param misMatch a value given by the user, used in the calculate max Formula.
 * @param gapa value given by the user, used in the calculate max Formula.
 */
void compereAllSeq(const char* fileName, int match, int misMatch, int gap)
{
    FILE* wordsFile = NULL;
    wordsFile = openFile(fileName);
    char* wordArr[MAX_NUM_OF_SEQUENCES];
    char* seqArr[MAX_NUM_OF_SEQUENCES];
    int counter = 0;
    counter = parsFile(wordsFile , wordArr, counter , seqArr);
    if(counter < MIN_NUMBER_OF_SEQ)
    {
        fprintf(stderr, ERROR_AMOUNT_OF_SEQ);
        exit(EXIT_FAILURE);
    }
    int i = 0;
    for (i = 0; i < counter ; i++ )
    {
        int j = 0;
        for (j = i + CHANGE; j < counter; j++ )
        {
            int sum = 0;
            sum = calculateMax(wordArr[i], wordArr[j], match, misMatch, gap);
            printf(SCORE_MSG, seqArr[i], seqArr[j], sum);
        }
    }
    freeTwoDArray(wordArr, counter);
    freeTwoDArray(seqArr, counter);
    fclose(wordsFile);
    wordsFile = NULL;
}

/**
 * the func check if the argument is valid if so its return is in value , else its printing error msg and closed the
 * the program
 * @param argToCheck the arg to check
 * @return the value of the argument if he is valid.
 */
int checkArgs(const char* argToCheck)
{
    char* end;
    float result = 0;
    errno = 0;
    result = strtol(argToCheck, &end, 0);
    if (result == 0 && (errno != 0 || end == argToCheck))
    {
        free(end);
        end = NULL;
        fprintf(stderr, INVAILD_ARGS_ERROR, argToCheck);
        exit(EXIT_FAILURE);
    }
    return (int) result;
}

/**
 * the main func of the progrem
 * @param argc the number of arguments
 * @param argv the array of arguments the user inserts
 * @return 1 if a fail , 0 if the program dont have fails
 */
int main(const int argc, const char** argv)
{
    if( argc != ARG_NUM)
    {
        fprintf(stderr, NUM_OF_ARGS_ERROR);
    }
    const char* filePath = argv[ARG1];
    int match, misMatch, gap;
    match = checkArgs(argv[ARG2]);
    misMatch = checkArgs(argv[ARG3]);
    gap = checkArgs(argv[ARG4]);
    compereAllSeq(filePath, match, misMatch, gap);
    return 0;
}


