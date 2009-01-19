#ifndef _SYNTAX_H_
#define _SYNTAX_H_
#include "config.h"
#include "header.h"
struct script_syntax{
	const char *name;
	int script_opcode;
	int permittion;
	int argc, compare;
	const int *argv_type;
};
enum{
	SYNTAX_ARGVTYPE_NULL,
	SYNTAX_ARGVTYPE_VALUE,
	SYNTAX_ARGVTYPE_HV,
	SYNTAX_ARGVTYPE_EXPRESSION,
	SYNTAX_ARGVTYPE_VARIABLE,
	SYNTAX_ARGVTYPE_CONSTANT,
	SYNTAX_ARGVTYPE_TRANSTYPE
};
enum{
	SYNTAX_COMPARE_EQ,
	SYNTAX_COMPARE_GT
};
enum{
	SYNTAX_ARGV_TYPE_NUM = 4
};
enum{
	SCRIPT_OPCODE_MAPPER,
	SCRIPT_OPCODE_MIRROR,
	SCRIPT_OPCODE_CPU_ROMSIZE,
	SCRIPT_OPCODE_CPU_RAMSIZE,
	SCRIPT_OPCODE_CPU_FLASHSIZE,
	SCRIPT_OPCODE_PPU_ROMSIZE,
	SCRIPT_OPCODE_PPU_FLASHSIZE,
	SCRIPT_OPCODE_DUMP_START,
	SCRIPT_OPCODE_CPU_COMMAND,
	SCRIPT_OPCODE_CPU_READ,
	SCRIPT_OPCODE_CPU_WRITE,
	SCRIPT_OPCODE_CPU_RAMRW,
	SCRIPT_OPCODE_CPU_PROGRAM,
	SCRIPT_OPCODE_PPU_COMMAND,
	SCRIPT_OPCODE_PPU_RAMFIND,
	SCRIPT_OPCODE_PPU_SRAMTEST,
	SCRIPT_OPCODE_PPU_READ,
	SCRIPT_OPCODE_PPU_WRITE,
	SCRIPT_OPCODE_PPU_PROGRAM,
	SCRIPT_OPCODE_STEP_START,
	SCRIPT_OPCODE_STEP_END,
	SCRIPT_OPCODE_DUMP_END,
	SCRIPT_OPCODE_COMMENT,
	SCRIPT_OPCODE_NUM
};
enum{
	PERMITTION_ROM_DUMP = 1 << MODE_ROM_DUMP,
	PERMITTION_RAM_READ = 1 << MODE_RAM_READ,
	PERMITTION_RAM_WRITE = 1 << MODE_RAM_WRITE,
	PERMITTION_ROM_PROGRAM = 1 << MODE_ROM_PROGRAM,
	PERMITTION_ALL = 0xffff
};
extern const char OPSTR_CPU_ROMSIZE[];
extern const char OPSTR_CPU_RAMSIZE[];
extern const char OPSTR_CPU_FLASHSIZE[];
extern const char OPSTR_PPU_ROMSIZE[];
extern const char OPSTR_PPU_FLASHSIZE[];
extern const char OPSTR_CPU_RAMRW[];
struct script;
int syntax_check(char **text, int text_num, struct script *s, int mode);
#endif
