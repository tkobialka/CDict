#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INTERNAL_BUFFER_SIZE 100    // bytes

#define DICT_MAX_N_OF_KEYVALUES 1
#define IDENTIFIER_NAME_MAX_LENGTH 10   // overflow not handled!
#define KEY_MAX_LENGTH 10               // overflow not handled!
#define MAX_N_OF_DICTS 10               // overflow not handled!
#define VALUE_NAME_MAX_LENGTH 10        // overflow not handled!

#define IS_NEWLINE(c) ((c) == '\n')
#define IS_LEFT_BRACKET(c) ((c) == '[')
#define IS_RIGHT_BRACKET(c) ((c) == ']')
#define IS_EQUALITY(c) ((c) == '=')
#define IS_LETTER(c) (isalpha(c))
#define IS_NUMBER(c) (isdigit(c))
#define IS_SPACE(c) (isspace(c))

typedef enum {
    LEFT_STATE,
    READING_KEY_STATE,
    LEFT_HAVE_KEY_STATE,
    RIGHT_STATE
} LexerState;

typedef enum {
    NEWLINE_RESULT,
    LEFT_BRACKET_RESULT,
    RIGHT_BRACKET_RESULT,
    EQUALITY_RESULT,
    LETTER_RESULT,
    NUMBER_RESULT,
    SPACE_RESULT,
    PARSING_ERROR
} CharParseResult;

typedef enum {
    STREAM_PARSE_SUCCESS,
    STREAM_PARSE_ERROR
} StreamParseResult;

typedef enum {
    ASSIGN_KEY_SUCCESS,
    ASSIGN_KEY_MAX_REACHED
} AssignKeyResult;

typedef enum {
    STATEMENT_GET,
    STATEMENT_SET,
    STATEMENT_GET_ALL,
    STATEMENT_NO_ACTION
} StatementAction;

typedef struct {
    char *key;
    int value;
} KeyValue;

typedef struct {
    short int n_of_keys;
    KeyValue *keyvalues[DICT_MAX_N_OF_KEYVALUES];
} Dict;

typedef struct {
    char *id;
    Dict *dict;
} SymbolMap;

typedef struct {
    short int dict_n;
    SymbolMap *symbol_maps[MAX_N_OF_DICTS];
} DictTable;

typedef struct {
    StatementAction action;
    char *identifer_name;
    char *key;
    int value;
} Statement;

typedef struct {
    LexerState state;
    char *identifier_name;
    char *key_name;
    char *value_name;
    int identifier_name_position;
    int key_name_position;
    int value_name_position;
} StateHolder;

void print_promt(void);
void reset_state_holder(StateHolder *);

StatementAction parse_stream(char *, StateHolder *);
CharParseResult parse_char(char);

Statement *prepare_statement(StateHolder *, StatementAction);
void execute_statement(Statement *, DictTable *);

Dict *get_dict(char *, DictTable *);
AssignKeyResult assign_key(char [], int, Dict *);

int main(int argc, char *argv[]) {
    DictTable *dict_table = malloc(sizeof(DictTable));

    char *internal_buffer;
    size_t internal_buffer_size = INTERNAL_BUFFER_SIZE;

    internal_buffer = (char *)malloc(internal_buffer_size);
    
    StateHolder *state_holder = malloc(sizeof(StateHolder));

    while (true) {
        print_prompt();
        getline(&internal_buffer, &internal_buffer_size, stdin);

        reset_state_holder(state_holder);

        StatementAction statement_action = parse_stream(internal_buffer, state_holder);

        if (statement_action == STATEMENT_NO_ACTION) {
            printf("Statement ignored.\n");
            free(state_holder->identifier_name);
            free(state_holder->key_name);
            free(state_holder->value_name);
            continue;
        }
        
        state_holder->identifier_name[state_holder->identifier_name_position] = 0;
        state_holder->key_name[state_holder->key_name_position] = 0;
        state_holder->value_name[state_holder->value_name_position] = 0;

        Statement *statement = prepare_statement(state_holder, statement_action);
        
        execute_statement(statement, dict_table);

        free(statement);
    }

    return 0;
}

void print_prompt(void) { printf("> "); }

void reset_state_holder(StateHolder *state_holder) {
    state_holder->state = LEFT_STATE;
    state_holder->identifier_name = malloc(sizeof(char) * IDENTIFIER_NAME_MAX_LENGTH);
    state_holder->key_name = malloc(sizeof(char) * KEY_MAX_LENGTH);
    state_holder->value_name = malloc(sizeof(char) * VALUE_NAME_MAX_LENGTH);
    state_holder->identifier_name_position = 0, state_holder->key_name_position = 0, state_holder->value_name_position = 0;
}

CharParseResult parse_char(char ch) {
    if IS_NEWLINE(ch) {
        return NEWLINE_RESULT;
    } else if IS_LEFT_BRACKET(ch) {
        return LEFT_BRACKET_RESULT;
    } else if IS_RIGHT_BRACKET(ch) {
        return RIGHT_BRACKET_RESULT;
    } else if IS_EQUALITY(ch) {
        return EQUALITY_RESULT;
    } else if IS_LETTER(ch) {
        return LETTER_RESULT;
    } else if IS_NUMBER(ch) {
        return NUMBER_RESULT;
    } else if IS_SPACE(ch) {
        return SPACE_RESULT;
    } else return PARSING_ERROR;
}

StatementAction parse_stream(char * internal_buffer, StateHolder * state_holder) {
    for (int i = 0; i < INTERNAL_BUFFER_SIZE; i++) {
        char ch = internal_buffer[i];

        switch (parse_char(ch)) {
            case (LETTER_RESULT):
                if (state_holder->state == LEFT_STATE) state_holder->identifier_name[state_holder->identifier_name_position++] = ch;
                else if (state_holder->state == READING_KEY_STATE) state_holder->key_name[state_holder->key_name_position++] = ch;
                else return STATEMENT_NO_ACTION;
                break;
                
            case (LEFT_BRACKET_RESULT):
                if (state_holder->state == LEFT_STATE) state_holder->state = READING_KEY_STATE;
                else return STATEMENT_NO_ACTION;
                break;

            case (RIGHT_BRACKET_RESULT):
                if (state_holder->state == READING_KEY_STATE) state_holder->state = LEFT_HAVE_KEY_STATE;
                else return STATEMENT_NO_ACTION;
                break;
            
            case (EQUALITY_RESULT):
                if (state_holder->state == LEFT_HAVE_KEY_STATE) state_holder->state = RIGHT_STATE;
                else if (state_holder->state == READING_KEY_STATE) state_holder->key_name[state_holder->key_name_position++] = ch;
                else return STATEMENT_NO_ACTION;
                break;

            case (NUMBER_RESULT):
                if (state_holder->state == READING_KEY_STATE) state_holder->key_name[state_holder->key_name_position++] = ch;
                else if (state_holder->state == RIGHT_STATE) state_holder->value_name[state_holder->value_name_position++] = ch;
                else return STATEMENT_NO_ACTION;
                break;

            case (SPACE_RESULT):
                if (state_holder->state == READING_KEY_STATE) state_holder->key_name[state_holder->key_name_position++] = ch;
                else if (state_holder->state == LEFT_STATE) return STATEMENT_NO_ACTION;
                break;

            case (NEWLINE_RESULT):
                if (state_holder->state == LEFT_STATE) return STATEMENT_GET_ALL;
                else if (state_holder->state == LEFT_HAVE_KEY_STATE) return STATEMENT_GET;
                else if (state_holder->state == RIGHT_STATE) return STATEMENT_SET;
                else return STATEMENT_NO_ACTION;

            case (PARSING_ERROR):
                printf("parsing error\n");
                exit(EXIT_FAILURE);
        }
    }
}

Statement *prepare_statement(StateHolder *state_holder, StatementAction statement_action) {
    Statement *statement = malloc(sizeof(Statement));
    statement->action = statement_action;

    if (state_holder->identifier_name_position) {
        state_holder->identifier_name[state_holder->identifier_name_position] = 0;
        statement->identifer_name = state_holder->identifier_name;
    } else free(state_holder->identifier_name);

    if (state_holder->key_name_position) {
        state_holder->key_name[state_holder->key_name_position] = 0;
        statement->key = state_holder->key_name;
    } else free(state_holder->key_name);

    if (state_holder->value_name) {
        state_holder->value_name[state_holder->value_name_position] = 0;
        statement->value = atoi(state_holder->value_name);
    } else free(state_holder->value_name);

    return statement;
}

void execute_statement(Statement *statement, DictTable *dict_table) {
    char *symbol = statement->identifer_name;
    Dict *dict = get_dict(symbol, dict_table);
    if (dict == NULL) return;

    if (statement->action == STATEMENT_GET) {
        for (int i = 0; i < dict->n_of_keys; i++) {
            if (strcmp(statement->key, dict->keyvalues[i]->key) == 0) {
                printf("%d\n", dict->keyvalues[i]->value);
            }
        }
    } else if (statement->action == STATEMENT_SET) {
        assign_key(statement->key, statement->value, dict);
    } else if (statement->action == STATEMENT_GET_ALL) {
        if (!dict->n_of_keys) printf("{}\n");
        else {
            printf("{ ");
            int i;
            for (i = 0; i < dict->n_of_keys-1; i++) {
                printf("%s:%d, ", dict->keyvalues[i]->key, dict->keyvalues[i]->value);
            }
            printf("%s:%d }\n", dict->keyvalues[i]->key, dict->keyvalues[i]->value);
        }
    }
}

Dict *get_dict(char * symbol, DictTable *dict_table) {
    for (int i = 0; i < dict_table->dict_n; i++) {
        if (strcmp(symbol, dict_table->symbol_maps[i]->id) == 0) {
            return dict_table->symbol_maps[i]->dict;
        }
    }
    if (dict_table->dict_n >= MAX_N_OF_DICTS) {
        printf("Max number of dicts reached.\n");
        return NULL;
    }
    // Else create a new symbol map
    SymbolMap *symbol_map = malloc(sizeof(SymbolMap));
    symbol_map->id = symbol;
    symbol_map->dict = malloc(sizeof(Dict));
    symbol_map->dict->n_of_keys = 0;
    dict_table->symbol_maps[dict_table->dict_n++] = symbol_map;
    return symbol_map->dict;
}

AssignKeyResult assign_key(char key[], int value, Dict *dict) {
    // First, check if key already assigned
    for (int i = 0; i < dict->n_of_keys; i++) {
        if (strcmp(key, dict->keyvalues[i]->key) == 0) {
            dict->keyvalues[i]->value = value;
            return ASSIGN_KEY_SUCCESS;
        }
    }
    if (dict->n_of_keys >= DICT_MAX_N_OF_KEYVALUES) {
        printf("Max number of keys reached.\n");
        return ASSIGN_KEY_MAX_REACHED;
    }
    // Else append to dict
    KeyValue *kv = malloc(sizeof(KeyValue));
    kv->key = key;
    kv->value = value;
    dict->keyvalues[dict->n_of_keys++] = kv;

    return ASSIGN_KEY_SUCCESS;
}
