#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>

#include "lexer.h"
#include "parser.h"
#include "compiler.h"

const char* usage = "Usage: tur [arguments] [-o file] file\nArguments:\n--ki: Keep intermediate representation (.s file). Equivalent to --keep-intermediate.\n--h: Display this help. Equivalent to --help.\n-oi file: Set name of intermediate representation file.\n";

int main(int argc, char** argv) {
	if (argc > 1) {
		char* return_file = "./out";
		char* intermediate_file = "./out.o";
		char* assembly_file = "./out.s";
		char* code_file_name = "";
		bool keep_intermediate = false;
		int expecting = 0;
		
		bool code_file_name_set = false;

		for (int i = 1; i < argc; i++) {
			if (!strncmp("-o", argv[i], strlen(argv[i]))) {
				expecting = 1;
			} else if(!strncmp("-oi", argv[i], strlen(argv[i]))) {
				expecting = 2;
			} else if (!strncmp("--ki", argv[i], strlen(argv[i])) || !strncmp("--keep-intermediate", argv[i], strlen(argv[i]))) {
				keep_intermediate = true;
			} else if (!strncmp("--h", argv[i], strlen(argv[i])) || !strncmp("--help", argv[i], strlen(argv[i]))) {
				puts(usage);
				exit(0);
			} else if (!(expecting == 0 && code_file_name_set)) {
				switch (expecting) {
					case 0:
						code_file_name = (argv[i]);
						code_file_name_set = true;
						break;
					case 1:
						return_file = (argv[i]);
						break;
					case 2:
						assembly_file = (argv[i]);
						break;
				}
				expecting = 0;
			} else {
				printf("Unknown argument '%s'. Use --h for help.\n", argv[i]);
				exit(1);
			}
		}
		if (expecting) {
			printf("ERROR: Expecting argument value %d\n", expecting);
			exit(1);
		}
		if (strlen(code_file_name) == 0) {
			printf("ERROR: You must provide a code file.");
			exit(1);
		}

		FILE* code_file = fopen(code_file_name, "r");
		
		if (code_file == NULL) {
			printf("ERROR: Unable to find file: '%s'\n", code_file_name);
			exit(2);
		}

		fseek(code_file, 0, SEEK_END);
		long size = ftell(code_file);
		char* buffer = malloc(size * sizeof(char));
		//printf("GETTING FILE SIZE: %ld\n", size);	
		fseek(code_file, 0, SEEK_SET);
		fread(buffer, 1, size, code_file);
		fclose(code_file);
		
		*(buffer+size-1) = '\x00';
		
		//printf("FILE\n%s\n\n", buffer);

		/*for (int i = 0; i < size; i++) {
			printf("- 0x%x", *(buffer+i));
		}*/
		
		struct Token* tokens = lexer(buffer);
		free(buffer);

		int i = 0;
		while (tokens[i].token_val != END_OF_FILE) {
			//printf("Getting token %s with value '%s'\n", token_names[tokens[i].token_val], tokens[i].string_val);
			i += 1;
		}
		//printf("Getting token %s with value '%s'\n", token_names[tokens[i].token_val], tokens[i].string_val);
		
		parse_list = tokens;
			
		struct Block* initial_block = parse();
		compile_list = tokens;	

		char* output;
		compile(&output, initial_block);
		//printf("\nOutput: \n%s\n", output);
		
		FILE* output_assembly = fopen(assembly_file, "w");
		fwrite(output, 1, strlen(output), output_assembly);
		fclose(output_assembly);
		
		free(output);
		// free(initial_block); Already freed by the compiler.
		
		char* temp_sys_string = malloc(300);
		sprintf(temp_sys_string, "/bin/as -o %s %s; /bin/ld -o %s %s; /bin/rm %s", intermediate_file, assembly_file, return_file, intermediate_file, intermediate_file);
		system(temp_sys_string);
		free(temp_sys_string);
		
		if (!keep_intermediate) {
			temp_sys_string = malloc(100);
			sprintf(temp_sys_string, "/bin/rm %s", assembly_file);
			system(temp_sys_string);
			free(temp_sys_string);
		}

		return 0;
	} else {
		printf("Got invalid arguments count: %d.\n", argc);
		return 1;
	}
}
