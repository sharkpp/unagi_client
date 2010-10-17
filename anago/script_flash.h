#ifndef _SCRIPT_FLASH_H_
#define _SCRIPT_FLASH_H_
struct config_flash{
	const char *script, *target;
	struct flash_device flash_cpu, flash_ppu;
	const struct reader_driver *reader;
	struct romimage rom;
	bool compare, testrun;
	struct gauge gauge_cpu, gauge_ppu;
	struct textcontrol log;
};
void script_flash_execute(struct config_flash *c);
#endif
