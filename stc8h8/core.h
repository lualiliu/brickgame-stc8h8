#include "sys.h"

#ifndef CORE_H
#define CORE_H

static void run_game(uint8_t *rom, sysctx_t *sys, cpu_state_t *s) {
	unsigned pc = s->pc;
	unsigned a = s->a, cf = s->cf;
	uint8_t pa = 0, pm = 0xf, ps = 0xf, pp = 0xf;
	uint32_t tickcount = 0, prev_tick = 0, tmr_frac = 0;
	uint64_t last_time;
	uint32_t keys = 0;
	
#define CPU_TRACE 0

	last_time = get_time_usec();

	for (;;) {
		unsigned x, op;

		op = rom[pc];
#define R1R0 s->r[1] << 4 | s->r[0]
#define R3R2 s->r[3] << 4 | s->r[2]

#if CPU_TRACE
#define TRACE(...) fprintf(stderr, "  " __VA_ARGS__)
		fprintf(stderr, "%03x: o=%02x,r=%x:%02x:%02x:%x,c%u",
				pc, op, a, R1R0, R3R2, s->r[4], cf);
#else
#define TRACE() (void)0
#endif

	switch (op) {

	case 0x00: /* RR A */
		cf = a & 1; a = (a << 4 | a) >> 1 & 15; TRACE("a=%x,c=%u", a, cf); break;
	case 0x01: /* RL A */
		cf = a >> 3; a = (a << 4 | a) >> 3 & 15; TRACE("a=%x,c=%u", a, cf); break;
	case 0x02: /* RRC A */
		a = cf << 4 | a; cf = a & 1; a >>= 1; TRACE("a=%x,c=%u", a, cf); break;
	case 0x03: /* RLC A */
		a = a << 1 | cf; cf = a >> 4; a &= 15; TRACE("a=%x,c=%u", a, cf); break;

	case 0x04: // MOV A, [R1R0]
	case 0x06: // MOV A, [R3R2]
		x = op & 2; x = s->r[x + 1] << 4 | s->r[x]; a = s->mem[x]; TRACE("a=%x", a); break;
	case 0x05: // MOV [R1R0], A
	case 0x07: // MOV [R3R2], A
		x = op & 2; x = s->r[x + 1] << 4 | s->r[x]; s->mem[x] = a; TRACE("m[%02x]=%x", x, a); break;

	case 0x08: /* ADC A, [R1R0] */
	case 0x09: /* ADD A, [R1R0] */
		cf &= ~op;
		a += s->mem[R1R0] + cf; cf = a >> 4; a &= 15;
		TRACE("a=%x", a);
		break;

	case 0x0a: /* SBC A, [R1R0] */
	case 0x0b: /* SUB A, [R1R0] */
		cf |= op & 1;
		a += 15 - s->mem[R1R0] + cf; cf = a >> 4; a &= 15;
		TRACE("a=%x", a);
		break;

	case 0x0c: // INC [R1R0]
	case 0x0d: // DEC [R1R0]
	case 0x0e: // INC [R3R2]
	case 0x0f: // DEC [R3R2]
		x = op & 2; x = s->r[x + 1] << 4 | s->r[x];
		s->mem[x] = (s->mem[x] + (op & 1 ? -1 : 1)) & 15;
		TRACE("m[%02x]=%x", x, s->mem[x]);
		break;

	case 0x10: case 0x12: // INC Rn
	case 0x14: case 0x16: case 0x18:
		x = op >> 1 & 7; s->r[x] = (s->r[x] + 1) & 15; TRACE("r%u=%x", x, s->r[x]); break;

	case 0x11: case 0x13: // DEC Rn
	case 0x15: case 0x17: case 0x19:
		x = op >> 1 & 7; s->r[x] = (s->r[x] - 1) & 15; TRACE("r%u=%x", x, s->r[x]); break;

	case 0x1a: /* AND A, [R1R0] */ a &= s->mem[R1R0]; TRACE("a=%x", a); break;
	case 0x1b: /* XOR A, [R1R0] */ a ^= s->mem[R1R0]; TRACE("a=%x", a); break;
	case 0x1c: /* OR A, [R1R0] */ a |= s->mem[R1R0]; TRACE("a=%x", a); break;
	case 0x1d: /* AND [R1R0], A */ s->mem[R1R0] &= a; TRACE("m[%02x]=%x", R1R0, s->mem[R1R0]); break;
	case 0x1e: /* XOR [R1R0], A */ s->mem[R1R0] ^= a; TRACE("m[%02x]=%x", R1R0, s->mem[R1R0]); break;
	case 0x1f: /* OR [R1R0], A */ s->mem[R1R0] |= a; TRACE("m[%02x]=%x", R1R0, s->mem[R1R0]); break;

	case 0x20: case 0x22: // MOV Rn, A
	case 0x24: case 0x26: case 0x28:
		s->r[op >> 1 & 7] = a; TRACE("r%u=%x", op >> 1 & 7, a); break;

	case 0x21: case 0x23: // MOV A, Rn
	case 0x25: case 0x27: case 0x29:
		a = s->r[op >> 1 & 7]; TRACE("a=%x", a); break;

	case 0x2a: /* CLC */ cf = 0; TRACE("c=%x", cf); break;
	case 0x2b: /* STC */ cf = 1; TRACE("c=%x", cf); break;
	case 0x2c: /* EI */ /* TODO */; TRACE("i=%x", 1); break;
	case 0x2d: /* DI */ /* TODO */; TRACE("i=%x", 0); break;
	case 0x2e: /* RET */
		pc = s->stack; TRACE("pc=%03x", pc); pc--; break;
	case 0x2f: /* RETI */
		pc = s->stack; cf = pc >> 12; TRACE("pc=%03x,c=%u", pc, cf); pc--; break;

	case 0x30: /* OUT PA, A */ pa = a; TRACE("pa=%x", pa); break;
	case 0x31: /* INC A */ a = (a + 1) & 15; TRACE("a=%x", a); break;
	case 0x32: /* IN A, PM */ a = pm; TRACE("a=%x", a); break;
	case 0x33: /* IN A, PS */ a = ps; TRACE("a=%x", a); break;
	case 0x34: /* IN A, PP */ a = pp; TRACE("a=%x", a); break;
	case 0x35: /* unknown */ break;
	case 0x36: /* DAA */
		if (a >= 10 || cf) a = (a + 6) & 15, cf = 1, TRACE("a=%x,c=%u", a, cf);
		break;
	case 0x37: /* HALT */
		TRACE("halt"); break;
	case 0x38: /* TIMER ON */
		s->timer_en = 1; TRACE("timer on"); break;
	case 0x39: /* TIMER OFF */
		s->timer_en = 0; TRACE("timer off"); break;
	case 0x3a: /* MOV A, TMRL */
		a = s->tmr & 15; TRACE("a=%x", a); break;
	case 0x3b: /* MOV A, TMRH */
		a = s->tmr >> 4; TRACE("a=%x", a); break;
	case 0x3c: /* MOV TMRL, A */
		s->tmr = (s->tmr & 0xf0) | a; TRACE("tmrl=%x", a); break;
	case 0x3d: /* MOV TMRH, A */
		s->tmr = a << 4 | (s->tmr & 15); TRACE("tmrh=%x", a); break;
	case 0x3e: /* NOP */
		TRACE("nop"); break;
	case 0x3f: /* DEC A */ a = (a - 1) & 15; TRACE("a=%x", a); break;

	case 0x40: // ADD A, imm4
		a += rom[++pc & 0xfff] & 15;
		cf = a >> 4; a &= 15; TRACE("a=%x", a); break;
	case 0x41: // SUB A, imm4
		a += 16 - (rom[++pc & 0xfff] & 15);
		cf = a >> 4; a &= 15; TRACE("a=%x", a); break;
	case 0x42: // AND A, imm4
		a &= rom[++pc & 0xfff]; TRACE("a=%x", a); break;
	case 0x43: // XOR A, imm4
		a ^= rom[++pc & 0xfff] & 15; TRACE("a=%x", a); break;
	case 0x44: // OR A, imm4
		a |= rom[++pc & 0xfff] & 15; TRACE("a=%x", a); break;
	case 0x45: // SOUND imm4
		x = rom[++pc & 0xfff] & 15; TRACE("sound %x", x);
		(void)x; /* TODO */ break;
	case 0x46: // MOV R4, imm4
		s->r[4] = rom[++pc & 0xfff] & 15; TRACE("r4=%x", s->r[4]); break;
	case 0x47: // TIMER imm8
		s->tmr = rom[++pc & 0xfff]; TRACE("tmr=%02x", s->tmr); break;
	case 0x48: /* SOUND ONE */ TRACE("sound one"); /* TODO */ break;
	case 0x49: /* SOUND LOOP */ TRACE("sound loop"); /* TODO */ break;
	case 0x4a: /* SOUND OFF */ TRACE("sound off"); /* TODO */ break;
	case 0x4b: /* SOUND A */ TRACE("sound a"); /* TODO */ break;
	case 0x4c: /* READ R4A */
		a = rom[(pc & 0xf00) | a << 4 | s->mem[R1R0]];
		TRACE("r4:a=%02x", a);
		s->r[4] = a >> 4; a &= 15; break;
	case 0x4d: /* READF R4A */
		a = rom[0xf00 | a << 4 | s->mem[R1R0]];
		TRACE("r4:a=%02x", a);
		s->r[4] = a >> 4; a &= 15; break;
	case 0x4e: /* READ MR0A */
		a = rom[(pc & 0xf00) | a << 4 | s->r[4]];
		TRACE("m[%02x]:a=%02x", R1R0, a);
		s->mem[R1R0] = a >> 4; a &= 15; break;
	case 0x4f: /* READF MR0A */
		a = rom[0xf00 | a << 4 | s->r[4]];
		TRACE("m[%02x]:a=%02x", R1R0, a);
		s->mem[R1R0] = a >> 4; a &= 15; break;

#define CASE8(x) \
	case x:     case x + 1: case x + 2: case x + 3: \
	case x + 4: case x + 5: case x + 6: case x + 7:

	CASE8(0x50) CASE8(0x58) // MOV R1R0, imm8
		s->r[0] = op & 0xf; s->r[1] = rom[++pc & 0xfff] & 15; TRACE("r1r0=%02x", R1R0); break;
	CASE8(0x60) CASE8(0x68) // MOV R3R2, imm8
		s->r[2] = op & 0xf; s->r[3] = rom[++pc & 0xfff] & 15; TRACE("r3r2=%02x", R3R2); break;

	CASE8(0x70) CASE8(0x78) /* MOV A, imm4 */ a = op & 15; TRACE("a=%x", a); break;

#define JMP11 \
	x = (pc & 0x800) | (op & 7) << 8 | rom[(pc + 1) & 0xfff]; pc++;
#define TRACE_JUMP TRACE("pc=%03x", pc + 1); else TRACE("no jump")
#define X(cond) JMP11 if (cond) pc = x - 1, TRACE_JUMP; break;
	CASE8(0x80) CASE8(0x88) // JAn imm11
	CASE8(0x90) CASE8(0x98) X(a >> (op >> 3 & 3) & 1)
	CASE8(0xa0) /* JNZ R0, imm11 */ X(s->r[0])
	CASE8(0xa8) /* JNZ R1, imm11 */ X(s->r[1])
	CASE8(0xb0) /* JZ A, imm11 */ X(!a)
	CASE8(0xb8) /* JNZ A, imm11 */ X(a)
	CASE8(0xc0) /* JC imm11 */ X(cf)
	CASE8(0xc8) /* JNC imm11 */ X(!cf)
	CASE8(0xd0) /* JTMR imm11 */ JMP11 if (s->tf) pc = x - 1, TRACE_JUMP; s->tf = 0; break;
	CASE8(0xd8) /* JNZ R4, imm11 */ X(s->r[4])
#undef X
#undef JMP11

	CASE8(0xe0) CASE8(0xe8) // JMP imm12
		pc = (op & 15) << 8 | rom[(pc + 1) & 0xfff];
		TRACE("pc=%03x", pc);
		pc--; break;
	CASE8(0xf0) CASE8(0xf8) // CALL imm12
		s->stack = (pc + 2) & 0xfff;
		pc = (op & 15) << 8 | rom[(pc + 1) & 0xfff];
		TRACE("pc=%03x,ret=%03x", pc, s->stack);
		pc--; break;

	default:
		TRACE("unknown opcode\n");
	} // end switch

#if CPU_TRACE
		fprintf(stderr, "\n");
#endif
		pc = (pc + 1) & 0xfff;
		tickcount++;

#if CPU_TRACE
		if (tickcount > 2150) {
			// display_redraw(sys, s->mem);
			break;
		}
#endif
/*
		// 1ms
		if (tickcount - prev_tick >= sys->sleep_ticks) {
			uint64_t new_time, delay;
			uint32_t keys, sleep_delay;
			prev_tick = tickcount;
			sys_redraw(sys, s->mem);
			new_time = get_time_usec();
			delay = new_time - last_time;
			sleep_delay = sys->sleep_delay;
			if (delay > sleep_delay) {
				last_time = new_time;
			} else {
				last_time += sleep_delay;
				//usleep(sleep_delay - delay);
				//delay_ms(sleep_delay - delay);
			}
			keys = ~sys_events(sys);
			if (!(keys & 0x10000)) break;
			pp = keys & 15;
			ps = keys >> 4 & 15;
		}
*/
		
		sys_redraw(sys, s->mem);
		keys = ~sys_events(sys);
		if (!(keys & 0x10000)) break;
		pp = keys & 15;
		ps = keys >> 4 & 15;

		if (s->timer_en) {
			tmr_frac += sys->timer_inc;
			if (tmr_frac >= 0x10000) {
				tmr_frac -= 0x10000;
				if (!++s->tmr) s->tf = 1;
			}
		}

		(void)pa;

	} // end for

	s->pc = pc; s->a = a; s->cf = cf;
}

#endif
