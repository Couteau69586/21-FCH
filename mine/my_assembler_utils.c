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
#define DEBUG (puts("gothere"))

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
int read_data_segment(FILE *input, SymbolTable *symtbl){ 

    char buf[ASSEMBLER_BUF_SIZE];
    char *token;
    char *name;
    int size_all=0;
    long space=0;
    uint32_t input_line=0;
    int valid=1;
    fgets(buf,ASSEMBLER_BUF_SIZE,input);
    token=strtok(buf,ASSEMBLER_IGNORE_CHARS);
    if(strcmp(token,".data")!=0){
        valid=0;
    }
    while(fgets(buf,ASSEMBLER_BUF_SIZE,input)){
        skip_comment(buf);
        input_line++;
        if(buf[0]=='\n'){
            break;
        }
        //读Label
        token=strtok(buf,ASSEMBLER_IGNORE_CHARS);
        if (token == NULL) {
            continue;
        }
        //如果Label以：结尾
        if(token[strlen(token)-1]==':'){
            token[strlen(token)-1]='\0';
            //如果Label合法
            if(!is_valid_label(token)){
                valid=0;
                raise_label_error(input_line,token);
                continue;
            }
        }
        //Label不以：结尾
        else{
            valid=0;
            raise_label_error(input_line,token);
            continue;
        }
        name=create_copy_of_str(token);
        name[0]='%';
        strcpy(name+1,token);

        //读.space
        token=strtok(NULL,ASSEMBLER_IGNORE_CHARS);
        if(strcmp(token,".space")!=0){
            valid=0;
            continue;
        }

        //读数字
        token=strtok(NULL,ASSEMBLER_IGNORE_CHARS);
        if(translate_num(&space,token,0,0x7fffffff)==-1){
            valid=0;
            continue;
        }
        if(space%4==0){
            if(add_to_table(symtbl,name,size_all)==0){
                size_all+=space;
            }
        }
    }
    return valid?size_all:-1;
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

    //没对齐
    if(addr%4!=0){
        addr_alignment_incorrect();
        return -1;
    }

    //不允许重复出现的情况
    if(table->mode==SYMTBL_UNIQUE_NAME&&get_addr_for_symbol(table,name)!=-1){
        name_already_exists(name);
        return -1;
    } 

    //大小达到最大容量
    if(table->len==table->cap){
        Symbol* newPtr=(Symbol*) realloc(table->tbl,table->cap*sizeof(Symbol)*SCALING_FACTOR);
        if(!newPtr){
            allocation_failed();
            return -1;
        }
        table->cap*=SCALING_FACTOR;
        table->tbl=newPtr;
    }

    //正常插入
    char* newname=create_copy_of_str(name);
    (table->tbl)[table->len]=(Symbol){newname,addr};
    (table->len)++;
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

    //翻译$rt寄存器
    int regname=0;
    if((regname=translate_reg(args[0]))==-1){
        return -1;
    }
    long imm_16=0;
    uint32_t instruction=0;
    char *name;
    //把args[1]转换为long,失败则说明arg1是个label
    if(translate_num(&imm_16,args[1],0,0x7fffffff)==-1){
        //label则imm_16直接赋0
        imm_16=0;
        name=create_copy_of_str(args[1]);
        //去掉@之后的部分，采用\0截断
        name[strlen(name)-3]='\0';
        if(!is_valid_label(name)){
            raise_label_error(addr,name);
            return -1;
        }
        //加入到重定位表中
        add_to_table(reltbl,args[1],addr);
    }
    instruction=opcode;
    instruction<<=10;
    instruction+=regname;
    instruction<<=16;
    instruction+=imm_16;
    write_inst_hex(output,instruction);
    return 0;

}


    

