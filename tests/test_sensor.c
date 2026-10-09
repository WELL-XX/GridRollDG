/* SPDX-License-Identifier: MIT; exercises the production driver's logic. */
#include <assert.h>
#include <stdio.h>
#include "../drivers/input/paw3222.c"

struct chip { uint8_t regs[128]; unsigned int tx, bits, reads; bool failing; };
static int chip_select(void *ctx, bool selected) {
    struct chip *c=ctx;
    if (c->failing) return -EIO;
    if (selected) { c->tx=0; c->bits=0; c->reads=0; }
    else if (c->bits==16) c->regs[(c->tx>>8)&0x7f]=c->tx&0xff;
    return 0;
}
static int chip_clock(void *ctx, bool high) { ARG_UNUSED(ctx); ARG_UNUSED(high); return 0; }
static int chip_direction(void *ctx, bool output, bool high) {
    ARG_UNUSED(ctx); ARG_UNUSED(output); ARG_UNUSED(high); return 0;
}
static int chip_out(void *ctx, bool high) {
    struct chip *c=ctx; c->tx=(c->tx<<1)|high; c->bits++; return 0;
}
static int chip_in(void *ctx) {
    struct chip *c=ctx;
    return (c->regs[c->tx&0x7f]>>(7-c->reads++))&1;
}
static void chip_delay(void *ctx, unsigned int us) { ARG_UNUSED(ctx); ARG_UNUSED(us); }

int main(void) {
    struct chip chip={0}; struct paw_data data={0};
    struct device controller={0};
    const struct paw_config config={ .sclk={&controller,12,0}, .sdio={&controller,9,0},
        .cs={&controller,13,0}, .motion={&controller,15,0},
        .cpi=988, .half_us=5, .turnaround_us=100, .gap_us=50, .poll_ms=5 };
    const struct device dev={&config,&data}; data.dev=&dev;
    data.bus=(struct paw_sdio_io){&chip,chip_select,chip_clock,chip_direction,
                                chip_out,chip_in,chip_delay,5,100,50};
    assert(sensor_start(&dev)==-ENODEV);
    assert(input_count==0 && interrupts==0);
    chip.regs[REG_ID]=0x30;
    assert(sensor_start(&dev)==0);
    assert(chip.regs[REG_CPI_X]==26 && chip.regs[REG_CPI_Y]==26);
    assert(chip.regs[REG_PROTECT]==0 && (chip.regs[REG_OPERATION]&0x18)==0x18);
    chip.regs[REG_MOTION]=0x80;
    chip.regs[REG_X]=0x7f; chip.regs[REG_Y]=0x80;
    motion_work(&data.work.work);
    assert(input_count==2 && input_values[0]==127 && input_values[1]==-128);
    assert(input_codes[0]==INPUT_REL_X && input_codes[1]==INPUT_REL_Y);
    assert(!input_sync[0] && input_sync[1]);
    chip.regs[REG_X]=0xff; chip.regs[REG_Y]=1;
    motion_work(&data.work.work);
    assert(input_values[2]==-1 && input_values[3]==1);
    chip.regs[REG_MOTION]=0;
    motion_work(&data.work.work);
    assert(input_count==4);
    chip.failing=true;
    for (int i=0;i<3;i++) { motion_work(&data.work.work); assert(scheduled_ms==20); }
    motion_work(&data.work.work); assert(scheduled_ms==1000 && data.errors==4);
    chip.failing=false;
    motion_work(&data.work.work); assert(data.errors==0);
    /* The GPIO shim has no sensor, so the complete real init must fail quietly. */
    assert(paw_init(&dev)==-ENODEV);
    assert(interrupts==0 && input_count==4);
    puts("PAW signed motion, init, missing sensor and retry recovery passed");
}
