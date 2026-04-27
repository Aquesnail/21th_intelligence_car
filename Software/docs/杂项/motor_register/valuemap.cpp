#include "valuemap.hpp"

#include <cstdint>

#include "foc_soft.hpp"

extern foc_motor_datastructure foc_motor_datastructure_A;
extern foc_motor_datastructure foc_motor_datastructure_B;

namespace
{
enum value_addr : uint8_t
{
    A_adc_vref = 0x10u,
    A_vbus_divider_ratio = 0x11u,
    A_shunt_resistance = 0x12u,
    A_current_sense_gain = 0x13u,
    A_pid_d_kp = 0x14u,
    A_pid_d_ki = 0x15u,
    A_pid_d_kd = 0x16u,
    A_pid_q_kp = 0x17u,
    A_pid_q_ki = 0x18u,
    A_pid_q_kd = 0x19u,
    A_adc_full_scale = 0x1Au,

    B_adc_vref = 0x20u,
    B_vbus_divider_ratio = 0x21u,
    B_shunt_resistance = 0x22u,
    B_current_sense_gain = 0x23u,
    B_pid_d_kp = 0x24u,
    B_pid_d_ki = 0x25u,
    B_pid_d_kd = 0x26u,
    B_pid_q_kp = 0x27u,
    B_pid_q_ki = 0x28u,
    B_pid_q_kd = 0x29u,
    B_adc_full_scale = 0x2Au
};

template <typename T>
bool register_rw(spi_decode& decoder, uint8_t addr, T& ref)
{
    return decoder.register_map(addr, ref, true, true);
}
} // namespace

bool valuemap_register(spi_decode& decoder)
{
    bool ok = true;

    ok &= register_rw(decoder, A_adc_vref, foc_motor_datastructure_A.adc_vref);
    ok &= register_rw(decoder, A_vbus_divider_ratio, foc_motor_datastructure_A.vbus_divider_ratio);
    ok &= register_rw(decoder, A_shunt_resistance, foc_motor_datastructure_A.shunt_resistance);
    ok &= register_rw(decoder, A_current_sense_gain, foc_motor_datastructure_A.current_sense_gain);
    ok &= register_rw(decoder, A_pid_d_kp, foc_motor_datastructure_A.pid_d_kp);
    ok &= register_rw(decoder, A_pid_d_ki, foc_motor_datastructure_A.pid_d_ki);
    ok &= register_rw(decoder, A_pid_d_kd, foc_motor_datastructure_A.pid_d_kd);
    ok &= register_rw(decoder, A_pid_q_kp, foc_motor_datastructure_A.pid_q_kp);
    ok &= register_rw(decoder, A_pid_q_ki, foc_motor_datastructure_A.pid_q_ki);
    ok &= register_rw(decoder, A_pid_q_kd, foc_motor_datastructure_A.pid_q_kd);
    ok &= register_rw(decoder, A_adc_full_scale, foc_motor_datastructure_A.adc_full_scale);

    ok &= register_rw(decoder, B_adc_vref, foc_motor_datastructure_B.adc_vref);
    ok &= register_rw(decoder, B_vbus_divider_ratio, foc_motor_datastructure_B.vbus_divider_ratio);
    ok &= register_rw(decoder, B_shunt_resistance, foc_motor_datastructure_B.shunt_resistance);
    ok &= register_rw(decoder, B_current_sense_gain, foc_motor_datastructure_B.current_sense_gain);
    ok &= register_rw(decoder, B_pid_d_kp, foc_motor_datastructure_B.pid_d_kp);
    ok &= register_rw(decoder, B_pid_d_ki, foc_motor_datastructure_B.pid_d_ki);
    ok &= register_rw(decoder, B_pid_d_kd, foc_motor_datastructure_B.pid_d_kd);
    ok &= register_rw(decoder, B_pid_q_kp, foc_motor_datastructure_B.pid_q_kp);
    ok &= register_rw(decoder, B_pid_q_ki, foc_motor_datastructure_B.pid_q_ki);
    ok &= register_rw(decoder, B_pid_q_kd, foc_motor_datastructure_B.pid_q_kd);
    ok &= register_rw(decoder, B_adc_full_scale, foc_motor_datastructure_B.adc_full_scale);

    return ok;
}
