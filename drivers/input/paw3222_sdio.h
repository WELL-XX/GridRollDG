// SPDX-License-Identifier: MIT
#pragma once
#include <stdbool.h>
#include <stdint.h>

/* GPIO callbacks return zero on success, negative errno on failure.
 * data_in returns a physical bit (0/1) or a negative errno. */
struct paw_sdio_io {
    void *ctx;
    int (*select)(void *ctx, bool selected);
    int (*clock)(void *ctx, bool high);
    int (*direction)(void *ctx, bool output, bool initial_high);
    int (*data_out)(void *ctx, bool high);
    int (*data_in)(void *ctx);
    void (*delay)(void *ctx, unsigned int us);
    unsigned int half_period_us;
    unsigned int turnaround_us;
    unsigned int gap_us;
};

int paw_sdio_read(const struct paw_sdio_io *io, uint8_t reg, uint8_t *value);
int paw_sdio_write(const struct paw_sdio_io *io, uint8_t reg, uint8_t value);
