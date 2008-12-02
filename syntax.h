//included from script.c only
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
	SYNTAX_ARGVTYPE_VARIABLE
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
	SCRIPT_OPCODE_PPU_ROMSIZE,
	SCRIPT_OPCODE_DUMP_START,
	SCRIPT_OPCODE_CPU_READ,
	SCRIPT_OPCODE_CPU_WRITE,
	SCRIPT_OPCODE_CPU_RAMRW,
	SCRIPT_OPCODE_CPU_PROGRAM,
	SCRIPT_OPCODE_PPU_RAMTEST,
	SCRIPT_OPCODE_PPU_READ,
	SCRIPT_OPCODE_PPU_WRITE,
	SCRIPT_OPCODE_PPU_PROGRAM,
	SCRIPT_OPCODE_STEP_START,
	SCRIPT_OPCODE_STEP_END,
	SCRIPT_OPCODE_WAIT,
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
static const char OPSTR_CPU_ROMSIZE[] = "CPU_ROMSIZE";
static const char OPSTR_CPU_RAMSIZE[] = "CPU_RAMSIZE";
static const char OPSTR_PPU_ROMSIZE[] = "PPU_ROMSIZE";
static const char OPSTR_CPU_RAMRW[] = "CPU_RAMRW";
static const int ARGV_TYPE_VALUE_ONLY[SYNTAX_ARGV_TYPE_NUM] = {
	SYNTAX_ARGVTYPE_VALUE, SYNTAX_ARGVTYPE_NULL,
	SYNTAX_ARGVTYPE_NULL, SYNTAX_ARGVTYPE_NULL
};
static const int ARGV_TYPE_HV[SYNTAX_ARGV_TYPE_NUM] = {
	SYNTAX_ARGVTYPE_HV, SYNTAX_ARGVTYPE_NULL,
	SYNTAX_ARGVTYPE_NULL, SYNTAX_ARGVTYPE_NULL
};
static const int ARGV_TYPE_NULL[SYNTAX_ARGV_TYPE_NUM] = {
	SYNTAX_ARGVTYPE_NULL, SYNTAX_ARGVTYPE_NULL,
	SYNTAX_ARGVTYPE_NULL, SYNTAX_ARGVTYPE_NULL
};
static const int ARGV_TYPE_ADDRESS_EXPRESSION[SYNTAX_ARGV_TYPE_NUM] = {
	SYNTAX_ARGVTYPE_VALUE,
	SYNTAX_ARGVTYPE_EXPRESSION, SYNTAX_ARGVTYPE_EXPRESSION, SYNTAX_ARGVTYPE_EXPRESSION
};
static const int ARGV_TYPE_ADDRESS_LENGTH[SYNTAX_ARGV_TYPE_NUM] = {
	SYNTAX_ARGVTYPE_VALUE, SYNTAX_ARGVTYPE_VALUE,
	SYNTAX_ARGVTYPE_NULL, SYNTAX_ARGVTYPE_NULL
};
static const int ARGV_TYPE_STEP_START[SYNTAX_ARGV_TYPE_NUM] = {
	SYNTAX_ARGVTYPE_VARIABLE, SYNTAX_ARGVTYPE_VALUE,
	SYNTAX_ARGVTYPE_VALUE, SYNTAX_ARGVTYPE_VALUE
};
static const struct script_syntax SCRIPT_SYNTAX[] = {
	{
		name: "MAPPER",
		script_opcode: SCRIPT_OPCODE_MAPPER,
		permittion: PERMITTION_ROM_DUMP,
		argc: 1, compare: SYNTAX_COMPARE_EQ,
		argv_type: ARGV_TYPE_VALUE_ONLY
	},{
		name: "MIRROR",
		script_opcode: SCRIPT_OPCODE_MIRROR,
		permittion: PERMITTION_ROM_DUMP,
		argc: 1, compare: SYNTAX_COMPARE_EQ,
		argv_type: ARGV_TYPE_HV
	},{
		name: OPSTR_CPU_ROMSIZE,
		script_opcode: SCRIPT_OPCODE_CPU_ROMSIZE,
		permittion: PERMITTION_ROM_DUMP | PERMITTION_ROM_PROGRAM,
		argc: 1, compare: SYNTAX_COMPARE_EQ,
		argv_type: ARGV_TYPE_VALUE_ONLY
	},{
		name: OPSTR_CPU_RAMSIZE,
		script_opcode: SCRIPT_OPCODE_CPU_RAMSIZE,
		permittion: PERMITTION_RAM_READ | PERMITTION_RAM_WRITE,
		argc: 1, compare: SYNTAX_COMPARE_EQ,
		argv_type: ARGV_TYPE_VALUE_ONLY
	},{
		name: OPSTR_PPU_ROMSIZE,
		script_opcode: SCRIPT_OPCODE_PPU_ROMSIZE,
		permittion: PERMITTION_ROM_DUMP | PERMITTION_ROM_PROGRAM,
		argc: 1, compare: SYNTAX_COMPARE_EQ,
		argv_type: ARGV_TYPE_VALUE_ONLY
	},{
		name: "DUMP_START",
		script_opcode: SCRIPT_OPCODE_DUMP_START,
		permittion: PERMITTION_ALL,
		argc: 0, compare: SYNTAX_COMPARE_EQ,
		argv_type: ARGV_TYPE_NULL
	},{
		name: "CPU_READ",
		script_opcode: SCRIPT_OPCODE_CPU_READ,
		permittion: PERMITTION_ROM_DUMP,
		argc: 2, compare: SYNTAX_COMPARE_EQ,
		argv_type: ARGV_TYPE_ADDRESS_LENGTH
	},{
		name: "CPU_WRITE",
		script_opcode: SCRIPT_OPCODE_CPU_WRITE,
		permittion: PERMITTION_ALL,
		argc: 2, compare: SYNTAX_COMPARE_GT,
		argv_type: ARGV_TYPE_ADDRESS_EXPRESSION
	},{
		name: OPSTR_CPU_RAMRW,
		script_opcode: SCRIPT_OPCODE_CPU_RAMRW,
		permittion: PERMITTION_RAM_READ | PERMITTION_RAM_WRITE,
		argc: 2, compare: SYNTAX_COMPARE_EQ,
		argv_type: ARGV_TYPE_ADDRESS_LENGTH
	},{
		name: "CPU_PROGRAM",
		script_opcode: SCRIPT_OPCODE_CPU_PROGRAM,
		permittion: PERMITTION_ROM_PROGRAM,
		argc: 2, compare: SYNTAX_COMPARE_EQ,
		argv_type: ARGV_TYPE_ADDRESS_LENGTH
	},{
		name: "PPU_RAMTEST",
		script_opcode: SCRIPT_OPCODE_PPU_RAMTEST,
		permittion: PERMITTION_ROM_DUMP,
		argc: 0, compare: SYNTAX_COMPARE_EQ,
		argv_type: ARGV_TYPE_NULL
	},{
		name: "PPU_READ",
		script_opcode: SCRIPT_OPCODE_PPU_READ,
		permittion: PERMITTION_ROM_DUMP,
		argc: 2, compare: SYNTAX_COMPARE_EQ,
		argv_type: ARGV_TYPE_ADDRESS_LENGTH
	},
#if OP_PPU_WRITE_ENABLE==1
	{
		name: "PPU_WRITE",
		script_opcode: SCRIPT_OPCODE_PPU_WRITE,
		permittion: PERMITTION_ROM_DUMP | PERMITTION_ROM_PROGRAM,
		argc: 2, compare: SYNTAX_COMPARE_EQ,
		argv_type: ARGV_TYPE_ADDRESS_LENGTH
	},
#endif
	{
		name: "PPU_PROGRAM",
		script_opcode: SCRIPT_OPCODE_PPU_PROGRAM,
		permittion: PERMITTION_ROM_PROGRAM,
		argc: 2, compare: SYNTAX_COMPARE_EQ,
		argv_type: ARGV_TYPE_ADDRESS_LENGTH,
	},{
		name: "STEP_START",
		script_opcode: SCRIPT_OPCODE_STEP_START,
		permittion: PERMITTION_ALL,
		argc: 4, compare: SYNTAX_COMPARE_EQ,
		argv_type: ARGV_TYPE_STEP_START
	},{
		name: "STEP_END",
		script_opcode: SCRIPT_OPCODE_STEP_END,
		permittion: PERMITTION_ALL,
		argc: 0, compare: SYNTAX_COMPARE_EQ,
		argv_type: ARGV_TYPE_NULL
	},{
		name: "WAIT",
		script_opcode: SCRIPT_OPCODE_WAIT,
		permittion: PERMITTION_ROM_PROGRAM,
		argc: 1, compare: SYNTAX_COMPARE_EQ,
		argv_type: ARGV_TYPE_VALUE_ONLY
	},{
		name: "DUMP_END",
		script_opcode: SCRIPT_OPCODE_DUMP_END,
		permittion: PERMITTION_ALL,
		argc: 0, compare: SYNTAX_COMPARE_EQ,
		argv_type: ARGV_TYPE_NULL
	}
};

