/*****************************************************************
 * BUAA Fall 2021 Fundamentals of Computer Hardware
 * Project7 Assembler and Linker
 *****************************************************************
 * my_linker_utils.c
 * Linker Submission
 *****************************************************************/
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "lib/tables.h"
#include "linker-src/linker_utils.h"

/*
 * Builds the symbol table and relocation data for a single file.
 * Read the .data, .text, .symbol, .relocation segments in that order.
 * The size of the .data and .text segments are read and saved in the
 * relocation table of the current file. For the .symbol and .relocation
 * segments, save the symbols in them in the corresponding locations.
 *
 * Return value:
 * 0 if no errors, -1 if error.
 *
 * Arguments:
 * input:            file pointer.
 * symtbl:           symbol table.
 * reldt:            pointer to a Relocdata struct.
 * base_text_offset: base text offset.
 * base_data_offset: base data offset.
 */
int fill_data(FILE *input, SymbolTable *symtbl, RelocData *reldt, uint32_t base_text_offset, uint32_t base_data_offset) {
    char buf[LINKER_BUF_SIZE];
    char* token;
    int ret=0;
    while(fgets(buf,LINKER_BUF_SIZE,input)){
        token=strtok(buf,LINKER_IGNORE_CHARS);
        if(strcmp(token,".data")==0){
            reldt->data_size=calc_data_size(input);
        }
        else if(strcmp(token,".text")==0){
            reldt->text_size=calc_text_size(input);
        }
        else if(strcmp(token,".symbol")==0){
            if(add_to_symbol_table(input,symtbl,base_text_offset,base_data_offset)==-1) ret=-1;
        }
        else if(strcmp(token,".relocation")==0){
            if(add_to_symbol_table(input,reldt->table,0,0)==-1) ret=-1;
        }
    }
    return ret;


    
}

/*
 * Detect whether the given instruction with offset OFFSET needs relocation.
 *
 * Return value:
 * 1 if the instruction needs relocation, 0 otherwise.
 *
 * Arguments:
 * offset: Address offset of the instruction in the current file.
 * reltbl: relocation table corresponding to the current file.
 */
int inst_needs_relocation(SymbolTable *reltbl, uint32_t offset) {
    if(get_symbol_for_addr(reltbl,offset)!=NULL) return 1;
    return 0;
}

/*
 * Given an instruction that needs relocation, relocate the instruction based on
 * the given symbol and relocation table.
 *
 * Note that we need to handle the relocation symbols for the .data segment and
 * the relocation symbols for the .text segment separately.
 *
 * For the .text segment, the symbols will only appear in the jump instruction
 *
 * For .data, it will only appear in the lui and ori instructions,
 * and we have processed it in the assembler as label@Hi/Lo
 *
 * You should return error if:
 * (1). the addr is not in the relocation table.
 * (2). the symbol name is not in the symbol table.
 * Otherwise, assume that the relocation will perform successfully.
 *
 * Return:
 * the relocated instruction, -1 if error.
 *
 * Arguments:
 * inst: an instruction that needs relocate.
 * offset: the byte offset of the instruction in the current file.
 * symtbl: the symbol table.
 * reltbl: the relocation table.
 */
int32_t relocate_inst(uint32_t inst, uint32_t offset, SymbolTable *symtbl, SymbolTable *reltbl){
    char* name;
    char* tempname;
    int64_t addr;
    /*
    通过偏移地址可以从reltbl(重定位表)中获得这个指令对应的标签
    事实上my_linker_utils中inst_needs_relocation的也可以直接通过这个来判断
    */
    name=get_symbol_for_addr(reltbl,offset);
    if(name==NULL){
        return -1;
    }
    /*
    获得name之后，有两种情况：
    1.Label来自于.data段，只会是在lui和ori指令中出现，这样的Label会有@Hi/Lo
    2.Label来自于.text段，只会是在j型指令中出现，这样的Label只有一个名字

    我们需要对name进行处理才能在汇总的全局符号表里面找到对应的地址。
    这里有个坑，如果读注释不太仔细的话可能注意不到。
    在fill_data中，我们用了add_to_symboltable()来自动两个向表中加name和addr，在add_to_symboltable()运行时，它会把
    来自于.text段的Label的%去掉，再加入到表中，所以我们在查找时，并不需要重新把%添加进去，处理掉@之后部分就可以查找。
    而对于.data段的Label,不需要进行处理。
    */
    tempname=create_copy_of_str(name);
    if(name[strlen(name)-3]=='@'){
        tempname[strlen(name)-3]='\0';   //去掉@之后的部分
    }
    /*
    处理好了name,就可以用name在汇总的symtbl(全局符号表)里面查找相应的addr，这里得到的已经是绝对地址了
    因为程序运行到这里，已经跑完了fill_data部分，把symtbl整合在一起并且得到的是绝对地址。

    后面的操作就很简单了，如果你看不太懂的话，可能是对重定位知识掌握的不太好，建议再看看指导书和PPT（任何有的资料）
    思考两个问题：
    1.为什么对于j类指令addr需要/4?
    2.把la指令翻译成lui+ori之后，具体需要干什么，不妨在纸上写一写。
    比如：
    la $v1 num
    会变成
    lui $at num@Hi
    ori $v1 $at num@Lo
    为了达到la的效果，下面两条指令应该是什么样的机器码？
    */
    addr=get_addr_for_symbol(symtbl,tempname);
    if(name[strlen(name)-3]!='@'){
        inst+=(addr/4);
    }
    else{
        //加载地址高16位，直接右移16位即可，更保险的做法是addr&=0xffff0000取高16位
        //其他变量全部跟他要用的位数跟1与，不用的跟0与，比较保险（
        if(name[strlen(name)-2]=='H'){
            inst+=(addr>>16);
        }
        else{
            addr&=0x0000ffff;
            inst+=addr;
        }
    }
    return inst;
}