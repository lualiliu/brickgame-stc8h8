#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include "sys.h"
#include "core.h"
#include "rom.h"

static void test_keys() {
	char x;
	while ((x | 32) != 'q')
		if (read(0, &x, 1) == 1)
			printf("0x%02x %u\n", x, x);
}

int main(){
	uint32_t hold_time = 50;
	uint32_t sleep_ticks = 1000, sleep_delay = 1000;
	uint32_t timer_inc = 32;
	timer_inc = timer_inc ? 0x10000 / timer_inc : 0x10000;
	if (timer_inc > 0x10000) timer_inc = 0x10000;
	
	sysctx_t ctx;

	cpu_state_t cpu;
	memset(&cpu, 0, sizeof(cpu));

	memset(&ctx, 0, sizeof(ctx));
	
	sys_init(&ctx);

	ctx.hold_time = hold_time;
	ctx.sleep_ticks = sleep_ticks;
	ctx.sleep_delay = sleep_delay;
	ctx.timer_inc = timer_inc;
	
	//test_keys();

	run_game(E23PlusMarkII96in1_bin, &ctx, &cpu);

	sys_close(&ctx);
	return 0;
}
