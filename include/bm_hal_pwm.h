#ifndef BM_HAL_PWM_H
#define BM_HAL_PWM_H

#include "bm_hrt.h"
#include "bm_types.h"

typedef struct bm_hal_pwm bm_hal_pwm_t;

typedef struct bm_hal_hrt_binding {
    bm_hrt_callback_t callback;
    void *context;
} bm_hal_hrt_binding_t;

int bm_hal_pwm_set_duty(const bm_hal_pwm_t *pwm, uint32_t phase, uint16_t duty);
int bm_hal_pwm_enable_outputs(const bm_hal_pwm_t *pwm, int enable);
void bm_hal_pwm_request_safe_state(const bm_hal_pwm_t *pwm);
int bm_hal_pwm_bind_update(const bm_hal_pwm_t *pwm,
                           const bm_hal_hrt_binding_t *binding);

#endif /* BM_HAL_PWM_H */
