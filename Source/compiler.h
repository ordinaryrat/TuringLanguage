#ifndef COMPILER_H
#define COMPILER_H

#include "lexer.h"
#include "parser.h"

struct Token* compile_list;
int current_compile_element = 0;

char str_jump_equals[] = "je";
char str_jump_not_equals[] = "jne";
char str_jump_greater[] = "ja";
char str_jump_greater_equals[] = "jae";
char str_jump_less[] = "jl";
char str_jump_less_equals[] = "jle";

char* syscall_args[] = {"rax", "rdi", "rsi", "rdx", "r10", "r8", "r9"};
char* syscall_args_b[] = {"al", "dil", "sil", "dl", "r10b", "r8b", "r9b"};

void recursivelyDeallocate(struct Block* this_block) {
	if (this_block == NULL) {
		return;
	}
	//printf("Current block: %d, %p. Sibling block: %p, Child block: %p, Condition: %p\n", this_block->id, this_block, this_block->c_block, this_block->s_block, this_block->child_condition);
	if (this_block->child_condition != NULL) {
		free(this_block->child_condition->value);
	}
	free(this_block->child_condition);
	if (this_block->commands_length > 0) {
		for (int i = 0; i < this_block->commands_length; i++) {
			if ((this_block->commands + i)->syscall != NULL) {
				free((this_block->commands + i)->syscall->syscall_args);
				free((this_block->commands + i)->syscall);
			} else if ((this_block->commands + i)->value != NULL) {
				free((this_block->commands + i)->value);
			}
		}
		free(this_block->commands);
	}

	recursivelyDeallocate(this_block->c_block);
	recursivelyDeallocate(this_block->s_block);

	free(this_block);
}

bool convertValue(struct Value* this_value, char** return_value, char** preamble) {
	if (this_value->value_type == NUMBER || this_value->value_type == BYTE) { // special considerations actually must apply for bytes > 0xff or numbers greater than 255 but we ignore that for now	
		*return_value = malloc(sizeof(this_value->string_val));
		strcpy(*return_value, this_value->string_val);
	}
	else if (this_value->value_type == CHAR) {
		*return_value = malloc(6);
		sprintf(*return_value, "0x%x", *(this_value->string_val + 1)); 
	} 	
	if (this_value->type == POINTER) {
		*preamble = malloc(80);
		sprintf(*preamble, "\n\tmov r15, 0\n\tmov r15b, BYTE PTR [r14-%s]", *return_value);
		
		free(*return_value);
		*return_value = malloc(5);
		sprintf(*return_value, "r15b");
	} else if (this_value->type == LEA) {
		*preamble = malloc(40);
		sprintf(*preamble, "\n\tlea r15, [r14-%s]", *return_value);

		free(*return_value);
		*return_value = malloc(4);
		sprintf(*return_value, "r15");
	}
	return (bool)!(this_value->type == VALUE);
}

void convertBlock(char* output, struct Block* this_block) {
	//printf("Doing block");
	//printf("Block at %p, parent: %p, sibling: %p, child: %p\n", this_block, this_block->p_block, this_block->s_block, this_block->c_block);
	char* label_string = malloc(15);
	sprintf(label_string, "\n\nlabel%d:", this_block->id);
	output = strcat(output, label_string);
	free(label_string);

	for (int i = 0; i < this_block->commands_length; i++) {
		struct Command* command = (this_block->commands + i);
		/*if (command->value != NULL && command->syscall == NULL) {
			printf("%d: %s %s\n", i, token_names[command->action], command->value->string_val);
		} else {
			printf("%d: %s\n", i, token_names[command->action]);
		}*/
		char* tmp; 
		char* preamble;
		char* string;

		bool clear_vars = true;
		switch (command->action) {
			case SET:
				if (command->value->value_type == STRING) {
					char* string_ptr = command->value->string_val;
					int string_length = strlen(string_ptr);
					//printf("string ptr: %s\n", string_ptr);
					char* tmp_string;
					for (int i = 1; i < string_length - 1; i++) {
						tmp_string = malloc(50);
						//printf("char is %c %d", *(string_ptr+i), *(string_ptr+i));
						sprintf(tmp_string, "\n\tmov BYTE PTR [r12+%d], 0x%x", i - 1, *(string_ptr+i)); 
						output = strcat(output, tmp_string);
						free(tmp_string);
					}
					clear_vars = false;
				} else {
					string = (char*)malloc(40);
					if (convertValue(command->value, &tmp, &preamble)) {
						output = strcat(output, preamble);
					}
					sprintf(string, "\n\tmov BYTE PTR [r12], %s", tmp);
					output = strcat(output, string);
				}
				break;
			case ALLOC:
				string = (char*)malloc(50);
				if (convertValue(command->value, &tmp, &preamble)) {
					output = strcat(output, preamble);
				}
				sprintf(string, "\n\tmov rax, %s\n\tcall alloc", tmp);
				output = strcat(output, string);
				break;
			case DEALLOC:
				string = (char*)malloc(50);
				if (convertValue(command->value, &tmp, &preamble)) {
					output = strcat(output, preamble);
				}
				sprintf(string, "\n\tmov rax, %s\n\tcall dealloc", tmp);
				output = strcat(output, string);
				break;
			case SYSCALL:
				for (uint8_t i = 0; i < command->syscall->arg_count; i++) {
					if (i > 7) {
						printf("WARNING: More than 7 arguments are provided for a syscall. Will still work but all beyond the 7th will be ignored");
						break;
					}
					preamble = NULL;
					tmp = NULL;
					string = (char*)malloc(30);
						
					if (convertValue((command->syscall->syscall_args + i), &tmp, &preamble)) {
						output = strcat(output, preamble);
					}
					if (!strncmp(tmp, "r15b", 4)) {
						sprintf(string, "\n\tmov %s, %s", syscall_args_b[i], tmp); 
					} else {
						sprintf(string, "\n\tmov %s, %s", syscall_args[i], tmp); 
					}
					output = strcat(output, string);
					free(string);
					free(preamble);
					free(tmp);
				}
				string = (char*)malloc(100);
				sprintf(string, "\n\tsyscall\n\tmov rsi, rax\n\tmov rax, 8\n\tcall alloc\n\tmov [r13+1], rsi\n"); // Need to capture and 'append' return value.
				output = strcat(output, string);
				free(string);
				break;
			case GOR: // Its flipped in reality because of stack direction. Going right means going down the stack.
				string = (char*)malloc(40);
				if (convertValue(command->value, &tmp, &preamble)) {
					output = strcat(output, preamble);
				}
				sprintf(string, "\n\tsub r12, %s", tmp);
				output = strcat(output, string);
				break;
			case GOL: 
				string = (char*)malloc(40);
				if (convertValue(command->value, &tmp, &preamble)) {
					output = strcat(output, preamble);
				}
				sprintf(string, "\n\tadd r12, %s", tmp);
				output = strcat(output, string);
				break;
			case GO: 
				string = (char*)malloc(120);
				if (convertValue(command->value, &tmp, &preamble)) {
					output = strcat(output, preamble);
				}
				if (!strncmp(tmp, "r15b", 4)) {
					sprintf(string, "\n\tmov rcx, r14\n\tsub rcx, r15\n\tmov r12, rcx");
				} else {
					sprintf(string, "\n\tlea r12, [r14-%s]", tmp);
				}	
				output = strcat(output, string);
				break;
			case ADD: 
				string = (char*)malloc(40);
				if (convertValue(command->value, &tmp, &preamble)) {
					output = strcat(output, preamble);
				}
				sprintf(string, "\n\tadd BYTE PTR [r12], %s", tmp);
				output = strcat(output, string);
				break;
			case SUB: 
				string = (char*)malloc(40);
				if (convertValue(command->value, &tmp, &preamble)) {
					output = strcat(output, preamble);
				}
				sprintf(string, "\n\tsub BYTE PTR [r12], %s", tmp);
				output = strcat(output, string);
				break;
			default:
				break;
		}
		if (command->action != SYSCALL && clear_vars) {
			free(tmp);
			free(preamble);
			free(string);
		}
		tmp = NULL;
		preamble = NULL;
		string = NULL;
	} 
	if (this_block->c_block != NULL) {
		//printf("Going to child\n");
		char* tmp;
		char* preamble;
		if (convertValue(this_block->child_condition->value, &tmp, &preamble)) {
			output = strcat(output, preamble);
		}
		char* temp_jump_type;
		switch (this_block->child_condition->relation) {
			case EQUAL:
				temp_jump_type = str_jump_equals;
				break;
			case NOT:
				temp_jump_type = str_jump_not_equals;
				break;
			case LESS:
				temp_jump_type = str_jump_less;
				break;
			case LESS_EQUAL:
				temp_jump_type = str_jump_less_equals;
				break;
			case GREATER:
				temp_jump_type = str_jump_greater;
				break;
			case GREATER_EQUAL:
				temp_jump_type = str_jump_greater_equals;
				break;
			default:
				break;
		}
		label_string = malloc(60);
		sprintf(label_string, "\n\tcmp BYTE PTR [r12], %s\n\t%s label%d\n\tjmp label%d", tmp, temp_jump_type, this_block->c_block->id, this_block->s_block->id);
		output = strcat(output, label_string);
		free(label_string);
		free(tmp);
		free(preamble);
		
		convertBlock(output, this_block->c_block);
		convertBlock(output, this_block->s_block);
	}
	else if (this_block->s_block != NULL) {
		label_string = malloc(25);
		sprintf(label_string, "\n\tjmp label%d", this_block->s_block->id);
		output = strcat(output, label_string);
		free(label_string);
		
		convertBlock(output, this_block->s_block);
	} else if (this_block->p_block != NULL) {
		label_string = malloc(25);
		if (this_block->p_block->child_condition->condition == WHILE) {
			sprintf(label_string, "\n\tjmp label%d", this_block->p_block->id);
		} else {
			sprintf(label_string, "\n\tjmp label%d", this_block->p_block->s_block->id);
		}
		output = strcat(output, label_string);
		free(label_string);
	}
}

void compile(char** output, struct Block* initial_block) {
	*output = malloc(3000);
	*output = strcpy(*output, ".intel_syntax noprefix\n\n.global _start\n\ndealloc:\n\tadd r13, rax\n\tmov BYTE PTR [r13], 0x3\n\tret\n\nalloc:\n\tsub r13, rax\n\tmov BYTE PTR [r13], 0x3\n\tret\n\n_start:\n\tlea r13, [rsp-24]\n\tmov r14, r13\n\tsub r13, 1\n\tmov BYTE PTR [r13], 0x3\n\tmov BYTE PTR [r13+1], 0x2\n\tmov r12, r13\n\tjmp label0\x00");

	convertBlock(*output, initial_block);
	
	strcat(*output, "\n\x00");
	
	recursivelyDeallocate(initial_block);
}

#endif
