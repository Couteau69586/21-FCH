/*****************************************************************
 * BUAA Fall 2021 Fundamentals of Computer Hardware
 * Project7 Assembler and Linker
 *****************************************************************
 * my_assembler_utils.c
 * Assembler Submission
 *****************************************************************/
#include "my_assembler_utils.h"
#include "assembler-src/assembler_utils.h"
#include "lib/translate_utils.h"

#include <string.h>
#include <stdlib.h>

/*
 * This function reads .data symbol from INPUT and add them to the SYMTBL
 * Note that in order to distinguish in the symbol table whether a symbol
 * comes from the .data segment or the .text segment, we append a % to the
 * symbol name when adding the .data segment symbol. Since only underscores and
 * letters will appear in legal symbols, distinguishing them by adding % will
 * not cause a conflict between the new symbol and the symbol in the assembly file.
 *
 * Return value:
 * Return the number of bytes in the .data segment.
 */
char* IGNORE_CHARS_RDS = " \f\n\r\t\v,():#";
#define lineN 100050
char line[lineN];

int read_data_segment(FILE *input, SymbolTable *symtbl) {	
	printf("Going into read_data_segment\n") ;
	int bytes = 0;
		
	fgets(line, lineN, input);
	if (strcmp(strtok(line, IGNORE_CHARS_RDS), ".data") != 0) {
        return -1;
    }

	char constChar[5] = "%";
	char name[lineN] = "%";
	fgets(line, lineN, input);
	skip_comment(line);
	while( strcmp(line, "\n") != 0 ){
		char *p = strtok(line, IGNORE_CHARS_RDS);
		int findName = 0, getSize = 0;
		int addErr = 0;
		while(p != NULL) {
			if(findName == 1 && getSize == 1)	break;
			if(*p >= 48 && *p <= 57 && getSize == 0){
				long int size = 0;
				//puts(p);
				int numErr = translate_num(&size, p, 0, 1e9);
				getSize = 1;
				if(numErr == 0){
					if(addErr == 0)
						bytes += size;
					else
						return -1;
				}
					
				printf("size: %d\n", size); 
			}
			else if(findName == 0){
				strcat(name, p);
				printf("name: %s\n", p);
				addErr = add_to_table(symtbl, name, bytes);
				//printf("addErr: %d\n", addErr);
				findName = 1; 
			}
			p = strtok(NULL, IGNORE_CHARS_RDS);
		}
		strcpy(name, constChar);
		fgets(line, lineN, input);
		skip_comment(line);
	}
	printf("Total bytes:%d\nEnd of read_data_segment\n", bytes);
	if(bytes == 0)
		return -1;
	else
    	return bytes;
}

/* Adds a new symbol and its address to the SymbolTable pointed to by TABLE.
 * ADDR is given as the byte offset from the first instruction. The SymbolTable
 * must be able to resize itself as more elements are added.
 *
 * Note that NAME may point to a temporary array, so it is not safe to simply
 * store the NAME pointer. You must store a copy of the given string.
 *
 * If ADDR is not word-aligned, you should call addr_alignment_incorrect() and
 * return -1. If the table's mode is SYMTBL_UNIQUE_NAME and NAME already exists
 * in the table, you should call name_already_exists() and return -1. If memory
 * allocation fails, you should call allocation_failed().And alloction_failed()
 * will print error message and exit with error code 1.
 *
 * Otherwise, you should store the symbol name and address and return 0.
 */
int add_to_table(SymbolTable *table, const char *name, uint32_t addr) {
	if( (table->mode) == SYMTBL_UNIQUE_NAME && get_addr_for_symbol(table, name) != -1){
		name_already_exists(name);
		return -1;
	}

	if(addr % 4 != 0){
		addr_alignment_incorrect();
		return -1;
	}

	char* nameCpy;
	nameCpy = create_copy_of_str(name);
	Symbol tmp;
	tmp.name = nameCpy, tmp.addr = addr;
	if(table->len == table->cap){
		Symbol* newPtr = (Symbol*) realloc(table->tbl, table->cap*sizeof(Symbol)*SCALING_FACTOR);
		if(!newPtr){
			allocation_failed();
			return -1;
		}
		table->cap = SCALING_FACTOR;
		table->tbl = newPtr;
	}
	table->tbl[table->len] = tmp;
	table->len++;
	return 0;
}

/*
 * Convert lui instructions to machine code. Note that for the imm field of lui,
 * it may be an immediate number or a symbol and needs to be handled separately.
 * Output the instruction to the **OUTPUT**.(Consider using write_inst_hex()).
 * 
 * Return value:
 * 0 on success, -1 otherwise.
 * 
 * Arguments:
 * opcode:     op segment in MIPS machine code
 * args:       args[0] is the $rt register, and args[1] can be either an imm field or 
 *             a .data segment label. The other cases are illegal and need not be considered
 * num_args:   length of args array
 * addr:       Address offset of the current instruction in the file
 */
int write_lui(uint8_t opcode, FILE *output, char **args, size_t num_args, uint32_t addr, SymbolTable *reltbl) {
    char* rt = args[0];
	printf("rt:%s\n", rt);
	char* imm_16 = args[1];
	if(!(imm_16[0] >= '0' && imm_16[0] <= '9')){
		char* p = strtok(imm_16, "@");
		p = strtok(NULL, "@");
		int pos = 0;
		if(strcmp(p, "Lo"))	pos = 1;
		int64_t labelAddr = get_addr_for_symbol(reltbl, imm_16);
		int low = labelAddr & 0xffff;
		int high = labelAddr >> 16;
		if(pos == 0)	sprintf(imm_16, "%016X", high);
		else	sprintf(imm_16, "%04X", low);
		printf("imm16:%s\n", imm_16);
	}
	char *ans = (char*) malloc(35);
	sprintf(ans, "%02X", opcode);
	strcat(ans, rt);
	//strcat()
	
	//translate_num()
	//write_inst_hex(output, ans);
	return 0;


	// int regname=0;
    // if((regname=translate_reg(args[0]))==-1){
    //     return -1;
    // }
    // long imm_16=0;
    // uint32_t instruction=0;
    // char *name;
    // //把args[1]转换为long,失败则说明arg1是个label
    // if(translate_num(&imm_16,args[1],0,0x7fffffff)==-1){
    //     //label则imm_16直接赋0
    //     imm_16=0;
    //     name=create_copy_of_str(args[1]);
    //     //去掉@之后的部分，采用\0截断
    //     name[strlen(name)-3]='\0';
    //     if(!is_valid_label(name)){
    //         raise_label_error(addr,name);
    //         return -1;
    //     }
    //     //加入到重定位表中
    //     add_to_table(reltbl,args[1],addr);
    // }
    // instruction=opcode;
    // instruction<<=10;
    // instruction+=regname;
    // instruction<<=16;
    // instruction+=imm_16;
    // write_inst_hex(output,instruction);
    // return 0;

}

