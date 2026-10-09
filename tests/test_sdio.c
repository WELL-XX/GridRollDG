// SPDX-License-Identifier: MIT
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include "paw3222_sdio.h"

struct mock {
    bool cs, clock, output;
    int calls, fail_at, reads;
    unsigned int transmitted, tx_bits, turnaround, last_delay;
};
static int failure(struct mock *m) { return ++m->calls == m->fail_at ? -EIO : 0; }
static int select_gpio(void *ctx, bool b) { struct mock *m=ctx; m->cs=b; return failure(m); }
static int clock_gpio(void *ctx, bool b) { struct mock *m=ctx; m->clock=b; return failure(m); }
static int direction_gpio(void *ctx, bool out, bool high) {
    (void)high; struct mock *m=ctx;
    if (!out && m->cs) m->turnaround++;
    m->output=out; return failure(m);
}
static int out_gpio(void *ctx, bool b) {
    struct mock *m=ctx; assert(m->cs && m->output && !m->clock);
    m->transmitted=(m->transmitted<<1)|b; m->tx_bits++; return failure(m);
}
static int in_gpio(void *ctx) {
    struct mock *m=ctx; assert(m->cs && !m->output && m->clock && m->turnaround);
    if (failure(m)) return -EIO;
    return (0xa5 >> (7-m->reads++)) & 1;
}
static void wait_gpio(void *ctx, unsigned int us) { ((struct mock *)ctx)->last_delay=us; }
static struct paw_sdio_io make_bus(struct mock *m) {
    return (struct paw_sdio_io){m,select_gpio,clock_gpio,direction_gpio,out_gpio,
        in_gpio,wait_gpio,5,100,50};
}
int main(void) {
    struct mock m={0}; struct paw_sdio_io bus=make_bus(&m); uint8_t value=0;
    assert(paw_sdio_read(&bus,3,&value)==0 && value==0xa5);
    assert(m.tx_bits==8 && m.transmitted==3 && m.reads==8);
    assert(!m.cs && !m.output && m.clock && m.last_delay==50);
    int read_calls=m.calls;
    m=(struct mock){0};
    assert(paw_sdio_write(&bus,0x0d,26)==0);
    assert(m.tx_bits==16 && m.transmitted==0x8d1a && !m.cs && !m.output && m.clock);
    int write_calls=m.calls;
    for (int write=0;write<2;write++) {
        for (int fail=1;fail<=(write?write_calls:read_calls);fail++) {
            m=(struct mock){.fail_at=fail}; value=0;
            int rc=write?paw_sdio_write(&bus,0x0d,26):paw_sdio_read(&bus,3,&value);
            assert(rc==-EIO);
            assert(!m.cs && !m.output && m.clock);
        }
    }
    assert(paw_sdio_read(&bus,0x80,&value)==-EINVAL);
    assert(paw_sdio_read(&bus,3,NULL)==-EINVAL);
    puts("SDIO read/write and every GPIO failure path passed");
}
