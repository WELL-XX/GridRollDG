/* SPDX-License-Identifier: MIT; minimal host shim for the actual PAW driver. */
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#define ARG_UNUSED(x) (void)(x)
#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))
#define BIT(n) (1U << (n))
#define CONTAINER_OF(ptr, type, member) ((type *)((char *)(ptr)-offsetof(type, member)))
#define DT_INST_FOREACH_STATUS_OKAY(fn)
#define CONFIG_GRIDROLLDG_PAW3222_LOG_LEVEL 1
#define LOG_MODULE_REGISTER(...)
#define LOG_INF(...)
#define LOG_WRN(...)
#define LOG_ERR(...)
#define K_FOREVER (-1)
#define K_NO_WAIT 0
#define K_MSEC(ms) (ms)
#define K_SECONDS(s) ((s)*1000)
#define GPIO_OUTPUT_HIGH 1
#define GPIO_OUTPUT_LOW 2
#define GPIO_OUTPUT_INACTIVE 3
#define GPIO_INPUT 4
#define GPIO_INT_EDGE_TO_ACTIVE 5
#define INPUT_REL_X 0
#define INPUT_REL_Y 1
struct device { const void *config; void *data; };
struct gpio_dt_spec { const struct device *port; uint32_t pin; uint32_t dt_flags; };
struct gpio_callback { int unused; };
struct k_mutex { int unused; };
struct k_work { int unused; };
struct k_work_delayable { struct k_work work; };
static int scheduled_ms, input_count, input_values[8], input_codes[8], interrupts;
static bool input_sync[8];
static bool gpio_is_ready_dt(const struct gpio_dt_spec *s) { return s->port != NULL; }
static int gpio_pin_set_dt(const struct gpio_dt_spec *s, int v) { ARG_UNUSED(s); ARG_UNUSED(v); return 0; }
static int gpio_pin_configure_dt(const struct gpio_dt_spec *s, int v) { ARG_UNUSED(s); ARG_UNUSED(v); return 0; }
static int gpio_pin_get_dt(const struct gpio_dt_spec *s) { ARG_UNUSED(s); return 0; }
static void gpio_init_callback(struct gpio_callback *c,
    void (*fn)(const struct device *, struct gpio_callback *, uint32_t), uint32_t pins) {
    ARG_UNUSED(c); ARG_UNUSED(fn); ARG_UNUSED(pins);
}
static int gpio_add_callback(const struct device *d, struct gpio_callback *c) {
    ARG_UNUSED(d); ARG_UNUSED(c); interrupts++; return 0;
}
static int gpio_pin_interrupt_configure_dt(const struct gpio_dt_spec *s, int flags) {
    ARG_UNUSED(s); ARG_UNUSED(flags); interrupts++; return 0;
}
static void k_busy_wait(unsigned int us) { ARG_UNUSED(us); }
static void k_msleep(int ms) { ARG_UNUSED(ms); }
static void k_mutex_init(struct k_mutex *m) { ARG_UNUSED(m); }
static int k_mutex_lock(struct k_mutex *m, int t) { ARG_UNUSED(m); ARG_UNUSED(t); return 0; }
static void k_mutex_unlock(struct k_mutex *m) { ARG_UNUSED(m); }
static void k_work_init_delayable(struct k_work_delayable *w, void (*fn)(struct k_work *)) {
    ARG_UNUSED(w); ARG_UNUSED(fn);
}
static struct k_work_delayable *k_work_delayable_from_work(struct k_work *w) {
    return CONTAINER_OF(w, struct k_work_delayable, work);
}
static int k_work_reschedule(struct k_work_delayable *w, int ms) {
    ARG_UNUSED(w); scheduled_ms = ms; return 0;
}
static int input_report_rel(const struct device *dev, uint16_t code, int32_t value,
                            bool sync, int timeout) {
    ARG_UNUSED(dev); ARG_UNUSED(timeout);
    input_codes[input_count] = code; input_values[input_count] = value;
    input_sync[input_count++] = sync; return 0;
}
