#include <assert.h>
#include <string.h>
#include <squirrel.h>
#include <sqstdio.h>
#include <sqstdaux.h>
#include <kazzo_task.h>
#include "type.h"
#include "widget.h"
#include "header.h"
#include "memory_manage.h"
#include "reader_master.h"
#include "squirrel_wrap.h"
#include "flash_device.h"
#include "progress.h"
#include "script_common.h"
#include "script_program.h"

static SQInteger vram_mirrorfind(HSQUIRRELVM v)
{
	struct flash_config *d;
	SQRESULT r =  qr_userpointer_get(v, (SQUserPointer) &d);
	if(SQ_FAILED(r)){
		return r;
	}
	return 0;
}
static SQInteger command_set(HSQUIRRELVM v, struct flash_memory_driver *t)
{
	long command, address ,mask;
	SQRESULT r = qr_argument_get(v, 3, &command, &address, &mask);
	if(SQ_FAILED(r)){
		return r;
	}
	long d = command & (mask - 1);
	d |= address;
	switch(command){
	case 0x0000:
		t->c000x = d;
		break;
	case 0x02aa: case 0x2aaa:
		t->c2aaa = d;
		break;
	case 0x0555: case 0x5555:
		t->c5555 = d;
		break;
	default:
		return sq_throwerror(v, "unknown command address");
	}
	t->command_change = true;
	return 0;
}
static SQInteger cpu_command(HSQUIRRELVM v)
{
	struct program_config *d;
	SQRESULT r =  qr_userpointer_get(v, (SQUserPointer) &d);
	if(SQ_FAILED(r)){
		return r;
	}
	return command_set(v, &d->cpu);
}
static SQInteger ppu_command(HSQUIRRELVM v)
{
	struct program_config *d;
	SQRESULT r =  qr_userpointer_get(v, (SQUserPointer) &d);
	if(SQ_FAILED(r)){
		return r;
	}
	return command_set(v, &d->ppu);
}
static SQInteger write_memory(HSQUIRRELVM v, const struct reader_handle *h, struct flash_memory_driver *t)
{
	long address, data;
	SQRESULT r = qr_argument_get(v, 2, &address, &data);
	if(SQ_FAILED(r)){
		return r;
	}
	uint8_t d8 = (uint8_t) data;
	t->access->memory_write(h, address, 1, &d8);
	return 0;
}
static SQInteger cpu_write(HSQUIRRELVM v)
{
	struct program_config *d;
	SQRESULT r =  qr_userpointer_get(v, (SQUserPointer) &d);
	if(SQ_FAILED(r)){
		return r;
	}
	return write_memory(v, d->handle, &d->cpu);
}
static SQInteger erase_set(HSQUIRRELVM v, const struct reader_handle *h, struct flash_memory_driver *t, struct textcontrol *log)
{
	t->access->flash_config(h, t->c000x, t->c2aaa, t->c5555, t->flash.pagesize, t->flash.retry);
	t->command_change = false;
	if(t->flash.erase_require == true){
		t->access->flash_erase(h, t->c2aaa, false);
		t->gauge.label_set(t->gauge.label, "erasing ");
	}
	return 0;
}
static SQInteger cpu_erase(HSQUIRRELVM v)
{
	struct program_config *d;
	SQRESULT r =  qr_userpointer_get(v, (SQUserPointer) &d);
	if(SQ_FAILED(r)){
		return r;
	}
	return erase_set(v, d->handle, &d->cpu, &d->log);
}
static SQInteger ppu_erase(HSQUIRRELVM v)
{
	struct program_config *d;
	SQRESULT r =  qr_userpointer_get(v, (SQUserPointer) &d);
	if(SQ_FAILED(r)){
		return r;
	}
	return erase_set(v, d->handle, &d->ppu, &d->log);
}
static SQInteger program_regist(HSQUIRRELVM v, const struct reader_handle *h, struct flash_memory_driver *t)
{
	SQRESULT r = qr_argument_get(v, 2, &t->programming.address, &t->programming.length);
	if(SQ_FAILED(r)){
		return r;
	}
	t->compare = t->programming;
	t->compare.offset = t->memory.offset & (t->memory.size - 1);
	if(t->command_change == true){
		t->access->flash_config(
			h, t->c000x, t->c2aaa, t->c5555, 
			t->flash.pagesize, t->flash.retry
		);
		t->command_change = false;
	}
	
/*	printf("programming %s ROM area 0x%06x...\n", name, t->memory->offset);
	fflush(stdout);*/
	return sq_suspendvm(v);
}
static void program_execute(const struct reader_handle *h, struct flash_memory_driver *t)
{
	const long w = t->access->flash_program(
		h, &t->gauge, 
		t->programming.address, t->programming.length, 
		t->memory.data + t->memory.offset, false, 
		t->flash.erase_require
	);
	t->programming.address += w;
	t->programming.length -= w;
	t->memory.offset += w;
	t->memory.offset &= t->memory.size - 1;
	t->programming.offset += w;
}

static bool program_compare(const struct reader_handle *h, struct flash_memory_driver *t)
{
	uint8_t *comparea = Malloc(t->compare.length);
	bool ret = false;
	if(t->flash.erase_require == true){
		memset(comparea, 0xff, t->compare.length);
		int doread = memcmp(comparea, t->memory.data + t->compare.offset, t->compare.length);
		if(0){
			memset(comparea, 0, t->compare.length);
			doread &= memcmp(comparea, t->memory.data + t->compare.offset, t->compare.length);
		}
		if(doread == 0){
			Free(comparea);
			return true;
		}
	}
	
	t->access->memory_read(h, &GAUGE_DUMMY, t->compare.address, t->compare.length, comparea);
	if(memcmp(comparea, t->memory.data + t->compare.offset, t->compare.length) == 0){
		ret = true;
	}
	Free(comparea);
	return ret;
}
static SQInteger cpu_program_memory(HSQUIRRELVM v)
{
	struct program_config *d;
	SQRESULT r =  qr_userpointer_get(v, (SQUserPointer) &d);
	if(SQ_FAILED(r)){
		return r;
	}
	return program_regist(v, d->handle, &d->cpu);
}
static SQInteger ppu_program_memory(HSQUIRRELVM v)
{
	struct program_config *d;
	SQRESULT r =  qr_userpointer_get(v, (SQUserPointer) &d);
	if(SQ_FAILED(r)){
		return r;
	}
	return program_regist(v, d->handle, &d->ppu);
}

static long erase_timer_get(const struct reader_handle *h, struct flash_memory_driver *t)
{
	if(
		(t->memory.transtype != TRANSTYPE_EMPTY) && 
		(t->flash.erase_require == true)
	){
		return t->flash.erase_wait;
	}else{
		return 0;
	}
}
static SQInteger erase_wait(HSQUIRRELVM v)
{
	struct program_config *d;
	SQRESULT r =  qr_userpointer_get(v, (SQUserPointer) &d);
	if(SQ_FAILED(r)){
		return r;
	}
	if(0){
		long timer_wait = erase_timer_get(d->handle, &d->cpu);
		long timer_ppu = erase_timer_get(d->handle, &d->ppu);
		if(timer_wait < timer_ppu){
			timer_wait = timer_ppu;
		}
		wait(timer_wait);
	}else{
		uint8_t s[2];
		do{
			wait(2);
			d->control->flash_status(d->handle, s);
		//����ΰտޤ���ǤϤ����ξ�Ｐ�� && �ǤϤʤ� || ��������� erase ������ä��ǥХ�����ư������ΤǻĤ��Ƥ���
		}while((s[0] != KAZZO_TASK_FLASH_IDLE) && (s[1] != KAZZO_TASK_FLASH_IDLE));
	}
	return 0;
}

static void gauge_init(struct flash_memory_driver *t)
{
	t->gauge.range_set(t->gauge.bar, t->programming.count);
	t->gauge.value_set(t->gauge.bar, t->gauge.label, t->programming.offset);
	if(t->programming.count == 0){
		t->gauge.label_set(t->gauge.label, "skip");
	}
}

static bool program_memoryarea(HSQUIRRELVM co, const struct reader_handle *h, struct flash_memory_driver *t, bool compare, SQInteger *state, struct textcontrol *log)
{
	if(t->programming.length == 0){
		if(t->programming.offset != 0 && compare == true){
			if(program_compare(h, t) == false){
				log->append(log->object, "%s memory compare error\n", t->memory.name);
				return false;
			}
		}

		sq_wakeupvm(co, SQFalse, SQFalse, SQTrue/*, SQTrue*/);
		*state = sq_getvmstate(co);
	}else{
		program_execute(h, t);
	}
	return true;
}

static SQInteger program_main(HSQUIRRELVM v)
{
	if(sq_gettop(v) != (1 + 3)){ //roottable, userpointer, co_cpu, co_ppu
		return sq_throwerror(v, "argument number error");
	}
	struct program_config *d;
	SQRESULT r =  qr_userpointer_get(v, (SQUserPointer) &d);
	if(SQ_FAILED(r)){
		return r;
	}
	HSQUIRRELVM co_cpu, co_ppu;
	if(SQ_FAILED(sq_getthread(v, 3, &co_cpu))){
		return sq_throwerror(v, "thread error");
	}
	if(SQ_FAILED(sq_getthread(v, 4, &co_ppu))){
		return sq_throwerror(v, "thread error");
	}
	SQInteger state_cpu = sq_getvmstate(co_cpu);
	SQInteger state_ppu = sq_getvmstate(co_ppu);
	const long sleepms = d->compare == true ? 6 : 2; //W29C040 �� compare �򤹤�ȡ�error ���Ф�ΤǽФʤ��ͤ�Ĵ�� (��äĤ��б�)
	
	while((state_cpu != SQ_VMSTATE_IDLE) || (state_ppu != SQ_VMSTATE_IDLE)){
		uint8_t s[2];
//		bool console_update = false;
		wait(sleepms);
		d->control->flash_status(d->handle, s);
		if(state_cpu != SQ_VMSTATE_IDLE && s[0] == KAZZO_TASK_FLASH_IDLE){
			if(program_memoryarea(co_cpu, d->handle, &d->cpu, d->compare, &state_cpu, &d->log) == false){
				return 0;
			}
		}
		if(state_ppu != SQ_VMSTATE_IDLE && s[1] == KAZZO_TASK_FLASH_IDLE){
			if(program_memoryarea(co_ppu, d->handle, &d->ppu, d->compare, &state_ppu, &d->log) == false){
				return 0;
			}
		}
	}
	return 0;
}

static SQInteger program_count(HSQUIRRELVM v, struct flash_memory_driver *t, const struct range *range_address, const struct range *range_length, struct textcontrol *log)
{
	SQRESULT r = qr_argument_get(v, 2, &t->programming.address, &t->programming.length);
	if(SQ_FAILED(r)){
		return r;
	}
	r = range_check(v, "length", t->programming.length, range_length);
	if(SQ_FAILED(r)){
		return r;
	}
	if((t->programming.address < range_address->start) || ((t->programming.address + t->programming.length) > range_address->end)){
		log->append(log->object, "address range must be 0x%06x to 0x%06x", (int) range_address->start, (int) range_address->end - 1);
		return sq_throwerror(v, "script logical error");;
	}
	t->programming.count += t->programming.length;
	return 0;
}
static SQInteger cpu_program_count(HSQUIRRELVM v)
{
	static const struct range range_address = {0x8000, 0x10000};
	static const struct range range_length = {0x0100, 0x4000};
	struct program_config *d;
	SQRESULT r =  qr_userpointer_get(v, (SQUserPointer) &d);
	if(SQ_FAILED(r)){
		return r;
	}
	return program_count(v, &d->cpu, &range_address, &range_length, &d->log);
}

static SQInteger ppu_program_count(HSQUIRRELVM v)
{
	static const struct range range_address = {0x0000, 0x2000};
	static const struct range range_length = {0x0100, 0x2000};
	struct program_config *d;
	SQRESULT r =  qr_userpointer_get(v, (SQUserPointer) &d);
	if(SQ_FAILED(r)){
		return r;
	}
	return program_count(v, &d->ppu, &range_address, &range_length, &d->log);
}

static bool script_execute(HSQUIRRELVM v, const char *function, struct program_config *c)
{
	bool ret = true;
	if(SQ_FAILED(sqstd_dofile(v, _SC("flashcore.nut"), SQFalse, SQTrue))){
		c->log.append(c->log.object, "flash core script error\n");
		ret = false;
	}else if(SQ_FAILED(sqstd_dofile(v, _SC(c->script), SQFalse, SQTrue))){
		c->log.append(c->log.object, "%s open error\n", c->script);
		ret = false;
	}else{
		SQRESULT r = qr_call(
			v, function, (SQUserPointer) c, true, 
			1 + 3 * 2, c->mappernum, 
			c->cpu.memory.transtype, c->cpu.memory.size, c->cpu.flash.capacity,
			c->ppu.memory.transtype, c->ppu.memory.size, c->cpu.flash.capacity
		);
		if(SQ_FAILED(r)){
			ret = false;
		}
	}
	return ret;
}

static void zendan(struct program_config *c)
{
//script test run
	{
		static const char *functionname[] = {
			"cpu_erase", "ppu_erase",
			"erase_wait", "program_main"
		};
		HSQUIRRELVM v = qr_open(&c->log);
		int i;
		for(i = 0; i < sizeof(functionname)/sizeof(char *); i++){
			qr_function_register_global(v, functionname[i], script_nop);
		}
		qr_function_register_global(v, "cpu_write", cpu_write_check);
		qr_function_register_global(v, "cpu_command", cpu_command);
		qr_function_register_global(v, "cpu_program", cpu_program_count);
		
		qr_function_register_global(v, "ppu_program", ppu_program_count);
		qr_function_register_global(v, "ppu_command", ppu_command);
		qr_function_register_global(v, "vram_mirrorfind", vram_mirrorfind);
		
		if(script_execute(v, "testrun", c) == false){
			qr_close(v);
			return;
		}
		qr_close(v);
		assert(c->cpu.memory.size != 0);

		if(c->cpu.programming.count % c->cpu.memory.size  != 0){
			c->log.append(c->log.object, "logical error: cpu_programsize is not connected 0x%06x/0x%06x\n", (int) c->cpu.programming.count, (int) c->cpu.memory.size);
			return;
		}
		if(c->ppu.memory.size != 0){
			if(c->ppu.programming.count % c->ppu.memory.size != 0){
				c->log.append(c->log.object, "logical error: ppu_programsize is not connected 0x%06x/0x%06x\n", (int) c->ppu.programming.count, (int) c->ppu.memory.size);
				return;
			}
		}
	}
//script execute 
	c->cpu.command_change = true;
	gauge_init(&c->cpu);
	c->ppu.command_change = true;
	gauge_init(&c->ppu);
	{
		HSQUIRRELVM v = qr_open(&c->log); 
		qr_function_register_global(v, "cpu_write", cpu_write);
		qr_function_register_global(v, "cpu_erase", cpu_erase);
		qr_function_register_global(v, "cpu_program", cpu_program_memory);
		qr_function_register_global(v, "cpu_command", cpu_command);
		qr_function_register_global(v, "ppu_erase", ppu_erase);
		qr_function_register_global(v, "ppu_program", ppu_program_memory);
		qr_function_register_global(v, "ppu_command", ppu_command);
		qr_function_register_global(v, "program_main", program_main);
		qr_function_register_global(v, "erase_wait", erase_wait);
		qr_function_register_global(v, "vram_mirrorfind", script_nop);
		script_execute(v, "program", c);
		qr_close(v);
	}
}

static bool memory_image_init(const struct memory *from, struct flash_memory_driver *t, struct textcontrol *log)
{
	t->memory.data = from->data;
	t->memory.size = from->size;
	t->memory.attribute = MEMORY_ATTR_READ;
	t->command_change = true;
	t->programming.count = 0;
	t->programming.offset = 0;
	if(t->memory.size == 0){
		t->memory.transtype = TRANSTYPE_EMPTY;
	}
	if(t->flash.capacity < from->size){
		log->append(log->object, t->memory.name);
		
		log->append(log->object, " image size is larger than target device");
		return false;
	}
	return true;
}

void script_program_execute(struct program_config *c)
{
//rom image load
	struct romimage rom;
	if(nesfile_load(&c->log, c->target, &rom) == false){
		c->log.append(c->log.object, "ROM image open error");
		return;
	}
//variable init
	c->mappernum = rom.mappernum;
	c->cpu.memory.name = "Program Flash";
	if(memory_image_init(&rom.cpu_rom, &c->cpu, &c->log) == false){
		nesbuffer_free(&rom, 0);
		return;
	}
	c->ppu.memory.name = "Charcter Flash";
	if(memory_image_init(&rom.ppu_rom, &c->ppu, &c->log) == false){
		nesbuffer_free(&rom, 0);
		return;
	}
//reader initalize
	c->handle = c->control->open(c->except);
	if(c->handle == NULL){
		c->log.append(c->log.object, "reader open error\n");
		nesbuffer_free(&rom, 0);
		return;
	}
//program start, reader finalize
	if(connection_check(c->handle, &c->log, c->cpu.access, c->ppu.access) == false){
		nesbuffer_free(&rom, 0);
		return;
	}
	zendan(c);
	c->control->close(c->handle);
	c->handle = NULL;
	nesbuffer_free(&rom, 0);
}