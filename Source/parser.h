#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>
#include <assert.h>
#include "lexer.h"

enum ValueFor {
	CONDITION,
	COMMAND,
	CALL	
};

struct Value {
	char* string_val;
	uint8_t size;
	enum TokenType value_type;
	int pointer_count;
	bool uses_lea;
};
struct Syscall {
	uint8_t arg_count;
	struct Value* syscall_args;
};
struct Command {
	enum TokenType action;
	struct Value* value;
	struct Syscall* syscall;
};
struct Condition {
	enum TokenType condition;
	struct Value* value;
	enum TokenType relation;
};
struct Block {
	int id;
	
	struct Command* commands;
	int commands_length;

	struct Block* p_block;
	struct Block* c_block;
	struct Block* s_block;
	
	struct Condition* child_condition;
};

struct Token* parse_list;
int current_parse_element = 0;
int block_count = 0;

void expect(enum TokenType token_type) {
	enum TokenType actual_token = parse_list[current_parse_element].token_val;
	if (token_type != actual_token) {
		printf("Syntax Error: Expecting token %s instead got %s with value '%s'\n", token_names[token_type], token_names[actual_token], parse_list[current_parse_element].string_val); 
		exit(1);
	}
	current_parse_element++;
	//printf("Current element is now %s with %s\n", token_names[parse_list[current_parse_element].token_val], parse_list[current_parse_element].string_val);
}

void parseActualValue(struct Block* this_block, enum ValueFor action) {
	//printf("Parsing actual value\n");
	enum TokenType current_token = parse_list[current_parse_element].token_val;
	char* string_val = parse_list[current_parse_element].string_val;

	switch (current_token) {
		case STRING:
			expect(STRING);
			break;
		case CHAR:
			expect(CHAR);
			break;
		case BYTE:
			expect(BYTE);
			break;
		case NUMBER:
			expect(NUMBER);
			break;
		default:
			expect(NUMBER);
			break;
	}
	if (action == COMMAND) {
		(this_block->commands + this_block->commands_length)->value->string_val = string_val;
		(this_block->commands + this_block->commands_length)->value->value_type = current_token;
	} else if (action == CONDITION) {
		this_block->child_condition->value->string_val = string_val;
		this_block->child_condition->value->value_type = current_token;
	} else {
		struct Command* this_command = (this_block->commands + this_block->commands_length); 
		(this_command->syscall->syscall_args + (this_command->syscall->arg_count - 1))->string_val = string_val;
		(this_command->syscall->syscall_args + (this_command->syscall->arg_count - 1))->value_type = current_token;
	}
}
void parseBrackValue(struct Block* this_block, enum ValueFor action) {
	//printf("Parsing brack value\n")
	enum TokenType current_token = parse_list[current_parse_element].token_val;
	if (current_token == LBRACK) {
		expect(LBRACK);
		if (action == COMMAND) {
			(this_block->commands + this_block->commands_length)->value->pointer_count++;
		} else if (action == CONDITION) {
			this_block->child_condition->value->pointer_count++;
		} else {
			struct Command* this_command = (this_block->commands + this_block->commands_length); 
			(this_command->syscall->syscall_args + (this_command->syscall->arg_count - 1))->pointer_count++;
		}
		//parseBrackValue(this_block, action);
		parseActualValue(this_block, action); // Until a usecase is found or if someone wants it, I am not allowing multiple dereferencing. This can be done with cpy pretty easily now. Technically though you just need to comment this out and uncomment the above to make them allowed again.
		expect(RBRACK);
	} else {
		parseActualValue(this_block, action);
	}
}


void parseValue(struct Block* this_block, enum ValueFor action) {
	//printf("Parsing value\n");
	enum TokenType current_token = parse_list[current_parse_element].token_val;
	if (action == COMMAND) {
		(this_block->commands + this_block->commands_length)->value = malloc(sizeof(struct Value));
	} else if (action == CONDITION) {
		this_block->child_condition->value = malloc(sizeof(struct Value));
	} else {
		// SYSCALL 
		struct Command* this_command = (this_block->commands + this_block->commands_length); 
		if (this_command->syscall->arg_count == 0) {
			this_command->syscall->syscall_args = malloc(sizeof(struct Value));
		} else {
			this_command->syscall->syscall_args = realloc(this_command->syscall->syscall_args, (this_command->syscall->arg_count + 1) * sizeof(struct Value));
		}
		this_command->syscall->arg_count++;
	}
	if (action == COMMAND) {
		(this_block->commands + this_block->commands_length)->value->uses_lea = false;
		(this_block->commands + this_block->commands_length)->value->pointer_count = 0;
	} else if (action == CONDITION) {
		this_block->child_condition->value->uses_lea = false;
		this_block->child_condition->value->pointer_count = 0;
	} else {
		// SYSCALL
		struct Command* this_command = (this_block->commands + this_block->commands_length); 
		(this_command->syscall->syscall_args + (this_command->syscall->arg_count - 1))->uses_lea = false;
		(this_command->syscall->syscall_args + (this_command->syscall->arg_count - 1))->pointer_count = 0;
	}
	
	
	if (current_token == LBRACE) {
		expect(LBRACE);
		if (action == COMMAND) {
			(this_block->commands + this_block->commands_length)->value->uses_lea = true;
		} else if (action == CONDITION) {
			this_block->child_condition->value->uses_lea = true;
		} else {
			// SYSCALL
			struct Command* this_command = (this_block->commands + this_block->commands_length); 
			(this_command->syscall->syscall_args + (this_command->syscall->arg_count - 1))->uses_lea = true;
		}
		parseBrackValue(this_block, action);
		expect(RBRACE);
	} else {
		parseBrackValue(this_block, action);
	}
}

void parseExpression(struct Block* this_block) {
	//printf("Parsing expression\n");
	enum TokenType current_token = parse_list[current_parse_element].token_val;
	if (current_token == NOT || current_token == GREATER || current_token == GREATER_EQUAL || current_token == LESS || current_token == LESS_EQUAL) {
		this_block->child_condition->relation = current_token;
		expect(current_token);
		parseValue(this_block, CONDITION);
	} else {
		this_block->child_condition->relation = EQUAL;
		parseValue(this_block, CONDITION);
	}
}

void parseDecision(struct Block* this_block) {
	//printf("Parsing decision\n");
	enum TokenType current_token = parse_list[current_parse_element].token_val;
	if (current_token == IF || current_token == WHILE) {
		this_block->child_condition->condition = current_token;
		expect(current_token);
	} else {
		expect(IF);
	}
}

void parseAction(struct Block* this_block) {
	//printf("Parsing action\n");
	enum TokenType current_token = parse_list[current_parse_element].token_val;
	(this_block->commands + (this_block->commands_length))->action = current_token;
	(this_block->commands + (this_block->commands_length))->syscall = NULL;
	//printf("This is %p\n", (this_block->commands + (sizeof(struct Command) * this_block->commands_length)));
	if (current_token == SET || current_token == ADD || current_token == SUB || current_token == GO || current_token == GOR || current_token == GOL || current_token == ALLOC || current_token == DEALLOC || current_token == CPY) {
		expect(current_token);
	} else {
		expect(SET);
	}
}

void parseSyscallArgs(struct Block* this_block) {
	enum TokenType current_token = parse_list[current_parse_element].token_val;
	if (current_token == LBRACE || current_token == LBRACK || current_token == BYTE || current_token == CHAR || current_token == NUMBER) {
		parseValue(this_block, CALL);
		parseSyscallArgs(this_block);
	} else {
		return;
	}
}

void parseSyscall(struct Block* this_block) {
	(this_block->commands + (this_block->commands_length))->action = SYSCALL;
	(this_block->commands + (this_block->commands_length))->value = NULL;
	expect(SYSCALL);
	
	(this_block->commands + (this_block->commands_length))->syscall = malloc(sizeof(struct Syscall));
	(this_block->commands + (this_block->commands_length))->syscall->arg_count = 0;
	parseSyscallArgs(this_block);	
}

void parseBlock(struct Block* parent_block, struct Block* this_block) {
	//printf("Parsing block\n");
	if (this_block == NULL) {
		this_block = malloc(sizeof(struct Block));
	}	
	this_block->id = block_count;
	block_count++;

	this_block->c_block = NULL;
	if (parent_block != NULL) {
		parent_block->c_block = this_block;
	}
	this_block->p_block = parent_block;
	this_block->s_block = NULL;

	this_block->child_condition = NULL;
	this_block->commands = NULL;
	this_block->commands_length = 0;
	enum TokenType current_token = parse_list[current_parse_element].token_val;
	while (!(current_token == END_OF_FILE || current_token == RBRACE)) {
		if (current_token == IF || current_token == WHILE) {
			struct Block* condition_block = malloc(sizeof(struct Block));
			condition_block->child_condition = malloc(sizeof(struct Condition));
			//printf("Condition block is at %p\n", condition_block);		
			parseDecision(condition_block);
			parseExpression(condition_block);
			expect(LBRACE);
			
			condition_block->id = block_count;
			block_count++;

			struct Block* child_block = malloc(sizeof(struct Block));
			parseBlock(condition_block, child_block);
			//printf("About to leave LBRACE\n");
			condition_block->c_block = child_block;
			expect(RBRACE);

			struct Block* sibling_block = (struct Block*)malloc(sizeof(struct Block));
			parseBlock(parent_block, sibling_block);
			this_block->s_block = condition_block;
			condition_block->s_block = sibling_block;
			break;
		} else if (current_token == SET || current_token == SUB || current_token == ADD || current_token == GO || current_token == GOR || current_token == GOL || current_token == ALLOC || current_token == DEALLOC || current_token == CPY) {
			if (this_block->commands == NULL) {
				this_block->commands = (struct Command*)malloc(sizeof(struct Command));
			} else {
				//printf("Calling from here %d\n", this_block->commands_length);
				this_block->commands = realloc(this_block->commands, (this_block->commands_length + 1) * sizeof(struct Command));
			}
			parseAction(this_block);
			parseValue(this_block, COMMAND);
			this_block->commands_length++;
		} else if (current_token == SYSCALL) {
			if (this_block->commands == NULL) {
				this_block->commands = (struct Command*)malloc(sizeof(struct Command));
			} else {
				this_block->commands = realloc(this_block->commands, (this_block->commands_length + 1) * sizeof(struct Command)); // Check if neccesary to multiply by sizeof(struct Command).
			}
			parseSyscall(this_block);
			this_block->commands_length++;
		} else {
			expect(IF);
		}
		current_token = parse_list[current_parse_element].token_val;
	}
}

struct Block* parse() {
	//printf("Parsing start...\n");
	struct Block* start_block = malloc(sizeof(struct Block));
	parseBlock(NULL, start_block);
	return start_block;
}
#endif
