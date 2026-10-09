/* SPDX-License-Identifier: MIT */
#define DT_DRV_COMPAT wellxx_behavior_wheel
#include <zephyr/device.h>
#include <zephyr/input/input.h>
#include <drivers/behavior.h>
#include <zmk/behavior.h>

struct wheel_config {
    int16_t delta;
};

static int pressed(struct zmk_behavior_binding *binding,
                   struct zmk_behavior_binding_event event) {
    ARG_UNUSED(event);
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct wheel_config *config = dev->config;
    return input_report_rel(dev, INPUT_REL_WHEEL, config->delta, true, K_NO_WAIT);
}

static int released(struct zmk_behavior_binding *binding,
                    struct zmk_behavior_binding_event event) {
    ARG_UNUSED(binding);
    ARG_UNUSED(event);
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api wheel_api = {
    .binding_pressed = pressed,
    .binding_released = released,
    .locality = BEHAVIOR_LOCALITY_CENTRAL,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .get_parameter_metadata = zmk_behavior_get_empty_param_metadata,
#endif
};

#define WHEEL_DEFINE(n) \
    static const struct wheel_config wheel_config_##n = { \
        .delta = (int16_t)DT_INST_PROP(n, delta), \
    }; \
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, NULL, &wheel_config_##n, \
                           POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY, &wheel_api);
DT_INST_FOREACH_STATUS_OKAY(WHEEL_DEFINE)
