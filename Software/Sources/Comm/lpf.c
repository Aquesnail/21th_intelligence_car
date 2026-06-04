#include "lpf.h"
#include <math.h>

void lpf_init(Lpf_t *f, float alpha)
{
    f->alpha  = alpha;
    f->output = 0.0f;
}

void lpf_init_fc(Lpf_t *f, float fc, float dt)
{
    f->freq = fc;
    f->alpha = 1.0f - (float)exp(-6.2831853 * fc * dt);
    f->output = 0.0f;
}

float lpf_update(Lpf_t *f, float input)
{
    f->output += f->alpha * (input - f->output);
    return f->output;
}

void lpf_reset(Lpf_t *f, float value)
{
    f->output = value;
}
