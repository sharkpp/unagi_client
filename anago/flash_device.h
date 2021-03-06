#ifndef _FLASH_DEVICE_H_
#define _FLASH_DEVICE_H_
struct flash_device{
	const wgChar *name;
	long capacity, pagesize;
	long erase_wait; //unit is msec
	bool erase_require, retry;
	uint8_t id_manufacurer, id_device;
	long command_mask;
};

bool flash_device_get(const wgChar *name, struct flash_device *t);
//0x80 以降は本当のデバイス重複しないと思う. 誰か JEDEC のとこをしらべて.
enum{
	FLASH_ID_DEVICE_SRAM = 0xf0, 
	FLASH_ID_DEVICE_DUMMY
};
//for GUI device listup
struct flash_listup{
	void *obj_cpu, *obj_ppu;
	void (*append)(void *obj, const wgChar *str);
};
void flash_device_listup(struct flash_listup *t);
#endif
