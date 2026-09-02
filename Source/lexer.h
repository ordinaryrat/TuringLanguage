#ifndef LEXER_H
#define LEXER_H

#include <regex.h>
#include <stdint.h>
#include <string.h>

enum TokenType {
	STAR,
	READ,
	ADD,
	STRING,
	CHAR,
	BYTE,
	NUMBER,
	SET,
	SUB,
	LBRACK,
	RBRACK,
	GO,
	GOR,
	GOL,
	WHILE,
	IF,
	ELSE,
	LBRACE,
	RBRACE,
	NOT,
	SYSCALL,
	ALLOC,
	DEALLOC,
	CPY,
	GREATER,
	GREATER_EQUAL,
	LESS,
	LESS_EQUAL,

	EQUAL, // keep these two at end
	END_OF_FILE, 
};

struct Token {
	enum TokenType token_val;
	char* string_val;

	int line_no;
	int character_number;
};

char* token_names[] = {
	"STAR", "READ", "ADD", "STRING", "CHAR", "BYTE", "NUMBER", "SET", "SUB", "LBRACK", "RBRACK", "GO", "GOR", "GOL", "WHILE", "IF", "ELSE", "LBRACE", "RBRACE", "NOT", "SYSCALL", "ALLOC", "DEALLOC", "CPY", "END_OF_FILE"
};
const int token_count = 28;

struct Token* lexer(char* input_text) {
	//printf("\n--- LEXING ---\n");

	struct Token* tokens = malloc(1 * sizeof(struct Token));
	int found_tokens = 0;

	char* str_regexes[] = {"\\*", "read", "add", "\"[^\"]*\"", "'[^\']'", "0?x([0-9]|[A-F]|[a-f]){1,16}", "[0-9]+", "set", "sub", "\\[", "\\]", "go", "gor", "gol", "while", "if", "else", "\\{", "\\}", "!", "syscall", "alloc", "dealloc", "cpy", ">", ">=", "<", "<="};
	regex_t regexes[token_count];
	
	for (int i = 0; i < token_count; i++) {
		int valid = regcomp(&regexes[i], str_regexes[i], REG_EXTENDED);
		
		if (valid) {
			//printf("invalid: %d, %d", valid, i);
			exit(1);
		}
	}

	char* s = input_text;
	int lengths[token_count];
	
	bool line_comment = false;
	bool multi_line_comment = false;
	while (*s != '\x00') {
		// //printf("Current char %c\n", *s);
		if (*s <= 32 || line_comment || multi_line_comment) {
			if (line_comment && *s == '\n') {
				line_comment = false;
			} else if (multi_line_comment && *s == '*') {
				if (*(s + 1) == '/') {
					multi_line_comment = false;
					s += 2;
					continue;
				}
			}
			s++;
			continue;
		}
		if (*s == '/') {
			if (*(s + 1) == '/') {
				line_comment = true;
				s += 2;
				continue;
			} else if (*(s + 1) == '*') {
				multi_line_comment = true;
				s += 2;
				continue;
			}
		} 
		////printf("Found %s", s);
		
		for (int i = 0; i < token_count; i++) {
			regmatch_t pmatch[1];
			if (!regexec(&regexes[i], s, 1, pmatch, 0) && !pmatch->rm_so) {
				lengths[i] = pmatch->rm_eo - pmatch->rm_so;
				// //printf("Finding token %s with length %d\n", token_names[i], pmatch->rm_eo - pmatch->rm_so);
			} else {
				lengths[i] = 0;
				// //printf("Unable to find token %s\n", token_names[i]);
			}
		}
		// //printf("S now is: \n%s\n", s);
	
		int max_val = 0;
		int max_val_index = 0;
		for (int i = 0; i < token_count; i++) {
			if (lengths[i] > max_val) {
				max_val = lengths[i];
				max_val_index = i;
			}
		}
		if (max_val == 0) {
			printf("Lexer Error: No matches found? (invalid character: 0x%x\n", *s);
			exit(1);
		}
		else {
			char* substring = (char*)malloc((max_val + 1) * sizeof(char));
			////printf("Found match! %d", max_val);
			////printf("Found %s", s);
			strncpy(substring, s, max_val);
			substring[max_val] = '\x00';
			////printf("\nMatching: %s\nOffset: %d\nLength: %d\nMatching substring: %s", str_regexes[max_val_index], 0, max_val, substring);
			
			s += max_val; 
			
			struct Token this_token;
			this_token.token_val = max_val_index;
			this_token.string_val = substring;
			tokens[found_tokens] = this_token; 
			
			found_tokens += 1;
			
			tokens = realloc(tokens, (found_tokens + 1) * sizeof(struct Token)); // Need to always have one open for EOF
			////printf("Max found for %s (%s) (%d)\n", token_names[max_val_index], substring, max_val);
		}
	}

	struct Token eof_token;
	eof_token.token_val = END_OF_FILE;
	eof_token.string_val = NULL;
	
	tokens[found_tokens] = eof_token; 
	
	for (int i = 0; i < token_count; i++) {
		regfree(&regexes[i]);
	}

	return tokens;
}
#endif
