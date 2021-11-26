#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define ELEMENT_MAX_SIZE 30
#define MAX_LINES 1000
#define UNIVERZUM_ALLOC 1024 // How many 'pointer sizes' are allocated when
                             // expanding univerzum size. TODO: For smaller
                             // values was getting segmentation error, maybe
                             // look into that.

typedef struct univerzum
{
    unsigned len;    // Number of elements.
    unsigned size;   // How much space was allocated for elements (number of
                     // possible pointers, not size in bytes).
    char **elements; // Dynamically allocated array of strings.
} Univerzum;

void univerzum_ctor(Univerzum *uni);
void univerzum_dtor(Univerzum *uni);
void univerzum_expand(Univerzum *uni);
void univerzum_add_element(Univerzum *uni, char element[]);
char *univerzum_get_element(Univerzum *uni, char element[]);

/************************************************************/

typedef struct relation
{
    char *first;
    char *second;
} Relation;

typedef struct set
{
    bool contains_relations; // Elements are treated differently
    char **elements;         // Pointers to elements in univerzum
    unsigned len;            // Useful for printing
    unsigned size;           
} Set;

void set_init(Set *set, bool realtion_set, char **elements, unsigned len);
void set_print(Set *set);

/************************************************************/

typedef enum
{
    def_univerzum = 85, // ord value of U
    def_set = 83,       // ord value of S
    def_relation = 82,  // ord value of R
    exe_command = 67    // ord value of C
} Operation;

typedef struct
{
    Operation operation;
    Set *related_set; // pro U - množina všech prvků
                      // pro S R - definovaný set
                      // pro C (prémiové řešení) - co je výsledek
                      // Možnost - budou dva speciální sety které budou
                      // reprezentovat TRUE a FALSE, nemusí se pak dělat další
                      // proměná tady aby se dalo zjistit jestli je to T/F
                      // operace

    // Command command;

    unsigned first_argument;  // pro U S R bude 0, jinak číslo řádku (0vý řádek neexistuje)
    unsigned second_argument; // ---------||--------, pro příkazy co berou jeden taky 0
    unsigned parameter;       // Nko z prémiového řešení
} Line;

void line_ctor(Line *line);
void line_dtor(Line *line);
void line_link_set(Line *line, Set *set);

/************************************************************/

typedef int (*LineParser)(FILE *input, Line *target);

typedef struct operation_parser
{
    Operation operation;
    LineParser parser;
} OperationParser;

int parse_line(unsigned index, FILE *input, Line *target);
int parse_univerzum(FILE *input, Line *target);
int parse_set(FILE *input, Line *target);

FILE *open_input_file(int argc, char **argv);
int load_word(FILE *input, char *target, unsigned *len, unsigned maxlen);
bool is_letter(char ch);

static Line lines[MAX_LINES + 1]; // Lines are 1-indexed
static Univerzum univerzum;
static OperationParser operation_parsers[] = {
    {def_univerzum, &parse_univerzum},
    {def_set, &parse_set},
    {0, NULL}};


int set_add_element(Set *set, Univerzum *uni, char element[]);
void print_bool(bool b);

void set_union(Set *set1, Set *set2, Univerzum *uni);
void set_intersect(Set *set1, Set *set2, Univerzum *uni);

int main(int argc, char **argv)
{
    FILE *input_file = open_input_file(argc, argv);
    univerzum_ctor(&univerzum);

    for (int line_index = 1; line_index <= MAX_LINES; line_index++)
    {
        Line line;
        line_ctor(&line);

        if (parse_line(line_index, input_file, &line))
            break;

        set_print(line.related_set);
        printf("\n");
        lines[line_index] = line;
    }

    set_union(lines[2].related_set,lines[3].related_set, &univerzum);
    set_intersect(lines[4].related_set,lines[3].related_set, &univerzum);

    fclose(input_file);
}

/**
 * @brief Loads next word (string containing letters of english alphabet) from
 * an input file.
 *
 * @param input input stream to be continued reading
 * @param target where the loaded element terminated with '\\0'is stored
 * @param len where the lenght of loaded word is stored
 * @param maxlen maximal length of an element, not counting the '\\0' char
 * @return int-parsed char following loaded word. If loading is stopped due to
 * reaching maxlen then the first overreaching char is returned
 */
int load_word(FILE *input, char *target, unsigned *len, unsigned maxlen)
{
    char character;
    unsigned index = 0;

    while (index <= maxlen)
    {
        if (!is_letter(character = fgetc(input)))
            break;

        target[index++] = character;
    }

    target[index] = '\0';
    *len = index;
    return character;
}

/**
 * @brief Checks if character is letter of english alphabet.
 *
 * @param ch char to be checked.
 * @return bool is valid.
 */
bool is_letter(char ch)
{
    return ('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z');
}

/**
 * @brief Opens input file based on program arguments. Exits program on invalid
 * arguments or when the file is not found.
 *
 * @param argc
 * @param argv
 * @return FILE* pointer to input file.
 */
FILE *open_input_file(int argc, char **argv)
{
    FILE *input_file = NULL;

    if (argc != 2)
    {
        fprintf(stderr, "Invalid number of arguments\n");
        exit(1);
    }

    input_file = fopen(argv[1], "r");

    if (input_file == NULL)
    {
        fprintf(stderr, "Cannot open file '%s'\n", argv[1]);
        exit(1);
    }

    return input_file;
}

// TODO docs
int parse_line(unsigned index, FILE *input, Line *target)
{
    LineParser parser = NULL;
    char character = fgetc(input);
    fgetc(input);

    for (int i = 0; operation_parsers[i].operation != 0; i++)
        if ((Operation)character == operation_parsers[i].operation)
            parser = operation_parsers[i].parser;

    if (parser == NULL)
        return 1;

    return (*parser)(input, target);
}

int parse_univerzum(FILE *input, Line *target)
{
    unsigned len;
    char last_char, element[ELEMENT_MAX_SIZE + 1];

    while (1)
    {
        last_char = load_word(input, element, &len, ELEMENT_MAX_SIZE);
        univerzum_add_element(&univerzum, element);

        if (last_char == EOF || last_char == '\n')
            break;

        else if (last_char != ' ' || len == 0)
            return 1;
    }

    Set all_elemets;
    set_init(&all_elemets, false, univerzum.elements, univerzum.len);
    line_link_set(target, &all_elemets);

    return 0;
}

int parse_set(FILE *input, Line *target)
{
    unsigned len, index = 0;
    char last_char, element[ELEMENT_MAX_SIZE + 1];
    char *set_elements[univerzum.len];

    while (1)
    {
        last_char = load_word(input, element, &len, ELEMENT_MAX_SIZE);
        set_elements[index] = univerzum_get_element(&univerzum, element);

        if (last_char == EOF || last_char == '\n')
            break;

        else if (last_char != ' ' || len == 0 || set_elements[index++] == NULL)
            return 1;
    }

    Set set;
    set_init(&set, false, set_elements, ++index);
    line_link_set(target, &set);

    return 0;
}

// Inializes univezum values.
void univerzum_ctor(Univerzum *uni)
{
    uni->len = 0;
    uni->size = 0;
    univerzum_expand(uni);
}

// Destructs uiverzum.
void univerzum_dtor(Univerzum *uni)
{
    for (unsigned i = 0; i < uni->len; i++)
        free(uni->elements[i]);

    free(uni->elements);
}

// Expands the allocated memory of univerzum.
void univerzum_expand(Univerzum *uni)
{
    uni->size += UNIVERZUM_ALLOC;
    uni->elements = realloc(uni->elements, uni->size * sizeof(char *));

    if (uni->elements == NULL)
    {
        fprintf(stderr, "Memory allocation failed");
        exit(1);
    }
}

// Adds string to the univerzum.
// TODO: Pořešit tohle:
//   "...Prvky univerza nesmí obsahovat identifikátory příkazů a klíčová slova true a false... "
//   "...Délka řetězce je maximálně 30 znaků..."
//   "...Pokud se prvek v množině nebo dvojice v relaci opakuje, jedná se o chybu..." musí se
//   kontrolovat i tady, ne jen při definici množin, v množinách se pak porovanjí jen ukazatele
//   mezi sebou
void univerzum_add_element(Univerzum *uni, char element[])
{
    if (univerzum_get_element(uni, element) != NULL)
        return; // TODO Maybe should throw an error

    int el_size = strlen(element) + 1;
    char **element_p = &(uni->elements[(uni->len)++]);
    *element_p = malloc(el_size * sizeof(char));

    if (*element_p == NULL)
    {
        fprintf(stderr, "Memory allocation failed.");
        exit(1);
    }

    if (uni->len >= uni->size)
        univerzum_expand(uni);

    for (int i = 0; i <= el_size; i++)
        (*element_p)[i] = element[i];
}

// Returns pointer to given element in univerzum. If no such element is
// contained, returns NULL instead.
char *univerzum_get_element(Univerzum *uni, char element[])
{
    for (unsigned i = 0; i < uni->len; i++)
        if (!strcmp(uni->elements[i], element))
            return uni->elements[i];
    return NULL;
}

void set_init(Set *set, bool realtion_set, char **elements, unsigned len)
{
    set->contains_relations = realtion_set;
    set->elements = malloc(sizeof(char **) * len);
    set->len = len;
    set->size = 0;
    for (int index = 0; index < len; index++)
        set->elements[index] = elements[index];
}

void set_print(Set *set)
{
    printf("%c ", set->contains_relations ? def_relation : def_set);

    for (int i = 0; i < set->len; i++)
        if (!set->contains_relations)
            printf("%s ", set->elements[i]);
}

void line_ctor(Line *line)
{
    line->first_argument = 0;
    line->second_argument = 0;
    line->parameter = 0;
    line->operation = 0;
    line->related_set = NULL;
}

void line_dtor(Line *line)
{
    if (line->related_set != NULL)
        free(line->related_set);
}

void line_link_set(Line *line, Set *set)
{
    line->related_set = malloc(sizeof(*set));
    *(line->related_set) = *set;
}

/**
 * @brief Adds an element to a set
 * 
 * @param set set, we want the element to be added to
 * @param uni universe containing the element
 * @param element element
 * @return 1 if realloc failed, 0 if succesful
 */

int set_add_element(Set *set, Univerzum *uni, char element[]) {
    set->elements = realloc(set->elements, set->len + sizeof(char *));
    if(set->elements == NULL) {
        fprintf(stderr,"Memory allocation failed");
        return 1;
    }
    set->elements[set->len] = univerzum_get_element(uni, element);
    set->len++;
    return 0;
}

//prints the bool value
void print_bool(bool b) {
    printf(b ? "true" : "false");
}

/**
 * @brief Prints the union of two sets
 * 
 * @param set1 1.set
 * @param set2 2.set
 * @param uni Universe containing elements of the two sets
 */
void set_union(Set *set1, Set *set2, Univerzum *uni) {
    Set union_set = *set1;
    //goes through the second set and if an element from it is not in the first set, adds it to the union 
    for(unsigned i = 0; i < set2->len; i++) {
        bool el_in_both = false;
        unsigned j = 0;
        for(; j < set1->len; j++) {
            if(set2->elements[i] == set1->elements[j]) {
                el_in_both = true;
                break;
            }    
        }
        if(!el_in_both) {
            set_add_element(&union_set, uni, set2->elements[i]);
        }
    }
    set_print(&union_set);
    printf("\n");
}

/**
 * @brief Prints the intersect of two sets
 * 
 * @param set1 1.set
 * @param set2 2.set
 * @param uni Universe containing elements of the two sets
 */
void set_intersect(Set *set1, Set *set2, Univerzum *uni) {
    Set intersect_set;
    set_init(&intersect_set, false, NULL, 0);
    for(unsigned i = 0; i < set1->len; i++) {
        unsigned j = 0;
        for(; j < set2->len; j++) {
            if(set1->elements[i] == set2->elements[j]) {
                set_add_element(&intersect_set, uni, set1->elements[i]);
                break;
            }    
        }
    }
    set_print(&intersect_set);
    printf("\n");
}