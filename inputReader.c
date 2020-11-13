#include "inputReader.h"

#include "neighbourManager.h"
#include "dataManager.h"

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <time.h>
#include <ctype.h>

#define YELLOW "\e[33m"
#define DEFAULT "\e[0m"

#include <fcntl.h>
#include <termios.h>

#define INPUT_TIMEOUT 5

#define INPUT_LEN 200
static char input[INPUT_LEN] = {0};
static int input_index = 0;

#define NAME_LEN 16
static char name[NAME_LEN] = {0};


/*******************/
/*     General     */
/*******************/

static int read_char() {

    int c = getchar();
    
    if(c == EOF) return c; 
    if( c == 127 ) {
        if(input[input_index] == '\0' && input_index > 0)
            input_index--;
        input[input_index] = '\0';
        
    } else if( !iscntrl(c) && input_index < INPUT_LEN-1) {    
        input[input_index++] = c;
        input[input_index] = '\0';
    }
    return c;
}

static void print_input() {
    fprintf(stdout, "\033[2K\033[50D" YELLOW "%s : " DEFAULT "%s", name, input);
    fflush(stdout);
}

static void print_waiting() {
    fprintf(stdout, "\033[2K\033[50D" YELLOW "Maintenance/Mise à jour..." DEFAULT "");
    fflush(stdout);
}

/********************/
/*  Initialisation  */
/********************/

void init_inputReader() {
    memset(input, 0, INPUT_LEN);
    
    fprintf(stdout, "Entrez le nom que vous voulez utiliser:\n");
    int c = 0;
    while( c != '\n' && strlen(input) < NAME_LEN-1 ) {
        c = read_char();
        fprintf(stdout, "\033[2K\033[50D%s",input);
        fflush(stdout);
    }

    // On enleve les espaces à la fin 
    int index = input_index-1;
    while( isspace(input[index]) )
        input[index--] = '\0';

    // On enleve les espaces au début
    index = 0;
    while( isspace(input[index]) )
        input[index++] = '\0';

    // On cherche le premier caractere nom blanc
    int len = strlen(input);
    char* start = input;
    while(*start == '\0' && start-input < NAME_LEN-1)
        start++;

    if(start-input == NAME_LEN-1) {
        fprintf(stderr, "Le nom ne doit pas être vide.\n");
        init_inputReader();
        return;
    }
    
    // On véifie que les caractères sont correctes.
    for(int i = 0; i < len; i++)
        if( !isgraph(start[i]) ) {
            fprintf(stderr, "Caractères incorectes '%d'(caractères invisibles), entrez un autre nom.\n", start[i]);
            init_inputReader();
            return;
        }
    
    memmove(name, start, NAME_LEN-1);
    name[NAME_LEN-1] = '\0';
    memset(start, 0, NAME_LEN);
    input_index = 0;
}


/*******************/
/*     Lecture     */
/*******************/

void read_input() {
    int rc;

    print_input();
    
    struct termios term;

    tcgetattr(0, &term);
    term.c_lflag &= ~ICANON;
    term.c_cc[VMIN] = 0;
    term.c_cc[VTIME] = 0;
    tcsetattr(0, TCSANOW, &term);
    
    int c;
    int oto = time(NULL) + INPUT_TIMEOUT;
    int to;
    
 again:

    to = oto - time(NULL);

    fd_set readfds;
    
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);
    struct timeval tv = {to, 0};
    rc = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &tv);
    if(rc < 0) {
        perror("select");
        exit(1);
    }
    if(rc > 0) {
        if(FD_ISSET(STDIN_FILENO, &readfds)) {
            
            // réponse bien reçue, on peut la gérer

            c = read_char();

            if(c != '\n') {
                print_input();
                goto again;
            }

            fprintf(stdout, "\033[2K\033[50D");
            fflush(stdout);

            if(strlen(input) > 1) {
                int size = strlen(name) + 3 + strlen(input);
                uint8_t buf[size];
                snprintf((char*)buf, size, "%s : %s", name, input);
                buf[size-1] = input[strlen(input)-1];
                
                add_my_data(buf, size);
            }
                
            memset(input, 0, strlen(input));
            input_index = 0;
            return;
            
        } else {
            fprintf(stderr, "FD_ISSET : Descripteur de fichier inattendu.\n");
            exit(1);
        }
    } else {
        // timeout
                
        if(input[0] != '\0') {

            start_hello_sender();
            symetrics_maintenance();
            
            oto = time(NULL) + INPUT_TIMEOUT;
            goto again;
        }

        print_waiting();
        
        return;
    }
            
}

/***************/
/*    Print    */
/***************/

void fprintn(FILE* f, const char* format, ...) {
    fprintf(f, "\033[2K\033[50D");
    fflush(f);
    va_list vargs;
    va_start(vargs, format);
    vfprintf(f, format, vargs);
    va_end(vargs);
    fprintf(f, "\n");
    print_waiting();
}

void printn(const char* format, ...) {
    fprintf(stdout, "\033[2K\033[50D");
    fflush(stdout);
    va_list vargs;
    va_start(vargs, format);
    vfprintf(stdout, format, vargs);
    va_end(vargs);
    fprintf(stdout, "\n");
    print_waiting();
}
