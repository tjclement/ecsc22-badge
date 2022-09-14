#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/structs/syscfg.h"
#include "hardware/interp.h"
#include <stdlib.h>
#include <string.h>

void keygen(uint8_t * key) {
	interp_config cfg = interp_default_config();
	interp_set_config(interp0, 1, &cfg);
	interp_config_set_blend(&cfg, true);
	interp_set_config(interp0, 0, &cfg);

	interp0->base[0] = *((uint32_t *)0x61);
	interp0->base[1] = *((uint32_t *)0x61 + 1);

	for(int i = 0; i < 64; i++) {
		interp0->accum[1] = 255 * i / 64;
		uint32_t *div = (uint32_t *)&(interp0->peek[1]);
		uint8_t *b = (uint8_t*)(div);
		key[i] = b[0] + b[1] + b[2] + b[3];
	}
}

void interpolarize(io_rw_32 accum0, io_rw_32 base0, io_rw_32 accum1, io_rw_32 base1) {
	interp_config cfg = interp_default_config();
	interp_set_config(interp0, 0, &cfg);
	interp_set_config(interp0, 1, &cfg);
	
	interp_set_config(interp1, 0, &cfg);
	interp_set_config(interp1, 1, &cfg);
	
	interp0->accum[0] = accum0;
	interp0->base[0] = base0;
	interp0->accum[1] = accum1;
	interp0->base[1] = base1;

	for(int i = 0; i < 63; i++) {
		uint8_t *refa = (uint8_t*)(interp0->peek[0]);
		uint8_t *refb = (uint8_t*)(interp0->pop[1]);
		
		interp1->accum[0] = *refa;
		interp1->accum[1] = *refb;
		
		*refa = interp1->pop[2];
	}
}

void hash(uint8_t *key, const char *input, uint8_t *output) {
	int len = strlen(input);
	memcpy(output, input, 64);
	memset(&output[len], 64-len, 64-len);
	
	interpolarize((io_rw_32)(output-1), 1, (io_rw_32)(key-1), 1);	
	interpolarize((io_rw_32)output, 1, (io_rw_32)(output-1), 1);
	interpolarize((io_rw_32)(output+63), -1, (io_rw_32)(output+64), -1);
}
