
/*  Changer is linux-based anti-forensics tool for reading,      */
/*  extracting, modifying and destroying selected                */ 
/*  information from files, partitions and whole disks           */ 
/*  using gained byte offsets from commands like grep.           */ 
/*  Basically, it's srm but on manual scale with surgical        */
/*  precision and bunch of other things.                         */
/*  Coded by Lazar Marković for no special reason.               */                         

/*  This code is free; you can redistribute it and/or            */
/*  modify it under the terms MIT Licence. While library         */
/*  'mt.h' called MT19937int is licensed it under the terms      */ 
/*  of the GNU Library General Public License as published       */ 
/*  by the Free Software Foundation; either version 2 of the     */
/*  License, or (at your option) any later version.              */  

/*  Copyright (C) 2019 Lazar Marković.                           */
/*  Any feedback is very welcome. For any question, comments,    */
/*  use my email <lazar тачка kmarkovic ет gmail тачка com>.     */


#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <unistd.h>

#include <stdlib.h>
#include <stdio.h>

#include <argp.h>

#include <inttypes.h>
#include <errno.h>

#include <string.h>
#include <ctype.h>

#include <sys/time.h>
#include "mt.h"


/* BEGIN Global constants */
///////////////////////////////////////////////////////////////////////////////

const int RNG_PAGE = 42;                /* Defines working page of Mersenne Twister RNG */
const int SHUFFLE_RAND_MAX = 3593;      /* Used for shuffling arrays */
const int DEFAULT_BUFFER_SIZE = 42;     /* Default buffer size */   

const int INI_ARRAY_SIZE = 100;         /* Defines initial array length for array loader filling logic */
const int RESIZE_FACTOR = 10;           /* Defines array length extention factor for array loader filling logic */          

const char *DUMMY_DATA =                /* Dummy data used to replace actual data in DESTROY_DATA command (id needed) */
"Lorem ipsum dolor sit amet, \
consectetur adipiscing elit, sed do eiusmod tempor \
incididunt ut labore et dolore magna aliqua. \
Ut enim ad minim veniam, quis nostrud exercitation \
ullamco laboris nisi ut aliquip ex ea commodo consequat. \
Duis aute irure dolor in reprehenderit in voluptate velit \
esse cillum dolore eu fugiat nulla pariatur. Excepteur \
sint occaecat cupidatat non proident, sunt in culpa qui \
officia deserunt mollit anim id est laborum.";      

const int DUMMY_DATA_LENGTH = 445;      /* Length of dummy data */

const char SPECIAL_DATA[27][3] = {      /* Special data used in Gutmann method for data sanitization */
    {0x55, 0x55, 0x55},
    {0xAA, 0xAA, 0xAA},
    {0x92, 0x49, 0x24},
    {0x49, 0x24, 0x92},
    {0x24, 0x92, 0x49},
    {0x00, 0x00, 0x00},
    {0x11, 0x11, 0x11},
    {0x22, 0x22, 0x22},
    {0x33, 0x33, 0x33},
    {0x44, 0x44, 0x44},
    {0x55, 0x55, 0x55},
    {0x66, 0x66, 0x66},
    {0x77, 0x77, 0x77},
    {0x88, 0x88, 0x88},
    {0x99, 0x99, 0x99},
    {0xAA, 0xAA, 0xAA},
    {0xBB, 0xBB, 0xBB},
    {0xCC, 0xCC, 0xCC},
    {0xDD, 0xDD, 0xDD},
    {0xEE, 0xEE, 0xEE},
    {0xFF, 0xFF, 0xFF},
    {0x92, 0x49, 0x24},
    {0x49, 0x24, 0x92},
    {0x24, 0x92, 0x49},
    {0x6D, 0xB6, 0xDB},
    {0xB6, 0xDB, 0x6D},
    {0xDB, 0x6D, 0xB6}
}; 
const int SPECIAL_DATA_LENGTH = 27;     /* Legth of special data array */

/* END Global constants */
///////////////////////////////////////////////////////////////////////////////


// BEGIN UTILS CODE
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int string_to_ulli(char *str, unsigned long long int *num) {
    char *pend;
    
    unsigned long long int potential_num = strtoull(str, &pend, 10);
    if (errno == ERANGE || errno == EINVAL) {
        return -1;
    }
    *num = potential_num;

    return 0;
}

void shuffle(int aa[], int n)
{
    int i;
    for (i = 0; i < n - 1; i++) 
    {
        int random_value = genrand(RNG_PAGE) % SHUFFLE_RAND_MAX;
        int j = i + random_value / (SHUFFLE_RAND_MAX / (n - i) + 1);


        int t = *(aa + j);
        *(aa + j) = *(aa + i);
        *(aa + i) = t;
    }
}

/* Print operation message if verbrose mode is on */
int print_text(char *message, int print) {
    if (print) {
        printf("%s", message);
    }
}

// END UTILS CODE
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// BEGIN ARGP SETUP
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
/* This structure is used by main to communicate with parse_opt. */
struct arguments
{
    char *infile;                           /* Argument for -i */
    char *args[1];                          /* MEM_OFFSET_LIST */
    int use_stream;

    int read_data;                          /* The -r flag */

    int save_data;                          /* The -s flag */
    char *outfile;                          /* Argument for -s */

    int mod_data;                           /* The -m flag */
    char* mod_string;                       /* Argument for -m */

    int destroy_data;                       /* The -d flag */
    int destroy_using_lorem;                /* The -l flag */

    unsigned long long int buffer_size;     /* Argument for -b */

    int verbose;                            /* The -v flag */

};

/*
   OPTIONS.  Field 1 in ARGP.
   Order of fields: {NAME, KEY, ARG, FLAGS, DOC}.
*/
static struct argp_option options[] =
{
    {"verbose",             'v', 0,            0, "Produce verbose output."},
    {"read-data",           'r', 0,            0, "Read data indexed with offsets and buffer size and print it."},
    {"save-data",           's', "FILE_PATH",  0, "Save data indexed with offsets and buffer size in defined file. Default file path: './saved_data.txt'."},
    {"destroy-data",        'd', 0,            0, "Destroy data indexed with offsets and buffer size. Data will be overwritten with Lorem Ipsum."},
    {"destroy-with-lorem",  'l', 0,            0, "Use this command in combination with '-d' flag to finish destroying data with Lorem Ipsum overwrite."},
    {"modify-data",         'm', "STRING",     0, "Define string to be written in place of data indexed with offsets and buffer size. Buffer size will be set to string length."},
    {"infile",              'i', "FILE_PATH",  0, "Define input file path or disk image."},
    {"buffer-size",         'b', "INT",        0, "Define read/write buffer size. Default value: 42."},
    {0}
};

/*
   PARSER. Field 2 in ARGP.
   Order of parameters: KEY, ARG, STATE.
*/
static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
    struct arguments *arguments = state->input;

    switch (key) {
        case 'v':
            arguments->verbose = 1;
            break;
        case 'r':
            arguments->read_data = 1;
            break;
        case 's':
            arguments->save_data = 1;
            arguments->outfile = arg;
            break;
        case 'd':
            arguments->destroy_data = 1;
            break;
        case 'l':
            arguments->destroy_using_lorem = 1;
            break;
        case 'm':
            arguments->mod_data = 1;
            arguments->mod_string = arg;
            arguments->buffer_size = strlen(arg);
            break;
        case 'i':
            arguments->infile = arg;
            break;
        case 'b': 
            if (string_to_ulli(arg, &(arguments->buffer_size)) < 0) {
                argp_usage (state);
            }
            break;
        case ARGP_KEY_ARG:
            if (state->arg_num >= 1) {
                argp_usage(state);
            }
            arguments->args[state->arg_num] = arg;
            break;
        case ARGP_KEY_END:
            if (state->arg_num < 1) {
                arguments->use_stream = 1;
            }
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

/*
   ARGS_DOC. Field 3 in ARGP.
   A description of the non-option command-line arguments
     that we accept.
*/
static char args_doc[] = "MEM_OFFSET_LIST (Format: \"INT INT INT INT...\")";

/*
  DOC.  Field 4 in ARGP.
  Program documentation.
*/
static char doc[] =
"changer -- A linux-based anti-forensics tool for reading, extracting, modifying and destroying data from files.\vFrom Lazar Markovic.";

/*
   The ARGP structure itself.
*/
static struct argp argp = {options, parse_opt, args_doc, doc};

// END ARGP SETUP
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


// BEGIN ARRAY FILLING LOGIC - ulli
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
int exchange_arrays(unsigned long long int **data, 
                    unsigned long long int **new_data) {
    unsigned long long int *temp = *data;
    *data = *new_data;
    *new_data = temp;
    return 0;
}

int copy_one_array_to_other(unsigned long long int *source, 
                            unsigned long long int *destination, 
                            unsigned long long int source_size) {
    unsigned long long int i = 0;
    for (i = 0; i < source_size; i++) {
        *(destination + i) = *(source + i);
    }
    return 0;
}

int add_element_to_array(unsigned long long int **array, 
                         unsigned long long int *array_index,
                         unsigned long long int *array_size, 
                         unsigned long long int element) {
    if (*array == NULL) {
        //printf("Init array\n");
        *array = malloc(sizeof(unsigned long long int) * INI_ARRAY_SIZE);
        *array_index = 0;
        *array_size = INI_ARRAY_SIZE;
    } 

    if (*array_index >= *array_size) {
        //printf("Expand array\n");
        unsigned long long int *new_array = 
            malloc(sizeof(unsigned long long int) * RESIZE_FACTOR * *array_size);
        *array_size = RESIZE_FACTOR * *array_size;

        copy_one_array_to_other(*array, new_array, *array_size);
        exchange_arrays(array, &new_array);
        free(new_array);
    }

    *(*array + *array_index) = element;
    ++*array_index;

    return 0;
}

//  END ARRAY FILLING LOGIC - ulli
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// BEGIN ARRAY FILLING LOGIC - char
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
int exchange_arrays_char(char **data, 
                         char **new_data) {
    char *temp = *data;
    *data = *new_data;
    *new_data = temp;
    return 0;
}

int copy_one_array_to_other_char(char *source, 
                                 char *destination, 
                                 unsigned long long int source_size) {
    unsigned long long int i = 0;
    for (i = 0; i < source_size; i++) {
        *(destination + i) = *(source + i);
    }
    return 0;
}

int add_element_to_array_char(char **array, 
                              unsigned long long int *array_index,
                              unsigned long long int *array_size, 
                              char element) {
    if (*array == NULL) {
        //printf("Init char array\n");
        *array = malloc(sizeof(char) * INI_ARRAY_SIZE);
        *array_index = 0;
        *array_size = INI_ARRAY_SIZE;
    } 

    if (*array_index >= *array_size) {
        //printf("Extend char array\n");
        char *new_array = malloc(sizeof(char) * RESIZE_FACTOR * *array_size);
        *array_size = RESIZE_FACTOR * *array_size;

        copy_one_array_to_other_char(*array, new_array, *array_size);
        exchange_arrays_char(array, &new_array);
        free(new_array);
    }

    *(*array + *array_index) = element;
    ++*array_index;

    // Set end of string at next position
    *(*array + *array_index) = '\0';

    return 0;
}

//  END ARRAY FILLING LOGIC - char
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////



// BEGIN INPUT STREAM PROCESSING LOGIC
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
int check_char(char c) {
    if (!isdigit(c) && c != ' ' && c != '\n') {
        return -1;
    }
    return 0;
}

int read_input_stream(char **raw_offset_data_array) {
    unsigned long long int array_index;
    unsigned long long int array_size;

    char c;
    while (1) {
        c = getchar();
        if (c == EOF || c == '\n') { break; }
        if (check_char(c) < 0) {
            printf("Invalid character '%c'. Program can only accept numeric values and spaces.\n", c);
            return -1;
        }
        add_element_to_array_char(raw_offset_data_array, &array_index, &array_size, c);
    }
    return 1;
}

int parse_raw_offset_string(char *raw_data, 
                            unsigned long long int **parsed_array, 
                            unsigned long long int *array_index, 
                            unsigned long long int *array_size) {
    const char *DATA_DELIMITER = " ";

    //printf("TOTAL RAW:->%s<-\n", raw_data);
    
    char *pEnd;
    unsigned long long int number;
    
    //printf("PARSED NUMBERS: \n");
    number = strtoull(raw_data, &pEnd, 10);
    //printf("->%llu\n", number);
    add_element_to_array(parsed_array, array_index, array_size, number);

    while (*pEnd != '\0') {
        number = strtoull(pEnd, &pEnd, 10);
        //printf("->%li\n", number);
        add_element_to_array(parsed_array, array_index, array_size, number);
    }

   return 0;
}

// END INPUT STREAM PROCESSING LOGIC
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// BEGIN MEMORY AND OPERATIONS LOGIC
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/* Buffer fillers */
char get_dummy_data(unsigned long long int write_index) {
    return *(DUMMY_DATA + (write_index % DUMMY_DATA_LENGTH));
}

char get_random_data() {
    return genrand(RNG_PAGE) % 255;
}

int get_special_data(int special_data_index) {
    
}

/* Operations */

int read_data(struct arguments arguments,
              unsigned long long int *data,
              unsigned long long int data_length,
              int *input_file,
              char *buffer) {

    print_text("\nREADING DATA...\n", arguments.verbose);
    print_text("Data will be displayed between '->' and '<-':\n", arguments.verbose);
    unsigned long long int i;
    for (i = 0; i < data_length; i++) {
        lseek(*input_file, *(data + i), SEEK_SET);
        read(*input_file, buffer, arguments.buffer_size);
        if (arguments.verbose) {
            printf("->%s<-\n", buffer);
        } else {
            printf("%s\n", buffer);
        }
    }
    print_text("DONE.\n", arguments.verbose);
}

int save_data(struct arguments arguments,
              unsigned long long int *data,
              unsigned long long int data_length,
              int *input_file,
              char *buffer) {

    print_text("\nSAVING DATA...\n", arguments.verbose);
    int output_file = open(arguments.outfile, O_WRONLY | O_APPEND | O_CREAT);
    if (output_file < 0) {
        printf("Cannot open specified file for data output.");
        return 0;
    } 
    unsigned long long int i;
    for (i = 0; i < data_length; i++) {
        lseek(*input_file, *(data + i), SEEK_SET);
        read(*input_file, buffer, arguments.buffer_size);

        if ((i != 0) && (write(output_file, "\n", 1) != 1)) {
            printf("Error while saving data to output file.\n");
            return -1;
        }

        if (write(output_file, buffer, arguments.buffer_size) != arguments.buffer_size) {
            printf("Error while saving data to output file.\n");
            return -1;
        }

        print_text("*", arguments.verbose);
    }

    if (close(output_file) < 0) {
        perror("Error while closing file.\n");
        exit(1);
    }

    print_text("\nDONE.\n", arguments.verbose);
}

int mod_data(struct arguments arguments,
              unsigned long long int *data,
              unsigned long long int data_length,
              int *input_file,
              char *buffer) {

    print_text("\nMODIFYING DATA...\n", arguments.verbose);
    unsigned long long int i;
    for (i = 0; i < data_length; i++) {
        lseek(*input_file, *(data + i), SEEK_SET);

        if (write(*input_file, arguments.mod_string, arguments.buffer_size) != arguments.buffer_size) {
            printf("Error while modifying data in output file.\n");
            return -1;
        }

        print_text("*", arguments.verbose);
    }

    print_text("\nDONE.\n", arguments.verbose);
}

/* Implements Gutmann method for specified segment of memory */
int destroy_data(struct arguments arguments,
              unsigned long long int *data,
              unsigned long long int data_length,
              int *input_file,
              char *buffer) {

    print_text("\nDESTROYING DATA...\n", arguments.verbose);
    if (arguments.verbose) printf("Offsets to be proessed: %llu\n", data_length);

    unsigned long long int i;
    for (i = 0; i < data_length; i++) {
        if (arguments.verbose) printf("\nStep progression for offset #%llu with value %llu: \n", i+1, *(data + i));

        /* Gutmann method implementation */

        // 1. 4 overwrites with random data
        int j;
        for (j = 0; j < 4; j++) {
            unsigned long long int k;
            for (k = 0; k < arguments.buffer_size; k++) {
                unsigned long long int new_offset = *(data + i) + k;
                lseek(*input_file, new_offset, SEEK_SET);
                char *ccc = malloc(sizeof(char));
                *ccc = get_random_data();
                if (write(*input_file, ccc, 1) != 1) {
                    printf("Error while destroying data data in output file.\n");
                    printf("Program stopped working at offset: %llu\n", new_offset);
                    return -1;
                }
                free(ccc);
            }
            if (arguments.verbose) printf("%d ", j+1);
        }


        // 1. 27 overwrites with special data

        // Shuffle special steps
        int exe_order[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26}; 
        shuffle(exe_order, SPECIAL_DATA_LENGTH);

        // Execute special steps
        for (j = 0; j < 27; j++) {
            unsigned long long int k;
            for (k = 0; k < arguments.buffer_size; k += 3) {
                int l;
                for (l = 0; l < 3; l++) {
                    // Check if end is reached
                    if (k+l >= arguments.buffer_size) break;

                    unsigned long long int new_offset = *(data + i) + k + l;
                    lseek(*input_file, new_offset, SEEK_SET);
                    char *ccc = malloc(sizeof(char));
                    *ccc = SPECIAL_DATA[exe_order[j]][l];
                    if (write(*input_file, ccc, 1) != 1) {
                        printf("Error while destroying data data in output file.\n");
                        printf("Program stopped working at offset: %llu\n", new_offset);
                        return -1;
                    }
                    free(ccc);
                }
            }
            if (arguments.verbose) printf("%d ", j + 5);
        }

        // 3. 4 additional overwrites with random data
        for (j = 0; j < 4; j++) {
            unsigned long long int k;
            for (k = 0; k < arguments.buffer_size; k++) {
                unsigned long long int new_offset = *(data + i) + k;
                lseek(*input_file, new_offset, SEEK_SET);
                char *ccc = malloc(sizeof(char));
                *ccc = get_random_data();
                if (write(*input_file, ccc, 1) != 1) {
                    printf("Error while destroying data data in output file.\n");
                    printf("Program stopped working at offset: %llu\n", new_offset);
                    return -1;
                }
                free(ccc);
            }
           if (arguments.verbose) printf("%d ", j + 32);
        }

       if (arguments.verbose) printf("\n");

        // 4. Optional overwrite using infinite Lorem Ipsum.
        if (arguments.destroy_using_lorem) {
            if (arguments.verbose) printf("Waiting for aditional Lorem Ipsum overwrite step: ");
            unsigned long long int k;
            for (k = 0; k < arguments.buffer_size; k++) {
                unsigned long long int new_offset = *(data + i) + k;
                lseek(*input_file, new_offset, SEEK_SET);
                char *ccc = malloc(sizeof(char));
                *ccc = get_dummy_data(k);
                if (write(*input_file, ccc, 1) != 1) {
                    printf("Error while destroying data data in output file.\n");
                    printf("Program stopped working at offset: %llu\n", new_offset);
                    return -1;
                }
                free(ccc);
            }
            if (arguments.verbose) printf(" done.\n\n");
        }
    }
    print_text("\nDONE.\n", arguments.verbose);
}


// END MEMORY AND OPERATIONS LOGIC
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/* Set default arguments */
int set_arg_defaults(struct arguments *arguments) {
    /* Set argument defaults */
    arguments->args[0] = "";
    arguments->infile = NULL;
    arguments->use_stream = 0;
    arguments->read_data = 0;
    arguments->save_data = 0;
    arguments->outfile = "";
    arguments->mod_data = 0;
    arguments->mod_string = "";
    arguments->destroy_data = 0;
    arguments->destroy_using_lorem = 0;
    arguments->buffer_size = DEFAULT_BUFFER_SIZE;
    arguments->verbose = 0;
}

/* Print all arguments after loading new arguments */
int print_args(struct arguments arguments) {
    printf("OTHER ARGUMENTS:\n");
    printf("Infile       : %s\n",     arguments.infile);
    printf("Use stream   : %d\n",     arguments.use_stream);
    printf("Read data    : %d\n",     arguments.read_data);
    printf("Save data    : %d\n",     arguments.save_data);
    printf("Output file  : %s\n",     arguments.outfile);
    printf("Modify data  : %d\n",     arguments.mod_data);
    printf("Modify string: %s\n",     arguments.mod_string);
    printf("Destroy data : %d\n",     arguments.destroy_data);
    printf("Dstr wht lorm: %d\n",     arguments.destroy_using_lorem);
    printf("Buffer size  : %d\n",     arguments.buffer_size);
    printf("Verbose      : %d\n",     arguments.verbose);
    printf("\n");
}

// Print offset data
int print_offset_data(unsigned long long int *parsed_array,
                      unsigned long long int array_index) {
    printf("\nOFFSET DATA:\n");
    unsigned long long int i;
    for (i = 0; i < array_index; i++) {
        printf("->%llu\n", *(parsed_array + i));
    }
    printf("\n");
}

int check_manual_input_string(char* str) {
    int i;
    int len = strlen(str);
    for (i = 0; i < len; i++) {
        if (check_char(*(str+i)) < 0) {
            return -1;
        }
    }
    return 0;
}

int main(int argc, char **argv) {
    /* Init args */
    struct arguments arguments;
    set_arg_defaults(&arguments);

    /* Start the input magic */
    argp_parse (&argp, argc, argv, 0, 0, &arguments);

    //printf("Finish argp_parse.\n");
    
    /* Read input stream if required */
    char *raw_offset_string = NULL;
    if (arguments.use_stream) {
        if (read_input_stream(&raw_offset_string) < 0) {
            printf("There was error while parsing input stream.\n");
            return 0;
        }
    } else {
        if (check_manual_input_string(arguments.args[0]) < 0) {
            printf("Error while parsing offset values. Either no data is passed or format is wrong. Remember, format is \"INT INT INT INT...\".\n");
            return 0;
        }
        raw_offset_string = arguments.args[0];
    }

     /* Check for empty string */
    if (raw_offset_string == NULL || strlen(raw_offset_string) == 0) {
            printf("Error while parsing offset values. Either no data is passed or format is wrong. Remember, format is \"INT INT INT INT...\".\n");
        return 0;
    }
   

    //printf("Finish reading input stream.\n");

    /* Parse input offset data */
    unsigned long long int *parsed_array = NULL;
    unsigned long long int array_index;
    unsigned long long int array_size;
    if (parse_raw_offset_string(raw_offset_string, &parsed_array, &array_index, &array_size) < 0) {
        printf("Error while parsing offset values. Remember, format is \"INT INT INT INT...\"\n");
        return 0;
    }

    /* If we use malloc-ated memory, free it */
    if (arguments.use_stream) {
        free(raw_offset_string);
    }
    //print_text("Finished reading and parsing offset data.\n", arguments.verbose);

    /* Print main argument value */
    //print_offset_data(parsed_array, array_index);

    // END reading and parsing 
    ////////////////////////////////////////////////////////////////////////////

    /* Resolve other arguments */
    /////////////////////////////////////////////////////////////////////////////
    int input_file;
    char *buffer = malloc(sizeof(char) * arguments.buffer_size); 

    /* Open input file stream */
    if (arguments.infile) {
        input_file = open(arguments.infile, O_RDWR);
        if (input_file < 0) {
            printf("Cannot open specified file.\n");
            return 0;
        }
    } else {
        printf("Input file not specified.\n");
        return 0;
    }

    /* Print other arguments */
    //print_args(arguments);


    /* Execute actions */
    ///////////////////////////////////////////////////////////////////////////////

    /* Initialize RNG */
    struct timeval  tv;
    gettimeofday(&tv, NULL);
    unsigned long time_mil = tv.tv_usec;
    int seed = time_mil % 1000;
    init_twister();
    sgenrand(RNG_PAGE, seed);

    if (arguments.read_data) {
        int o = read_data(arguments, parsed_array, array_index, &input_file, buffer);
        if (0 < 0) {
            printf("There was error during reading data from file.");
        }
    }
    if (arguments.save_data) {
        int o = save_data(arguments, parsed_array, array_index, &input_file, buffer);
        if (0 < 0) {
            printf("There was error during extracting and saving data.");
        }
    }
    if (arguments.mod_data) {
        int o = mod_data(arguments, parsed_array, array_index, &input_file, buffer);
        if (0 < 0) {
            printf("There was error during modifying data in given file.");
        }
    }
    if (arguments.destroy_data) {
        int o = destroy_data(arguments, parsed_array, array_index, &input_file, buffer);
        if (0 < 0) {
            printf("There was error during destroying data in given file.");
        }
    }

    // Close input file
    if (close(input_file) < 0) {
        perror("Error while closing file.");
        exit(1);
    }

    // Free memory
    free(parsed_array);
    free(buffer);

    // Exit
    return 0;
}