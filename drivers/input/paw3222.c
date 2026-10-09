// SPDX-License-Identifier: MIT
#define DT_DRV_COMPAT pandakb_paw3222_sdio
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "paw3222_sdio.h"

LOG_MODULE_REGISTER(gridrolldg_paw3222, CONFIG_GRIDROLLDG_PAW3222_LOG_LEVEL);

enum { REG_ID = 0x00, REG_MOTION = 0x02, REG_X = 0x03, REG_Y = 0x04,
       REG_OPERATION = 0x05, REG_CONFIGURATION = 0x06, REG_PROTECT = 0x09,
       REG_CPI_X = 0x0d, REG_CPI_Y = 0x0e, REG_DELTA_HI = 0x12 };

struct paw_config {
    struct gpio_dt_spec sclk, sdio, cs, motion;
    unsigned int half_us, turnaround_us, gap_us, poll_ms;
    uint16_t cpi;
};
struct paw_data {
    const struct device *dev;
    struct paw_sdio_io bus;
    struct gpio_callback callback;
    struct k_work_delayable work;
    struct k_mutex lock;
    unsigned int errors;
};

static int select_pin(void *ctx, bool selected) {
    const struct paw_config *c = ((const struct device *)ctx)->config;
    return gpio_pin_set_dt(&c->cs, selected);
}
static int clock_pin(void *ctx, bool high) {
    const struct paw_config *c = ((const struct device *)ctx)->config;
    return gpio_pin_set_dt(&c->sclk, high);
}
static int direction_pin(void *ctx, bool output, bool high) {
    const struct paw_config *c = ((const struct device *)ctx)->config;
    return gpio_pin_configure_dt(&c->sdio,
                                output ? (high ? GPIO_OUTPUT_HIGH : GPIO_OUTPUT_LOW)
                                       : GPIO_INPUT);
}
static int out_pin(void *ctx, bool high) {
    const struct paw_config *c = ((const struct device *)ctx)->config;
    return gpio_pin_set_dt(&c->sdio, high);
}
static int in_pin(void *ctx) {
    const struct paw_config *c = ((const struct device *)ctx)->config;
    return gpio_pin_get_dt(&c->sdio);
}
static void delay(void *ctx, unsigned int us) { ARG_UNUSED(ctx); k_busy_wait(us); }

static int read_reg(const struct device *dev, uint8_t reg, uint8_t *value) {
    struct paw_data *d = dev->data;
    k_mutex_lock(&d->lock, K_FOREVER);
    int rc = paw_sdio_read(&d->bus, reg, value);
    k_mutex_unlock(&d->lock);
    return rc;
}
static int write_reg(const struct device *dev, uint8_t reg, uint8_t value) {
    struct paw_data *d = dev->data;
    k_mutex_lock(&d->lock, K_FOREVER);
    int rc = paw_sdio_write(&d->bus, reg, value);
    k_mutex_unlock(&d->lock);
    return rc;
}

static void motion_work(struct k_work *work) {
    struct paw_data *d = CONTAINER_OF(k_work_delayable_from_work(work), struct paw_data, work);
    const struct device *dev = d->dev;
    const struct paw_config *c = dev->config;
    uint8_t motion, x, y;
    int rc = read_reg(dev, REG_MOTION, &motion);
    if (rc == 0 && (motion & 0x80)) {
        rc = read_reg(dev, REG_X, &x);
        if (rc == 0) rc = read_reg(dev, REG_Y, &y);
        if (rc == 0) {
            /* 8-bit signed deltas, matching the verified PAW3222 register path.
             * Non-blocking input enqueue keeps the system work queue available. */
            if (x || y) {
                rc = input_report_rel(dev, INPUT_REL_X, (int8_t)x, false, K_NO_WAIT);
                if (rc == 0) rc = input_report_rel(dev, INPUT_REL_Y, (int8_t)y, true, K_NO_WAIT);
            }
        }
    }
    if (rc < 0) {
        if (++d->errors <= 3) {
            k_work_reschedule(&d->work, K_MSEC(20));
        } else {
            if (d->errors == 4) LOG_WRN("Motion read failed (%d); retrying at 1 Hz", rc);
            k_work_reschedule(&d->work, K_SECONDS(1));
            d->errors = 4;
        }
        return;
    }
    d->errors = 0;
    /* Edge IRQ plus bounded polling avoids a permanent level-triggered IRQ storm. */
    int active = gpio_pin_get_dt(&c->motion);
    if (active != 0) k_work_reschedule(&d->work, K_MSEC(active < 0 ? 1000 : c->poll_ms));
}
static void motion_irq(const struct device *port, struct gpio_callback *cb, uint32_t pins) {
    ARG_UNUSED(port); ARG_UNUSED(pins);
    struct paw_data *d = CONTAINER_OF(cb, struct paw_data, callback);
    k_work_reschedule(&d->work, K_NO_WAIT);
}

static int sensor_start(const struct device *dev) {
    const struct paw_config *c = dev->config;
    uint8_t id = 0, op;
    int rc = write_reg(dev, REG_CONFIGURATION, 0x80);
    if (rc < 0) return rc;
    k_msleep(2);
    rc = read_reg(dev, REG_ID, &id);
    if (rc < 0) return rc;
    if (id != 0x30) { LOG_WRN("PAW3222 ID=0x%02x (expected 0x30)", id); return -ENODEV; }
    rc = write_reg(dev, REG_PROTECT, 0x5a);
    if (rc < 0) return rc;
    rc = write_reg(dev, REG_CPI_X, c->cpi / 38);
    if (rc == 0) rc = write_reg(dev, REG_CPI_Y, c->cpi / 38);
    if (rc == 0) rc = read_reg(dev, REG_OPERATION, &op);
    if (rc == 0) rc = write_reg(dev, REG_OPERATION, op | 0x18);
    /* CPI and sleep control both require write protection to be disabled. */
    int protect = write_reg(dev, REG_PROTECT, 0x00);
    if (rc < 0) return rc;
    if (protect < 0) return protect;
    const uint8_t clear[] = {REG_MOTION, REG_X, REG_Y, REG_DELTA_HI};
    for (size_t i = 0; i < ARRAY_SIZE(clear); i++) {
        uint8_t ignored;
        rc = read_reg(dev, clear[i], &ignored);
        if (rc < 0) return rc;
    }
    LOG_INF("PAW3222 ready, CPI=%u", c->cpi);
    return 0;
}

static int paw_init(const struct device *dev) {
    const struct paw_config *c = dev->config;
    struct paw_data *d = dev->data;
    d->dev = dev;
    k_mutex_init(&d->lock);
    k_work_init_delayable(&d->work, motion_work);
    if (c->cpi < 608 || c->cpi > 4826 || c->cpi % 38 || !c->half_us || !c->poll_ms)
        return -EINVAL;
    if (!gpio_is_ready_dt(&c->sclk) || !gpio_is_ready_dt(&c->sdio) ||
        !gpio_is_ready_dt(&c->cs) || !gpio_is_ready_dt(&c->motion)) return -ENODEV;
    int rc = gpio_pin_configure_dt(&c->cs, GPIO_OUTPUT_INACTIVE);
    if (rc == 0) rc = gpio_pin_configure_dt(&c->sclk, GPIO_OUTPUT_HIGH);
    if (rc == 0) rc = gpio_pin_configure_dt(&c->sdio, GPIO_INPUT);
    if (rc == 0) rc = gpio_pin_configure_dt(&c->motion, GPIO_INPUT);
    if (rc < 0) return rc;
    d->bus = (struct paw_sdio_io){ .ctx = (void *)dev, .select = select_pin,
        .clock = clock_pin, .direction = direction_pin, .data_out = out_pin,
        .data_in = in_pin, .delay = delay, .half_period_us = c->half_us,
        .turnaround_us = c->turnaround_us, .gap_us = c->gap_us };
    for (unsigned int attempt = 0; attempt < 3; attempt++) {
        rc = sensor_start(dev);
        if (rc == 0) break;
        if (attempt < 2) k_msleep(100);
    }
    if (rc < 0) {
        LOG_ERR("PAW3222 unavailable (%d); keyboard remains usable", rc);
        return rc;
    }
    gpio_init_callback(&d->callback, motion_irq, BIT(c->motion.pin));
    rc = gpio_add_callback(c->motion.port, &d->callback);
    if (rc == 0) rc = gpio_pin_interrupt_configure_dt(&c->motion, GPIO_INT_EDGE_TO_ACTIVE);
    if (rc < 0) return rc;
    /* Also drain motion asserted before the interrupt was enabled. */
    k_work_reschedule(&d->work, K_NO_WAIT);
    return 0;
}

#define PAW_DEFINE(n) \
    static struct paw_data paw_data_##n; \
    static const struct paw_config paw_config_##n = { \
        .sclk = GPIO_DT_SPEC_INST_GET(n, sclk_gpios), \
        .sdio = GPIO_DT_SPEC_INST_GET(n, sdio_gpios), \
        .cs = GPIO_DT_SPEC_INST_GET(n, cs_gpios), \
        .motion = GPIO_DT_SPEC_INST_GET(n, motion_gpios), \
        .cpi = DT_INST_PROP(n, res_cpi), \
        .half_us = DT_INST_PROP(n, clock_half_period_us), \
        .turnaround_us = DT_INST_PROP(n, turnaround_us), \
        .gap_us = DT_INST_PROP(n, transaction_gap_us), \
        .poll_ms = DT_INST_PROP(n, poll_interval_ms), \
    }; \
    DEVICE_DT_INST_DEFINE(n, paw_init, NULL, &paw_data_##n, &paw_config_##n, \
                          POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY, NULL);
DT_INST_FOREACH_STATUS_OKAY(PAW_DEFINE)
