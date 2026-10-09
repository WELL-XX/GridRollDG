// SPDX-License-Identifier: MIT
#include "paw3222_sdio.h"
#include <errno.h>

static int transmit(const struct paw_sdio_io *io, uint8_t value) {
    for (unsigned int bit = 0; bit < 8; bit++) {
        int rc = io->clock(io->ctx, false);
        if (rc < 0) return rc;
        rc = io->data_out(io->ctx, (value & 0x80) != 0);
        if (rc < 0) return rc;
        io->delay(io->ctx, io->half_period_us);
        rc = io->clock(io->ctx, true);
        if (rc < 0) return rc;
        io->delay(io->ctx, io->half_period_us);
        value <<= 1;
    }
    return 0;
}

static int transaction(const struct paw_sdio_io *io, uint8_t reg, uint8_t *value,
                       bool write) {
    if (!io || !value || !io->select || !io->clock || !io->direction ||
        !io->data_out || !io->data_in || !io->delay || reg > 0x7f) return -EINVAL;
    int rc = io->clock(io->ctx, true);
    if (rc < 0) goto cleanup;
    rc = io->direction(io->ctx, true, true);
    if (rc < 0) goto cleanup;
    rc = io->select(io->ctx, true);
    if (rc < 0) goto cleanup;
    io->delay(io->ctx, io->half_period_us);
    rc = transmit(io, write ? (reg | 0x80) : reg);
    if (rc < 0) goto cleanup;
    if (write) {
        rc = transmit(io, *value);
    } else {
        /* Release the shared wire BEFORE the sensor drives its reply. */
        rc = io->direction(io->ctx, false, false);
        if (rc < 0) goto cleanup;
        io->delay(io->ctx, io->turnaround_us);
        uint8_t received = 0;
        for (unsigned int bit = 0; bit < 8; bit++) {
            rc = io->clock(io->ctx, false);
            if (rc < 0) goto cleanup;
            io->delay(io->ctx, io->half_period_us);
            rc = io->clock(io->ctx, true);
            if (rc < 0) goto cleanup;
            int sample = io->data_in(io->ctx);
            if (sample < 0) { rc = sample; goto cleanup; }
            received = (received << 1) | (sample != 0);
            io->delay(io->ctx, io->half_period_us);
        }
        *value = received;
    }
cleanup:
    /* Attempt ALL idle operations, even when one GPIO operation fails. */
    {
        int idle_cs = io->select(io->ctx, false);
        int idle_clk = io->clock(io->ctx, true);
        int release = io->direction(io->ctx, false, false);
        io->delay(io->ctx, io->gap_us);
        if (rc >= 0 && idle_cs < 0) rc = idle_cs;
        if (rc >= 0 && idle_clk < 0) rc = idle_clk;
        if (rc >= 0 && release < 0) rc = release;
    }
    return rc;
}

int paw_sdio_read(const struct paw_sdio_io *io, uint8_t reg, uint8_t *value) {
    return transaction(io, reg, value, false);
}
int paw_sdio_write(const struct paw_sdio_io *io, uint8_t reg, uint8_t value) {
    return transaction(io, reg, &value, true);
}
