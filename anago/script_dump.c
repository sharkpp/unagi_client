#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <squirrel.h>
#include <sqstdio.h>
#include <sqstdaux.h>
#include "type.h"
#include "file.h"
#include "widget.h"
#include "romimage.h"
#include "memory_manage.h"
#include "reader_master.h"
#include "squirrel_wrap.h"
#include "script_common.h"
#include "script_dump.h"

static SQInteger cpu_write(HSQUIRRELVM v)
{
	struct dump_config *d;

	SQRESULT r =  qr_userpointer_get(v, (SQUserPointer) &d);
	if(SQ_FAILED(r)){
		return r;
	}
	cpu_write_execute(v, d->handle, d->cpu.access);
	return 0;
}

//ここの printf は debug 用に残しておく
static void buffer_show(struct memory *t, long length)
{
	int i;
	const uint8_t *buf = t->data + t->offset;
#ifdef _UNICODE
	wprintf(L"%s 0x%06x:", t->name, t->offset);
#else
	printf("%s 0x%06x:", t->name, t->offset);
#endif
	for(i = 0; i < 0x10; i++){
		wgChar dump[3+1];
#ifdef _UNICODE
		//wsprintf(dump, L"%02x", buf[i]);
#else
		sprintf(dump, "%02x", buf[i]);
#endif
		switch(i){
		case 7:
			dump[2] = wgT('-');
			break;
		case 0x0f:
			dump[2] = wgT('\0');
			break;
		default:
			dump[2] = wgT(' ');
			break;
		}
		dump[3] = wgT('\0');
#ifdef _UNICODE
		wprintf(L"%s", dump);
#else
		printf("%s", dump);
#endif
	}
	int sum = 0;
	while(length != 0){
		sum += (int) *buf;
		buf++;
		length--;
	}
#ifdef _UNICODE
	wprintf(L":0x%06x\n", sum);
#else
	printf(":0x%06x\n", sum);
#endif
	fflush(stdout);
}

static SQInteger read_memory(HSQUIRRELVM v, const struct reader_handle *h, struct dump_memory_driver *t, bool progress)
{
	long address, length;
	SQRESULT r = qr_argument_get(v, 2, &address, &length);
	if(SQ_FAILED(r)){
		return r;
	}
	assert(t->memory.attribute == MEMORY_ATTR_WRITE);
	t->access->memory_read(h, &t->gauge, address, length == 0 ? 1: length, t->memory.data + t->memory.offset);
	if((length != 0) && (progress == false)){
		buffer_show(&t->memory, length);
	}
	t->memory.offset += length;
	return 0;
}

static SQInteger cpu_read(HSQUIRRELVM v)
{
	struct dump_config *d;
	SQRESULT r =  qr_userpointer_get(v, (SQUserPointer) &d);
	if(SQ_FAILED(r)){
		return r;
	}
	r = read_memory(v, d->handle, &d->cpu, d->progress);
	return r;
}

static SQInteger ppu_read(HSQUIRRELVM v)
{
	struct dump_config *d;
	SQRESULT r =  qr_userpointer_get(v, (SQUserPointer) &d);
	if(SQ_FAILED(r)){
		return r;
	}
	r = read_memory(v, d->handle, &d->ppu, d->progress);
	return r;
}

static SQInteger ppu_ramfind(HSQUIRRELVM v)
{
	struct dump_config *d;
	enum{
		testsize = 8,
		testaddress = 1234
	};
	static const uint8_t test_val[testsize] = {0xaa, 0x55, 0, 0xff, 0x46, 0x49, 0x07, 0x21};
	static const uint8_t test_str[testsize] = "pputest";
	uint8_t test_result[testsize];
	SQRESULT r =  qr_userpointer_get(v, (SQUserPointer) &d);
	struct dump_memory_driver *p = &d->ppu;

	if(SQ_FAILED(r)){
		return r;
	}
	p->access->memory_write(d->handle, testaddress, testsize, test_val);
	p->access->memory_read(d->handle, &GAUGE_DUMMY, testaddress, testsize, test_result);
	if(memcmp(test_val, test_result, testsize) != 0){
		sq_pushbool(v, SQFalse);
		return 1;
	}
	p->access->memory_write(d->handle, testaddress, testsize, test_str);
	p->access->memory_read(d->handle, &GAUGE_DUMMY, testaddress, testsize, test_result);
	if(memcmp(test_str, test_result, testsize) != 0){
		sq_pushbool(v, SQFalse);
		return 1;
	}
	p->memory.offset = 0;
	p->memory.size = 0;
	sq_pushbool(v, SQTrue);
	return 1;
}

static SQInteger return_true(HSQUIRRELVM v)
{
	sq_pushbool(v, SQFalse);
	return 1;
}

static void memory_new_init(struct dump_memory_driver *d)
{
	d->memory.offset = 0;
	d->memory.data = Malloc(d->memory.size);
	d->gauge.range_set(d->gauge.bar, d->memory.size);
	d->gauge.value_set(d->gauge.bar, d->gauge.label, 0);
}

//test 時/1度目の call で使用
static SQInteger memory_new(HSQUIRRELVM v)
{
	struct dump_config *d;
	SQRESULT r =  qr_userpointer_get(v, (SQUserPointer) &d);
	if(SQ_FAILED(r)){
		return r;
	}
	r = qr_argument_get(v, 2, &d->cpu.memory.size, &d->ppu.memory.size);
	if(SQ_FAILED(r)){
		return r;
	}

	memory_new_init(&d->cpu);
	if(d->mode == MODE_ROM_DUMP){
		memory_new_init(&d->ppu);
	}
	return 0;
}

//dump 時/2度目の call で nesfile_save として使用
static SQInteger nesfile_save(HSQUIRRELVM v)
{
	struct dump_config *d;
	SQRESULT r =  qr_userpointer_get(v, (SQUserPointer) &d);
	if(SQ_FAILED(r)){
		return r;
	}
	struct romimage image;
	long mirrorfind;
	r = qr_argument_get(v, 2, &image.mappernum, &mirrorfind);
	if(SQ_FAILED(r)){
		return r;
	}
	image.cpu_rom = d->cpu.memory;
	image.cpu_ram.data = NULL;
	image.ppu_rom = d->ppu.memory;
	image.mirror = MIRROR_PROGRAMABLE;
	if(mirrorfind == 1){
		if(d->control->vram_connection(d->handle) == 0x05){
			image.mirror = MIRROR_VERTICAL;
		}else{
			image.mirror = MIRROR_HORIZONAL;
		}
	}
	image.backupram = 0;
	if(d->battery == true){
		image.backupram = 1;
	}
	d->crc = nesfile_create(&d->log, &image, d->target);
	nesbuffer_free(&image, 0); //0 is MODE_xxx_xxxx
	
	d->cpu.memory.data = NULL;
	d->ppu.memory.data = NULL;
	return 0;
}

//dump 時/1度目の call で nesfile_save として使用
static SQInteger length_check(HSQUIRRELVM v)
{
	struct dump_config *d;
	SQRESULT r =  qr_userpointer_get(v, (SQUserPointer) &d);
	if(SQ_FAILED(r)){
		return r;
	}
	bool cpu = true, ppu = true;
	r = 0;
	if(d->cpu.memory.size != d->cpu.read_count){
		cpu = false;
	}
	if(cpu == false){
		d->log.append(d->log.object, wgT("cpu_romsize is not connected 0x%06x/0x%06x\n"), (int) d->cpu.read_count, (int) d->cpu.memory.size);
	}
	if(d->ppu.memory.size != d->ppu.read_count){
		ppu = false;
	}
	if(ppu == false){
		d->log.append(d->log.object, wgT("ppu_romsize is not connected 0x%06x/0x%06x\n"), (int) d->ppu.read_count, (int) d->ppu.memory.size);
	}
	if(cpu == false || ppu == false){
		r = sq_throwerror(v, wgT("script logical error"));
	}
	return r;
}

static SQInteger read_count(HSQUIRRELVM v, const struct textcontrol *l, struct dump_memory_driver *t, const struct range *range_address, const struct range *range_length)
{
	long address, length;
	SQRESULT r = qr_argument_get(v, 2, &address, &length);
	if(SQ_FAILED(r)){
		return r;
	}
	r = range_check(v, wgT("length"), length, range_length);
	if(SQ_FAILED(r)){
		return r;
	}
	if((address < range_address->start) || ((address + length) > range_address->end)){
		l->append(l->object, wgT("address range must be 0x%06x to 0x%06x"), (int) range_address->start, (int) range_address->end);
		return sq_throwerror(v, wgT("script logical error"));;
	}
	t->read_count += length;
	return 0;
}
static SQInteger cpu_read_count(HSQUIRRELVM v)
{
	static const struct range range_address = {0x8000, 0x10000};
	//length == 0 は 対象アドレスを呼んで、バッファにいれない。mmc2, mmc4 で使用する。
	static const struct range range_length = {0x0000, 0x4000};
	struct dump_config *d;
	SQRESULT r =  qr_userpointer_get(v, (SQUserPointer) &d);
	if(SQ_FAILED(r)){
		return r;
	}
	return read_count(v, &d->log, &d->cpu, &range_address, &range_length);
}

static SQInteger ppu_read_count(HSQUIRRELVM v)
{
	static const struct range range_address = {0x0000, 0x2000};
	static const struct range range_length = {0x0001, 0x2000};
	struct dump_config *d;
	SQRESULT r =  qr_userpointer_get(v, (SQUserPointer) &d);
	if(SQ_FAILED(r)){
		return r;
	}
	return read_count(v, &d->log, &d->ppu, &range_address, &range_length);
}

static SQInteger memory_size_set(HSQUIRRELVM v)
{
	struct dump_config *d;
	SQRESULT r =  qr_userpointer_get(v, (SQUserPointer) &d);
	if(SQ_FAILED(r)){
		return r;
	}
	r = qr_argument_get(v, 2, &d->cpu.memory.size, &d->ppu.memory.size);
	return r;
}

static bool script_execute(HSQUIRRELVM v, struct dump_config *d)
{
	bool ret = true;
	if(SQ_FAILED(sqstd_dofile(v, wgT("dumpcore.nut"), SQFalse, SQTrue))){
		d->log.append(d->log.object, wgT("dump core script error\n"));
		ret = false;
	}else{
		SQRESULT r = qr_call(
			v, wgT("dump"), (SQUserPointer) d, d->script, 
			3, d->mappernum, d->cpu.increase, d->ppu.increase
		);
		if(SQ_FAILED(r)){
			ret = false;
			Free(d->cpu.memory.data);
			Free(d->ppu.memory.data);
			d->cpu.memory.data = NULL;
			d->ppu.memory.data = NULL;
		}
	}
	return ret;
}

static void dump_memory_driver_init(struct dump_memory_driver *dd, enum memory_attribute at)
{
	dd->memory.size = 0;
	dd->memory.offset = 0;
	dd->memory.attribute = at;
	dd->memory.transtype = TRANSTYPE_FULL;
	dd->memory.data = NULL;
	dd->read_count = 0;
}

bool script_dump_execute(struct dump_config *d)
{
	dump_memory_driver_init(&d->cpu, MEMORY_ATTR_WRITE);
	d->cpu.memory.name = wgT("Program");
	
	dump_memory_driver_init(&d->ppu, MEMORY_ATTR_WRITE);
	d->ppu.memory.name = wgT("Charcter");
	
	{
		HSQUIRRELVM v = qr_open(&d->log);
		qr_function_register_global(v, wgT("cpu_write"), cpu_write_check);
		qr_function_register_global(v, wgT("memory_new"), memory_size_set);
		qr_function_register_global(v, wgT("nesfile_save"), length_check);
		qr_function_register_global(v, wgT("cpu_read"), cpu_read_count);
		qr_function_register_global(v, wgT("ppu_read"), ppu_read_count);
		qr_function_register_global(v, wgT("ppu_ramfind"), return_true);
		if(script_execute(v, d) == false){
			qr_close(v);
			return false;
		}
		qr_close(v);
	}

	d->handle = d->control->open(d->except, &d->log);
	if(d->handle == NULL){
		d->log.append(d->log.object, wgT("reader open error\n"));
		return false;
	}
	d->control->init(d->handle);
	if(connection_check(d->handle, &d->log, d->cpu.access, d->ppu.access) == false){
		d->control->close(d->handle);
		return false;
	}
	{
		HSQUIRRELVM v = qr_open(&d->log); 
		qr_function_register_global(v, wgT("memory_new"), memory_new);
		qr_function_register_global(v, wgT("nesfile_save"), nesfile_save);
		qr_function_register_global(v, wgT("cpu_write"), cpu_write);
		qr_function_register_global(v, wgT("cpu_read"), cpu_read);
		qr_function_register_global(v, wgT("ppu_read"), ppu_read);
		qr_function_register_global(v, wgT("ppu_ramfind"), ppu_ramfind);
		script_execute(v, d);
		qr_close(v);
	}
	d->control->close(d->handle);
	d->handle = NULL;
	return true;
}

static bool workram_execute(HSQUIRRELVM v, struct dump_config *d)
{
	bool ret = true;
	if(SQ_FAILED(sqstd_dofile(v, wgT("dumpcore.nut"), SQFalse, SQTrue))){
		d->log.append(d->log.object, wgT("dump core script error\n"));
		ret = false;
	}else{
		SQRESULT r = qr_call(
			v, wgT("workram_rw"), (SQUserPointer) d, d->script, 
			1, d->cpu.increase
		);
		if(SQ_FAILED(r)){
			ret = false;
			Free(d->cpu.memory.data);
			d->cpu.memory.data = NULL;
//			Free(d->ppu.memory.data);
//			d->ppu.memory.data = NULL;
		}
	}
	return ret;
}

static SQInteger cpu_ramrw_check(HSQUIRRELVM v)
{
	static const struct range range_address = {0x6000, 0xdfff};
	static const struct range range_length = {1, 0x2000};
	struct dump_config *d;
	SQRESULT r =  qr_userpointer_get(v, (SQUserPointer) &d);
	if(SQ_FAILED(r)){
		return r;
	}
	return read_count(v, &d->log, &d->cpu, &range_address, &range_length);
}

static SQInteger ramimage_open(HSQUIRRELVM v)
{
	struct dump_config *d;
	SQRESULT r =  qr_userpointer_get(v, (SQUserPointer) &d);
	if(SQ_FAILED(r)){
		return r;
	}
	if(buf_load(d->cpu.memory.data, d->target, d->cpu.memory.size) == NG){
		return r = sq_throwerror(v, wgT("RAM image open error"));
	}
	return 0;
}

static SQInteger memory_finalize(HSQUIRRELVM v)
{
	struct dump_config *d;
	SQRESULT r =  qr_userpointer_get(v, (SQUserPointer) &d);
	if(SQ_FAILED(r)){
		return r;
	}
	if(d->mode == MODE_RAM_READ){
		buf_save(d->cpu.memory.data, d->target, d->cpu.memory.size);
	}
	Free(d->cpu.memory.data);
	d->cpu.memory.data = NULL;
//	Free(d->ppu.memory.data);
//	d->ppu.memory.data = NULL;
	
	return 0;
}

static SQInteger cpu_write_ramimage(HSQUIRRELVM v)
{
	struct dump_config *d;
	SQRESULT r =  qr_userpointer_get(v, (SQUserPointer) &d);
	if(SQ_FAILED(r)){
		return r;
	}

	long address, length;
	uint8_t *cmp;
	const uint8_t *writedata = d->cpu.memory.data;
	writedata += d->cpu.memory.offset;
	
	r = qr_argument_get(v, 2, &address, &length);
	if(SQ_FAILED(r)){
		return r;
	}
	cmp = Malloc(length);
	assert(d->cpu.memory.attribute == MEMORY_ATTR_READ);
	//&d->cpu.gauge, 
	d->cpu.access->memory_write(
		d->handle, address, length, writedata
	);
	d->cpu.access->memory_read(
		d->handle, &d->cpu.gauge, address, length, cmp
	);
	d->cpu.memory.offset += length;

	r = memcmp(cmp, writedata, length);
	Free(cmp);
	if(r != 0){
		r = sq_throwerror(v, wgT("memory write failed"));
	}
	return 0;
}

bool script_workram_execute(struct dump_config *d)
{
	dump_memory_driver_init(&d->cpu, d->mode == MODE_RAM_READ ? MEMORY_ATTR_WRITE : MEMORY_ATTR_READ);
	dump_memory_driver_init(&d->ppu, MEMORY_ATTR_NOTUSE);
	d->cpu.memory.name = wgT("Workram");
	d->ppu.memory.name = wgT("N/A");

	{
		HSQUIRRELVM v = qr_open(&d->log);
		qr_function_register_global(v, wgT("memory_new"), memory_new);
		qr_function_register_global(v, wgT("cpu_write"), cpu_write_check);
		qr_function_register_global(v, wgT("cpu_ramrw"), cpu_ramrw_check);
		qr_function_register_global(v, wgT("memory_finalize"), length_check);
		if(workram_execute(v, d) == false){
			qr_close(v);
			return false;
		}
		qr_close(v);
	}
	
	d->handle = d->control->open(d->except, &d->log);
	if(d->handle == NULL){
		d->log.append(d->log.object, wgT("reader open error\n"));
		return false;
	}
	d->control->init(d->handle);
	if(connection_check(d->handle, &d->log, d->cpu.access, d->ppu.access) == false){
		d->control->close(d->handle);
		return false;
	}
	{
		HSQUIRRELVM v = qr_open(&d->log); 
		qr_function_register_global(v, wgT("cpu_write"), cpu_write);
		switch(d->mode){
		case MODE_RAM_READ:
			qr_function_register_global(v, wgT("memory_new"), script_nop);
			qr_function_register_global(v, wgT("cpu_ramrw"), cpu_read);
			break;
		case MODE_RAM_WRITE:
			qr_function_register_global(v, wgT("memory_new"), ramimage_open);
			qr_function_register_global(v, wgT("cpu_ramrw"), cpu_write_ramimage);
			break;
		default:
			assert(0);
			break;
		}
		qr_function_register_global(v, wgT("memory_finalize"), memory_finalize);
		workram_execute(v, d);
		qr_close(v);
	}
	d->control->close(d->handle);
	d->handle = NULL;
	return true;
}
