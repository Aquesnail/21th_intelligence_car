/**
 ******************************************************************************
 * Copyright (c) 2020, STMicroelectronics - All Rights Reserved
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */




#include "vl53l1_ll_def.h"
#include "vl53l1_platform.h"
#include "vl53l1_platform_log.h"
#include "vl53l1_core.h"
#include "vl53l1_register_map.h"
#include "vl53l1_register_structs.h"
#include "vl53l1_register_funcs.h"

// #undef LOG_FUNCTION_START
// #define LOG_FUNCTION_START(fmt, ...) \
// 	_LOG_FUNCTION_START(VL53L1_TRACE_MODULE_REGISTERS, fmt, __VA_ARGS__)
// #undef LOG_FUNCTION_END
// #define LOG_FUNCTION_END(status, ...) \
// 	_LOG_FUNCTION_END(VL53L1_TRACE_MODULE_REGISTERS, status, __VA_ARGS__)
// #undef LOG_FUNCTION_END_FMT
// #define LOG_FUNCTION_END_FMT(status, fmt, ...) \
// 	_LOG_FUNCTION_END_FMT(VL53L1_TRACE_MODULE_REGISTERS,\
// 			status, fmt, __VA_ARGS__)


VL53L1_Error VL53L1_i2c_encode_static_nvm_managed(
	VL53L1_static_nvm_managed_t *pdat,
	uint16_t                  buf_size,
	uint8_t                  *pbuffer)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (buf_size < VL53L1_STATIC_NVM_MANAGED_I2C_SIZE_BYTES)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	*(pbuffer +   0) =
		pdat->i2c_slave__device_address & 0x7F;
	*(pbuffer +   1) =
		pdat->ana_config__vhv_ref_sel_vddpix & 0xF;
	*(pbuffer +   2) =
		pdat->ana_config__vhv_ref_sel_vquench & 0x7F;
	*(pbuffer +   3) =
		pdat->ana_config__reg_avdd1v2_sel & 0x3;
	*(pbuffer +   4) =
		pdat->ana_config__fast_osc__trim & 0x7F;
	VL53L1_i2c_encode_uint16_t(
		pdat->osc_measured__fast_osc__frequency,
		2,
		pbuffer +   5);
	*(pbuffer +   7) =
		pdat->vhv_config__timeout_macrop_loop_bound;
	*(pbuffer +   8) =
		pdat->vhv_config__count_thresh;
	*(pbuffer +   9) =
		pdat->vhv_config__offset & 0x3F;
	*(pbuffer +  10) =
		pdat->vhv_config__init;
	LOG_FUNCTION_END(status);


	return status;
}


VL53L1_Error VL53L1_i2c_decode_static_nvm_managed(
	uint16_t                   buf_size,
	uint8_t                   *pbuffer,
	VL53L1_static_nvm_managed_t  *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (buf_size < VL53L1_STATIC_NVM_MANAGED_I2C_SIZE_BYTES)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	pdat->i2c_slave__device_address =
		(*(pbuffer +   0)) & 0x7F;
	pdat->ana_config__vhv_ref_sel_vddpix =
		(*(pbuffer +   1)) & 0xF;
	pdat->ana_config__vhv_ref_sel_vquench =
		(*(pbuffer +   2)) & 0x7F;
	pdat->ana_config__reg_avdd1v2_sel =
		(*(pbuffer +   3)) & 0x3;
	pdat->ana_config__fast_osc__trim =
		(*(pbuffer +   4)) & 0x7F;
	pdat->osc_measured__fast_osc__frequency =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +   5));
	pdat->vhv_config__timeout_macrop_loop_bound =
		(*(pbuffer +   7));
	pdat->vhv_config__count_thresh =
		(*(pbuffer +   8));
	pdat->vhv_config__offset =
		(*(pbuffer +   9)) & 0x3F;
	pdat->vhv_config__init =
		(*(pbuffer +  10));

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_set_static_nvm_managed(
	VL53L1_DEV                 Dev,
	VL53L1_static_nvm_managed_t  *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_STATIC_NVM_MANAGED_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_encode_static_nvm_managed(
			pdat,
			VL53L1_STATIC_NVM_MANAGED_I2C_SIZE_BYTES,
			comms_buffer);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_WriteMulti(
			Dev,
			VL53L1_I2C_SLAVE__DEVICE_ADDRESS,
			comms_buffer,
			VL53L1_STATIC_NVM_MANAGED_I2C_SIZE_BYTES);

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_get_static_nvm_managed(
	VL53L1_DEV                 Dev,
	VL53L1_static_nvm_managed_t  *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_STATIC_NVM_MANAGED_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_ReadMulti(
			Dev,
			VL53L1_I2C_SLAVE__DEVICE_ADDRESS,
			comms_buffer,
			VL53L1_STATIC_NVM_MANAGED_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_decode_static_nvm_managed(
			VL53L1_STATIC_NVM_MANAGED_I2C_SIZE_BYTES,
			comms_buffer,
			pdat);

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_i2c_encode_customer_nvm_managed(
	VL53L1_customer_nvm_managed_t *pdat,
	uint16_t                  buf_size,
	uint8_t                  *pbuffer)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (buf_size < VL53L1_CUSTOMER_NVM_MANAGED_I2C_SIZE_BYTES)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	*(pbuffer +   0) =
		pdat->global_config__spad_enables_ref_0;
	*(pbuffer +   1) =
		pdat->global_config__spad_enables_ref_1;
	*(pbuffer +   2) =
		pdat->global_config__spad_enables_ref_2;
	*(pbuffer +   3) =
		pdat->global_config__spad_enables_ref_3;
	*(pbuffer +   4) =
		pdat->global_config__spad_enables_ref_4;
	*(pbuffer +   5) =
		pdat->global_config__spad_enables_ref_5 & 0xF;
	*(pbuffer +   6) =
		pdat->global_config__ref_en_start_select;
	*(pbuffer +   7) =
		pdat->ref_spad_man__num_requested_ref_spads & 0x3F;
	*(pbuffer +   8) =
		pdat->ref_spad_man__ref_location & 0x3;
	VL53L1_i2c_encode_uint16_t(
		pdat->algo__crosstalk_compensation_plane_offset_kcps,
		2,
		pbuffer +   9);
	VL53L1_i2c_encode_int16_t(
		pdat->algo__crosstalk_compensation_x_plane_gradient_kcps,
		2,
		pbuffer +  11);
	VL53L1_i2c_encode_int16_t(
		pdat->algo__crosstalk_compensation_y_plane_gradient_kcps,
		2,
		pbuffer +  13);
	VL53L1_i2c_encode_uint16_t(
		pdat->ref_spad_char__total_rate_target_mcps,
		2,
		pbuffer +  15);
	VL53L1_i2c_encode_int16_t(
		pdat->algo__part_to_part_range_offset_mm & 0x1FFF,
		2,
		pbuffer +  17);
	VL53L1_i2c_encode_int16_t(
		pdat->mm_config__inner_offset_mm,
		2,
		pbuffer +  19);
	VL53L1_i2c_encode_int16_t(
		pdat->mm_config__outer_offset_mm,
		2,
		pbuffer +  21);
	LOG_FUNCTION_END(status);


	return status;
}


VL53L1_Error VL53L1_i2c_decode_customer_nvm_managed(
	uint16_t                   buf_size,
	uint8_t                   *pbuffer,
	VL53L1_customer_nvm_managed_t  *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (buf_size < VL53L1_CUSTOMER_NVM_MANAGED_I2C_SIZE_BYTES)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	pdat->global_config__spad_enables_ref_0 =
		(*(pbuffer +   0));
	pdat->global_config__spad_enables_ref_1 =
		(*(pbuffer +   1));
	pdat->global_config__spad_enables_ref_2 =
		(*(pbuffer +   2));
	pdat->global_config__spad_enables_ref_3 =
		(*(pbuffer +   3));
	pdat->global_config__spad_enables_ref_4 =
		(*(pbuffer +   4));
	pdat->global_config__spad_enables_ref_5 =
		(*(pbuffer +   5)) & 0xF;
	pdat->global_config__ref_en_start_select =
		(*(pbuffer +   6));
	pdat->ref_spad_man__num_requested_ref_spads =
		(*(pbuffer +   7)) & 0x3F;
	pdat->ref_spad_man__ref_location =
		(*(pbuffer +   8)) & 0x3;
	pdat->algo__crosstalk_compensation_plane_offset_kcps =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +   9));
	pdat->algo__crosstalk_compensation_x_plane_gradient_kcps =
		(VL53L1_i2c_decode_int16_t(2, pbuffer +  11));
	pdat->algo__crosstalk_compensation_y_plane_gradient_kcps =
		(VL53L1_i2c_decode_int16_t(2, pbuffer +  13));
	pdat->ref_spad_char__total_rate_target_mcps =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  15));
	pdat->algo__part_to_part_range_offset_mm =
		(VL53L1_i2c_decode_int16_t(2, pbuffer +  17)) & 0x1FFF;
	pdat->mm_config__inner_offset_mm =
		(VL53L1_i2c_decode_int16_t(2, pbuffer +  19));
	pdat->mm_config__outer_offset_mm =
		(VL53L1_i2c_decode_int16_t(2, pbuffer +  21));

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_set_customer_nvm_managed(
	VL53L1_DEV                 Dev,
	VL53L1_customer_nvm_managed_t  *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_CUSTOMER_NVM_MANAGED_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_encode_customer_nvm_managed(
			pdat,
			VL53L1_CUSTOMER_NVM_MANAGED_I2C_SIZE_BYTES,
			comms_buffer);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_WriteMulti(
			Dev,
			VL53L1_GLOBAL_CONFIG__SPAD_ENABLES_REF_0,
			comms_buffer,
			VL53L1_CUSTOMER_NVM_MANAGED_I2C_SIZE_BYTES);

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_get_customer_nvm_managed(
	VL53L1_DEV                 Dev,
	VL53L1_customer_nvm_managed_t  *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_CUSTOMER_NVM_MANAGED_I2C_SIZE_BYTES];
	VL53L1_LLDriverData_t   *pLLData;
	uint8_t module_type;
	int16_t offset;

	LOG_FUNCTION_START("");

	pLLData =  VL53L1DevStructGetLLDriverHandle(Dev);
	module_type = pLLData->nvm_copy_data.identification__module_type;

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_ReadMulti(
			Dev,
			VL53L1_GLOBAL_CONFIG__SPAD_ENABLES_REF_0,
			comms_buffer,
			VL53L1_CUSTOMER_NVM_MANAGED_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_decode_customer_nvm_managed(
			VL53L1_CUSTOMER_NVM_MANAGED_I2C_SIZE_BYTES,
			comms_buffer,
			pdat);


	if ((status == VL53L1_ERROR_NONE) && (module_type == 0xAA)) {
		offset = pdat->algo__part_to_part_range_offset_mm;
		offset = offset / 4;
		if (offset >= 1024)
			offset -= 2048;
		pdat->algo__part_to_part_range_offset_mm = 0;
		pdat->mm_config__inner_offset_mm = offset;
		pdat->mm_config__outer_offset_mm = offset;
	}

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_i2c_encode_static_config(
	VL53L1_static_config_t   *pdat,
	uint16_t                  buf_size,
	uint8_t                  *pbuffer)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (buf_size < VL53L1_STATIC_CONFIG_I2C_SIZE_BYTES)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	VL53L1_i2c_encode_uint16_t(
		pdat->dss_config__target_total_rate_mcps,
		2,
		pbuffer +   0);
	*(pbuffer +   2) =
		pdat->debug__ctrl & 0x1;
	*(pbuffer +   3) =
		pdat->test_mode__ctrl & 0xF;
	*(pbuffer +   4) =
		pdat->clk_gating__ctrl & 0xF;
	*(pbuffer +   5) =
		pdat->nvm_bist__ctrl & 0x1F;
	*(pbuffer +   6) =
		pdat->nvm_bist__num_nvm_words & 0x7F;
	*(pbuffer +   7) =
		pdat->nvm_bist__start_address & 0x7F;
	*(pbuffer +   8) =
		pdat->host_if__status & 0x1;
	*(pbuffer +   9) =
		pdat->pad_i2c_hv__config;
	*(pbuffer +  10) =
		pdat->pad_i2c_hv__extsup_config & 0x1;
	*(pbuffer +  11) =
		pdat->gpio_hv_pad__ctrl & 0x3;
	*(pbuffer +  12) =
		pdat->gpio_hv_mux__ctrl & 0x1F;
	*(pbuffer +  13) =
		pdat->gpio__tio_hv_status & 0x3;
	*(pbuffer +  14) =
		pdat->gpio__fio_hv_status & 0x3;
	*(pbuffer +  15) =
		pdat->ana_config__spad_sel_pswidth & 0x7;
	*(pbuffer +  16) =
		pdat->ana_config__vcsel_pulse_width_offset & 0x1F;
	*(pbuffer +  17) =
		pdat->ana_config__fast_osc__config_ctrl & 0x1;
	*(pbuffer +  18) =
		pdat->sigma_estimator__effective_pulse_width_ns;
	*(pbuffer +  19) =
		pdat->sigma_estimator__effective_ambient_width_ns;
	*(pbuffer +  20) =
		pdat->sigma_estimator__sigma_ref_mm;
	*(pbuffer +  21) =
		pdat->algo__crosstalk_compensation_valid_height_mm;
	*(pbuffer +  22) =
		pdat->spare_host_config__static_config_spare_0;
	*(pbuffer +  23) =
		pdat->spare_host_config__static_config_spare_1;
	VL53L1_i2c_encode_uint16_t(
		pdat->algo__range_ignore_threshold_mcps,
		2,
		pbuffer +  24);
	*(pbuffer +  26) =
		pdat->algo__range_ignore_valid_height_mm;
	*(pbuffer +  27) =
		pdat->algo__range_min_clip;
	*(pbuffer +  28) =
		pdat->algo__consistency_check__tolerance & 0xF;
	*(pbuffer +  29) =
		pdat->spare_host_config__static_config_spare_2;
	*(pbuffer +  30) =
		pdat->sd_config__reset_stages_msb & 0xF;
	*(pbuffer +  31) =
		pdat->sd_config__reset_stages_lsb;
	LOG_FUNCTION_END(status);


	return status;
}


VL53L1_Error VL53L1_i2c_decode_static_config(
	uint16_t                   buf_size,
	uint8_t                   *pbuffer,
	VL53L1_static_config_t    *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (buf_size < VL53L1_STATIC_CONFIG_I2C_SIZE_BYTES)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	pdat->dss_config__target_total_rate_mcps =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +   0));
	pdat->debug__ctrl =
		(*(pbuffer +   2)) & 0x1;
	pdat->test_mode__ctrl =
		(*(pbuffer +   3)) & 0xF;
	pdat->clk_gating__ctrl =
		(*(pbuffer +   4)) & 0xF;
	pdat->nvm_bist__ctrl =
		(*(pbuffer +   5)) & 0x1F;
	pdat->nvm_bist__num_nvm_words =
		(*(pbuffer +   6)) & 0x7F;
	pdat->nvm_bist__start_address =
		(*(pbuffer +   7)) & 0x7F;
	pdat->host_if__status =
		(*(pbuffer +   8)) & 0x1;
	pdat->pad_i2c_hv__config =
		(*(pbuffer +   9));
	pdat->pad_i2c_hv__extsup_config =
		(*(pbuffer +  10)) & 0x1;
	pdat->gpio_hv_pad__ctrl =
		(*(pbuffer +  11)) & 0x3;
	pdat->gpio_hv_mux__ctrl =
		(*(pbuffer +  12)) & 0x1F;
	pdat->gpio__tio_hv_status =
		(*(pbuffer +  13)) & 0x3;
	pdat->gpio__fio_hv_status =
		(*(pbuffer +  14)) & 0x3;
	pdat->ana_config__spad_sel_pswidth =
		(*(pbuffer +  15)) & 0x7;
	pdat->ana_config__vcsel_pulse_width_offset =
		(*(pbuffer +  16)) & 0x1F;
	pdat->ana_config__fast_osc__config_ctrl =
		(*(pbuffer +  17)) & 0x1;
	pdat->sigma_estimator__effective_pulse_width_ns =
		(*(pbuffer +  18));
	pdat->sigma_estimator__effective_ambient_width_ns =
		(*(pbuffer +  19));
	pdat->sigma_estimator__sigma_ref_mm =
		(*(pbuffer +  20));
	pdat->algo__crosstalk_compensation_valid_height_mm =
		(*(pbuffer +  21));
	pdat->spare_host_config__static_config_spare_0 =
		(*(pbuffer +  22));
	pdat->spare_host_config__static_config_spare_1 =
		(*(pbuffer +  23));
	pdat->algo__range_ignore_threshold_mcps =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  24));
	pdat->algo__range_ignore_valid_height_mm =
		(*(pbuffer +  26));
	pdat->algo__range_min_clip =
		(*(pbuffer +  27));
	pdat->algo__consistency_check__tolerance =
		(*(pbuffer +  28)) & 0xF;
	pdat->spare_host_config__static_config_spare_2 =
		(*(pbuffer +  29));
	pdat->sd_config__reset_stages_msb =
		(*(pbuffer +  30)) & 0xF;
	pdat->sd_config__reset_stages_lsb =
		(*(pbuffer +  31));

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_set_static_config(
	VL53L1_DEV                 Dev,
	VL53L1_static_config_t    *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_STATIC_CONFIG_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_encode_static_config(
			pdat,
			VL53L1_STATIC_CONFIG_I2C_SIZE_BYTES,
			comms_buffer);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_WriteMulti(
			Dev,
			VL53L1_DSS_CONFIG__TARGET_TOTAL_RATE_MCPS,
			comms_buffer,
			VL53L1_STATIC_CONFIG_I2C_SIZE_BYTES);

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_get_static_config(
	VL53L1_DEV                 Dev,
	VL53L1_static_config_t    *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_STATIC_CONFIG_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_ReadMulti(
			Dev,
			VL53L1_DSS_CONFIG__TARGET_TOTAL_RATE_MCPS,
			comms_buffer,
			VL53L1_STATIC_CONFIG_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_decode_static_config(
			VL53L1_STATIC_CONFIG_I2C_SIZE_BYTES,
			comms_buffer,
			pdat);

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_i2c_encode_general_config(
	VL53L1_general_config_t  *pdat,
	uint16_t                  buf_size,
	uint8_t                  *pbuffer)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (buf_size < VL53L1_GENERAL_CONFIG_I2C_SIZE_BYTES)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	*(pbuffer +   0) =
		pdat->gph_config__stream_count_update_value;
	*(pbuffer +   1) =
		pdat->global_config__stream_divider;
	*(pbuffer +   2) =
		pdat->system__interrupt_config_gpio;
	*(pbuffer +   3) =
		pdat->cal_config__vcsel_start & 0x7F;
	VL53L1_i2c_encode_uint16_t(
		pdat->cal_config__repeat_rate & 0xFFF,
		2,
		pbuffer +   4);
	*(pbuffer +   6) =
		pdat->global_config__vcsel_width & 0x7F;
	*(pbuffer +   7) =
		pdat->phasecal_config__timeout_macrop;
	*(pbuffer +   8) =
		pdat->phasecal_config__target;
	*(pbuffer +   9) =
		pdat->phasecal_config__override & 0x1;
	*(pbuffer +  11) =
		pdat->dss_config__roi_mode_control & 0x7;
	VL53L1_i2c_encode_uint16_t(
		pdat->system__thresh_rate_high,
		2,
		pbuffer +  12);
	VL53L1_i2c_encode_uint16_t(
		pdat->system__thresh_rate_low,
		2,
		pbuffer +  14);
	VL53L1_i2c_encode_uint16_t(
		pdat->dss_config__manual_effective_spads_select,
		2,
		pbuffer +  16);
	*(pbuffer +  18) =
		pdat->dss_config__manual_block_select;
	*(pbuffer +  19) =
		pdat->dss_config__aperture_attenuation;
	*(pbuffer +  20) =
		pdat->dss_config__max_spads_limit;
	*(pbuffer +  21) =
		pdat->dss_config__min_spads_limit;
	LOG_FUNCTION_END(status);


	return status;
}


VL53L1_Error VL53L1_i2c_decode_general_config(
	uint16_t                   buf_size,
	uint8_t                   *pbuffer,
	VL53L1_general_config_t   *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (buf_size < VL53L1_GENERAL_CONFIG_I2C_SIZE_BYTES)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	pdat->gph_config__stream_count_update_value =
		(*(pbuffer +   0));
	pdat->global_config__stream_divider =
		(*(pbuffer +   1));
	pdat->system__interrupt_config_gpio =
		(*(pbuffer +   2));
	pdat->cal_config__vcsel_start =
		(*(pbuffer +   3)) & 0x7F;
	pdat->cal_config__repeat_rate =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +   4)) & 0xFFF;
	pdat->global_config__vcsel_width =
		(*(pbuffer +   6)) & 0x7F;
	pdat->phasecal_config__timeout_macrop =
		(*(pbuffer +   7));
	pdat->phasecal_config__target =
		(*(pbuffer +   8));
	pdat->phasecal_config__override =
		(*(pbuffer +   9)) & 0x1;
	pdat->dss_config__roi_mode_control =
		(*(pbuffer +  11)) & 0x7;
	pdat->system__thresh_rate_high =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  12));
	pdat->system__thresh_rate_low =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  14));
	pdat->dss_config__manual_effective_spads_select =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  16));
	pdat->dss_config__manual_block_select =
		(*(pbuffer +  18));
	pdat->dss_config__aperture_attenuation =
		(*(pbuffer +  19));
	pdat->dss_config__max_spads_limit =
		(*(pbuffer +  20));
	pdat->dss_config__min_spads_limit =
		(*(pbuffer +  21));

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_set_general_config(
	VL53L1_DEV                 Dev,
	VL53L1_general_config_t   *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_GENERAL_CONFIG_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_encode_general_config(
			pdat,
			VL53L1_GENERAL_CONFIG_I2C_SIZE_BYTES,
			comms_buffer);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_WriteMulti(
			Dev,
			VL53L1_GPH_CONFIG__STREAM_COUNT_UPDATE_VALUE,
			comms_buffer,
			VL53L1_GENERAL_CONFIG_I2C_SIZE_BYTES);

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_get_general_config(
	VL53L1_DEV                 Dev,
	VL53L1_general_config_t   *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_GENERAL_CONFIG_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_ReadMulti(
			Dev,
			VL53L1_GPH_CONFIG__STREAM_COUNT_UPDATE_VALUE,
			comms_buffer,
			VL53L1_GENERAL_CONFIG_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_decode_general_config(
			VL53L1_GENERAL_CONFIG_I2C_SIZE_BYTES,
			comms_buffer,
			pdat);

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_i2c_encode_timing_config(
	VL53L1_timing_config_t   *pdat,
	uint16_t                  buf_size,
	uint8_t                  *pbuffer)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (buf_size < VL53L1_TIMING_CONFIG_I2C_SIZE_BYTES)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	*(pbuffer +   0) =
		pdat->mm_config__timeout_macrop_a_hi & 0xF;
	*(pbuffer +   1) =
		pdat->mm_config__timeout_macrop_a_lo;
	*(pbuffer +   2) =
		pdat->mm_config__timeout_macrop_b_hi & 0xF;
	*(pbuffer +   3) =
		pdat->mm_config__timeout_macrop_b_lo;
	*(pbuffer +   4) =
		pdat->range_config__timeout_macrop_a_hi & 0xF;
	*(pbuffer +   5) =
		pdat->range_config__timeout_macrop_a_lo;
	*(pbuffer +   6) =
		pdat->range_config__vcsel_period_a & 0x3F;
	*(pbuffer +   7) =
		pdat->range_config__timeout_macrop_b_hi & 0xF;
	*(pbuffer +   8) =
		pdat->range_config__timeout_macrop_b_lo;
	*(pbuffer +   9) =
		pdat->range_config__vcsel_period_b & 0x3F;
	VL53L1_i2c_encode_uint16_t(
		pdat->range_config__sigma_thresh,
		2,
		pbuffer +  10);
	VL53L1_i2c_encode_uint16_t(
		pdat->range_config__min_count_rate_rtn_limit_mcps,
		2,
		pbuffer +  12);
	*(pbuffer +  14) =
		pdat->range_config__valid_phase_low;
	*(pbuffer +  15) =
		pdat->range_config__valid_phase_high;
	VL53L1_i2c_encode_uint32_t(
		pdat->system__intermeasurement_period,
		4,
		pbuffer +  18);
	*(pbuffer +  22) =
		pdat->system__fractional_enable & 0x1;
	LOG_FUNCTION_END(status);


	return status;
}


VL53L1_Error VL53L1_i2c_decode_timing_config(
	uint16_t                   buf_size,
	uint8_t                   *pbuffer,
	VL53L1_timing_config_t    *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (buf_size < VL53L1_TIMING_CONFIG_I2C_SIZE_BYTES)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	pdat->mm_config__timeout_macrop_a_hi =
		(*(pbuffer +   0)) & 0xF;
	pdat->mm_config__timeout_macrop_a_lo =
		(*(pbuffer +   1));
	pdat->mm_config__timeout_macrop_b_hi =
		(*(pbuffer +   2)) & 0xF;
	pdat->mm_config__timeout_macrop_b_lo =
		(*(pbuffer +   3));
	pdat->range_config__timeout_macrop_a_hi =
		(*(pbuffer +   4)) & 0xF;
	pdat->range_config__timeout_macrop_a_lo =
		(*(pbuffer +   5));
	pdat->range_config__vcsel_period_a =
		(*(pbuffer +   6)) & 0x3F;
	pdat->range_config__timeout_macrop_b_hi =
		(*(pbuffer +   7)) & 0xF;
	pdat->range_config__timeout_macrop_b_lo =
		(*(pbuffer +   8));
	pdat->range_config__vcsel_period_b =
		(*(pbuffer +   9)) & 0x3F;
	pdat->range_config__sigma_thresh =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  10));
	pdat->range_config__min_count_rate_rtn_limit_mcps =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  12));
	pdat->range_config__valid_phase_low =
		(*(pbuffer +  14));
	pdat->range_config__valid_phase_high =
		(*(pbuffer +  15));
	pdat->system__intermeasurement_period =
		(VL53L1_i2c_decode_uint32_t(4, pbuffer +  18));
	pdat->system__fractional_enable =
		(*(pbuffer +  22)) & 0x1;

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_set_timing_config(
	VL53L1_DEV                 Dev,
	VL53L1_timing_config_t    *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_TIMING_CONFIG_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_encode_timing_config(
			pdat,
			VL53L1_TIMING_CONFIG_I2C_SIZE_BYTES,
			comms_buffer);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_WriteMulti(
			Dev,
			VL53L1_MM_CONFIG__TIMEOUT_MACROP_A_HI,
			comms_buffer,
			VL53L1_TIMING_CONFIG_I2C_SIZE_BYTES);

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_get_timing_config(
	VL53L1_DEV                 Dev,
	VL53L1_timing_config_t    *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_TIMING_CONFIG_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_ReadMulti(
			Dev,
			VL53L1_MM_CONFIG__TIMEOUT_MACROP_A_HI,
			comms_buffer,
			VL53L1_TIMING_CONFIG_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_decode_timing_config(
			VL53L1_TIMING_CONFIG_I2C_SIZE_BYTES,
			comms_buffer,
			pdat);

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_i2c_encode_dynamic_config(
	VL53L1_dynamic_config_t  *pdat,
	uint16_t                  buf_size,
	uint8_t                  *pbuffer)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (buf_size < VL53L1_DYNAMIC_CONFIG_I2C_SIZE_BYTES)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	*(pbuffer +   0) =
		pdat->system__grouped_parameter_hold_0 & 0x3;
	VL53L1_i2c_encode_uint16_t(
		pdat->system__thresh_high,
		2,
		pbuffer +   1);
	VL53L1_i2c_encode_uint16_t(
		pdat->system__thresh_low,
		2,
		pbuffer +   3);
	*(pbuffer +   5) =
		pdat->system__enable_xtalk_per_quadrant & 0x1;
	*(pbuffer +   6) =
		pdat->system__seed_config & 0x7;
	*(pbuffer +   7) =
		pdat->sd_config__woi_sd0;
	*(pbuffer +   8) =
		pdat->sd_config__woi_sd1;
	*(pbuffer +   9) =
		pdat->sd_config__initial_phase_sd0 & 0x7F;
	*(pbuffer +  10) =
		pdat->sd_config__initial_phase_sd1 & 0x7F;
	*(pbuffer +  11) =
		pdat->system__grouped_parameter_hold_1 & 0x3;
	*(pbuffer +  12) =
		pdat->sd_config__first_order_select & 0x3;
	*(pbuffer +  13) =
		pdat->sd_config__quantifier & 0xF;
	*(pbuffer +  14) =
		pdat->roi_config__user_roi_centre_spad;
	*(pbuffer +  15) =
		pdat->roi_config__user_roi_requested_global_xy_size;
	*(pbuffer +  16) =
		pdat->system__sequence_config;
	*(pbuffer +  17) =
		pdat->system__grouped_parameter_hold & 0x3;
	LOG_FUNCTION_END(status);


	return status;
}


VL53L1_Error VL53L1_i2c_decode_dynamic_config(
	uint16_t                   buf_size,
	uint8_t                   *pbuffer,
	VL53L1_dynamic_config_t   *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (buf_size < VL53L1_DYNAMIC_CONFIG_I2C_SIZE_BYTES)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	pdat->system__grouped_parameter_hold_0 =
		(*(pbuffer +   0)) & 0x3;
	pdat->system__thresh_high =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +   1));
	pdat->system__thresh_low =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +   3));
	pdat->system__enable_xtalk_per_quadrant =
		(*(pbuffer +   5)) & 0x1;
	pdat->system__seed_config =
		(*(pbuffer +   6)) & 0x7;
	pdat->sd_config__woi_sd0 =
		(*(pbuffer +   7));
	pdat->sd_config__woi_sd1 =
		(*(pbuffer +   8));
	pdat->sd_config__initial_phase_sd0 =
		(*(pbuffer +   9)) & 0x7F;
	pdat->sd_config__initial_phase_sd1 =
		(*(pbuffer +  10)) & 0x7F;
	pdat->system__grouped_parameter_hold_1 =
		(*(pbuffer +  11)) & 0x3;
	pdat->sd_config__first_order_select =
		(*(pbuffer +  12)) & 0x3;
	pdat->sd_config__quantifier =
		(*(pbuffer +  13)) & 0xF;
	pdat->roi_config__user_roi_centre_spad =
		(*(pbuffer +  14));
	pdat->roi_config__user_roi_requested_global_xy_size =
		(*(pbuffer +  15));
	pdat->system__sequence_config =
		(*(pbuffer +  16));
	pdat->system__grouped_parameter_hold =
		(*(pbuffer +  17)) & 0x3;

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_set_dynamic_config(
	VL53L1_DEV                 Dev,
	VL53L1_dynamic_config_t   *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_DYNAMIC_CONFIG_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_encode_dynamic_config(
			pdat,
			VL53L1_DYNAMIC_CONFIG_I2C_SIZE_BYTES,
			comms_buffer);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_WriteMulti(
			Dev,
			VL53L1_SYSTEM__GROUPED_PARAMETER_HOLD_0,
			comms_buffer,
			VL53L1_DYNAMIC_CONFIG_I2C_SIZE_BYTES);

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_get_dynamic_config(
	VL53L1_DEV                 Dev,
	VL53L1_dynamic_config_t   *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_DYNAMIC_CONFIG_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_ReadMulti(
			Dev,
			VL53L1_SYSTEM__GROUPED_PARAMETER_HOLD_0,
			comms_buffer,
			VL53L1_DYNAMIC_CONFIG_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_decode_dynamic_config(
			VL53L1_DYNAMIC_CONFIG_I2C_SIZE_BYTES,
			comms_buffer,
			pdat);

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_i2c_encode_system_control(
	VL53L1_system_control_t  *pdat,
	uint16_t                  buf_size,
	uint8_t                  *pbuffer)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (buf_size < VL53L1_SYSTEM_CONTROL_I2C_SIZE_BYTES)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	*(pbuffer +   0) =
		pdat->power_management__go1_power_force & 0x1;
	*(pbuffer +   1) =
		pdat->system__stream_count_ctrl & 0x1;
	*(pbuffer +   2) =
		pdat->firmware__enable & 0x1;
	*(pbuffer +   3) =
		pdat->system__interrupt_clear & 0x3;
	*(pbuffer +   4) =
		pdat->system__mode_start;
	LOG_FUNCTION_END(status);


	return status;
}


VL53L1_Error VL53L1_i2c_decode_system_control(
	uint16_t                   buf_size,
	uint8_t                   *pbuffer,
	VL53L1_system_control_t   *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (buf_size < VL53L1_SYSTEM_CONTROL_I2C_SIZE_BYTES)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	pdat->power_management__go1_power_force =
		(*(pbuffer +   0)) & 0x1;
	pdat->system__stream_count_ctrl =
		(*(pbuffer +   1)) & 0x1;
	pdat->firmware__enable =
		(*(pbuffer +   2)) & 0x1;
	pdat->system__interrupt_clear =
		(*(pbuffer +   3)) & 0x3;
	pdat->system__mode_start =
		(*(pbuffer +   4));

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_set_system_control(
	VL53L1_DEV                 Dev,
	VL53L1_system_control_t   *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_SYSTEM_CONTROL_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_encode_system_control(
			pdat,
			VL53L1_SYSTEM_CONTROL_I2C_SIZE_BYTES,
			comms_buffer);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_WriteMulti(
			Dev,
			VL53L1_POWER_MANAGEMENT__GO1_POWER_FORCE,
			comms_buffer,
			VL53L1_SYSTEM_CONTROL_I2C_SIZE_BYTES);

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_get_system_control(
	VL53L1_DEV                 Dev,
	VL53L1_system_control_t   *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_SYSTEM_CONTROL_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_ReadMulti(
			Dev,
			VL53L1_POWER_MANAGEMENT__GO1_POWER_FORCE,
			comms_buffer,
			VL53L1_SYSTEM_CONTROL_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_decode_system_control(
			VL53L1_SYSTEM_CONTROL_I2C_SIZE_BYTES,
			comms_buffer,
			pdat);

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_i2c_encode_system_results(
	VL53L1_system_results_t  *pdat,
	uint16_t                  buf_size,
	uint8_t                  *pbuffer)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (buf_size < VL53L1_SYSTEM_RESULTS_I2C_SIZE_BYTES)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	*(pbuffer +   0) =
		pdat->result__interrupt_status & 0x3F;
	*(pbuffer +   1) =
		pdat->result__range_status;
	*(pbuffer +   2) =
		pdat->result__report_status & 0xF;
	*(pbuffer +   3) =
		pdat->result__stream_count;
	VL53L1_i2c_encode_uint16_t(
		pdat->result__dss_actual_effective_spads_sd0,
		2,
		pbuffer +   4);
	VL53L1_i2c_encode_uint16_t(
		pdat->result__peak_signal_count_rate_mcps_sd0,
		2,
		pbuffer +   6);
	VL53L1_i2c_encode_uint16_t(
		pdat->result__ambient_count_rate_mcps_sd0,
		2,
		pbuffer +   8);
	VL53L1_i2c_encode_uint16_t(
		pdat->result__sigma_sd0,
		2,
		pbuffer +  10);
	VL53L1_i2c_encode_uint16_t(
		pdat->result__phase_sd0,
		2,
		pbuffer +  12);
	VL53L1_i2c_encode_uint16_t(
		pdat->result__final_crosstalk_corrected_range_mm_sd0,
		2,
		pbuffer +  14);
	VL53L1_i2c_encode_uint16_t(
	pdat->result__peak_signal_count_rate_crosstalk_corrected_mcps_sd0,
	2,
	pbuffer +  16);
	VL53L1_i2c_encode_uint16_t(
		pdat->result__mm_inner_actual_effective_spads_sd0,
		2,
		pbuffer +  18);
	VL53L1_i2c_encode_uint16_t(
		pdat->result__mm_outer_actual_effective_spads_sd0,
		2,
		pbuffer +  20);
	VL53L1_i2c_encode_uint16_t(
		pdat->result__avg_signal_count_rate_mcps_sd0,
		2,
		pbuffer +  22);
	VL53L1_i2c_encode_uint16_t(
		pdat->result__dss_actual_effective_spads_sd1,
		2,
		pbuffer +  24);
	VL53L1_i2c_encode_uint16_t(
		pdat->result__peak_signal_count_rate_mcps_sd1,
		2,
		pbuffer +  26);
	VL53L1_i2c_encode_uint16_t(
		pdat->result__ambient_count_rate_mcps_sd1,
		2,
		pbuffer +  28);
	VL53L1_i2c_encode_uint16_t(
		pdat->result__sigma_sd1,
		2,
		pbuffer +  30);
	VL53L1_i2c_encode_uint16_t(
		pdat->result__phase_sd1,
		2,
		pbuffer +  32);
	VL53L1_i2c_encode_uint16_t(
		pdat->result__final_crosstalk_corrected_range_mm_sd1,
		2,
		pbuffer +  34);
	VL53L1_i2c_encode_uint16_t(
		pdat->result__spare_0_sd1,
		2,
		pbuffer +  36);
	VL53L1_i2c_encode_uint16_t(
		pdat->result__spare_1_sd1,
		2,
		pbuffer +  38);
	VL53L1_i2c_encode_uint16_t(
		pdat->result__spare_2_sd1,
		2,
		pbuffer +  40);
	*(pbuffer +  42) =
		pdat->result__spare_3_sd1;
	*(pbuffer +  43) =
		pdat->result__thresh_info;
	LOG_FUNCTION_END(status);


	return status;
}


VL53L1_Error VL53L1_i2c_decode_system_results(
	uint16_t                   buf_size,
	uint8_t                   *pbuffer,
	VL53L1_system_results_t   *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (buf_size < VL53L1_SYSTEM_RESULTS_I2C_SIZE_BYTES)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	pdat->result__interrupt_status =
		(*(pbuffer +   0)) & 0x3F;
	pdat->result__range_status =
		(*(pbuffer +   1));
	pdat->result__report_status =
		(*(pbuffer +   2)) & 0xF;
	pdat->result__stream_count =
		(*(pbuffer +   3));
	pdat->result__dss_actual_effective_spads_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +   4));
	pdat->result__peak_signal_count_rate_mcps_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +   6));
	pdat->result__ambient_count_rate_mcps_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +   8));
	pdat->result__sigma_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  10));
	pdat->result__phase_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  12));
	pdat->result__final_crosstalk_corrected_range_mm_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  14));
	pdat->result__peak_signal_count_rate_crosstalk_corrected_mcps_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  16));
	pdat->result__mm_inner_actual_effective_spads_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  18));
	pdat->result__mm_outer_actual_effective_spads_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  20));
	pdat->result__avg_signal_count_rate_mcps_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  22));
	pdat->result__dss_actual_effective_spads_sd1 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  24));
	pdat->result__peak_signal_count_rate_mcps_sd1 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  26));
	pdat->result__ambient_count_rate_mcps_sd1 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  28));
	pdat->result__sigma_sd1 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  30));
	pdat->result__phase_sd1 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  32));
	pdat->result__final_crosstalk_corrected_range_mm_sd1 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  34));
	pdat->result__spare_0_sd1 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  36));
	pdat->result__spare_1_sd1 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  38));
	pdat->result__spare_2_sd1 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  40));
	pdat->result__spare_3_sd1 =
		(*(pbuffer +  42));
	pdat->result__thresh_info =
		(*(pbuffer +  43));

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_set_system_results(
	VL53L1_DEV                 Dev,
	VL53L1_system_results_t   *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_SYSTEM_RESULTS_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_encode_system_results(
			pdat,
			VL53L1_SYSTEM_RESULTS_I2C_SIZE_BYTES,
			comms_buffer);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_WriteMulti(
			Dev,
			VL53L1_RESULT__INTERRUPT_STATUS,
			comms_buffer,
			VL53L1_SYSTEM_RESULTS_I2C_SIZE_BYTES);

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_get_system_results(
	VL53L1_DEV                 Dev,
	VL53L1_system_results_t   *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_SYSTEM_RESULTS_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_ReadMulti(
			Dev,
			VL53L1_RESULT__INTERRUPT_STATUS,
			comms_buffer,
			VL53L1_SYSTEM_RESULTS_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_decode_system_results(
			VL53L1_SYSTEM_RESULTS_I2C_SIZE_BYTES,
			comms_buffer,
			pdat);

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_i2c_encode_core_results(
	VL53L1_core_results_t    *pdat,
	uint16_t                  buf_size,
	uint8_t                  *pbuffer)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (buf_size < VL53L1_CORE_RESULTS_I2C_SIZE_BYTES)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	VL53L1_i2c_encode_uint32_t(
		pdat->result_core__ambient_window_events_sd0,
		4,
		pbuffer +   0);
	VL53L1_i2c_encode_uint32_t(
		pdat->result_core__ranging_total_events_sd0,
		4,
		pbuffer +   4);
	VL53L1_i2c_encode_int32_t(
		pdat->result_core__signal_total_events_sd0,
		4,
		pbuffer +   8);
	VL53L1_i2c_encode_uint32_t(
		pdat->result_core__total_periods_elapsed_sd0,
		4,
		pbuffer +  12);
	VL53L1_i2c_encode_uint32_t(
		pdat->result_core__ambient_window_events_sd1,
		4,
		pbuffer +  16);
	VL53L1_i2c_encode_uint32_t(
		pdat->result_core__ranging_total_events_sd1,
		4,
		pbuffer +  20);
	VL53L1_i2c_encode_int32_t(
		pdat->result_core__signal_total_events_sd1,
		4,
		pbuffer +  24);
	VL53L1_i2c_encode_uint32_t(
		pdat->result_core__total_periods_elapsed_sd1,
		4,
		pbuffer +  28);
	*(pbuffer +  32) =
		pdat->result_core__spare_0;
	LOG_FUNCTION_END(status);


	return status;
}


VL53L1_Error VL53L1_i2c_decode_core_results(
	uint16_t                   buf_size,
	uint8_t                   *pbuffer,
	VL53L1_core_results_t     *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (buf_size < VL53L1_CORE_RESULTS_I2C_SIZE_BYTES)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	pdat->result_core__ambient_window_events_sd0 =
		(VL53L1_i2c_decode_uint32_t(4, pbuffer +   0));
	pdat->result_core__ranging_total_events_sd0 =
		(VL53L1_i2c_decode_uint32_t(4, pbuffer +   4));
	pdat->result_core__signal_total_events_sd0 =
		(VL53L1_i2c_decode_int32_t(4, pbuffer +   8));
	pdat->result_core__total_periods_elapsed_sd0 =
		(VL53L1_i2c_decode_uint32_t(4, pbuffer +  12));
	pdat->result_core__ambient_window_events_sd1 =
		(VL53L1_i2c_decode_uint32_t(4, pbuffer +  16));
	pdat->result_core__ranging_total_events_sd1 =
		(VL53L1_i2c_decode_uint32_t(4, pbuffer +  20));
	pdat->result_core__signal_total_events_sd1 =
		(VL53L1_i2c_decode_int32_t(4, pbuffer +  24));
	pdat->result_core__total_periods_elapsed_sd1 =
		(VL53L1_i2c_decode_uint32_t(4, pbuffer +  28));
	pdat->result_core__spare_0 =
		(*(pbuffer +  32));

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_set_core_results(
	VL53L1_DEV                 Dev,
	VL53L1_core_results_t     *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_CORE_RESULTS_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_encode_core_results(
			pdat,
			VL53L1_CORE_RESULTS_I2C_SIZE_BYTES,
			comms_buffer);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_disable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_WriteMulti(
			Dev,
			VL53L1_RESULT_CORE__AMBIENT_WINDOW_EVENTS_SD0,
			comms_buffer,
			VL53L1_CORE_RESULTS_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_enable_firmware(Dev);

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_get_core_results(
	VL53L1_DEV                 Dev,
	VL53L1_core_results_t     *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_CORE_RESULTS_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_ReadMulti(
			Dev,
			VL53L1_RESULT_CORE__AMBIENT_WINDOW_EVENTS_SD0,
			comms_buffer,
			VL53L1_CORE_RESULTS_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_decode_core_results(
			VL53L1_CORE_RESULTS_I2C_SIZE_BYTES,
			comms_buffer,
			pdat);

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_i2c_encode_debug_results(
	VL53L1_debug_results_t   *pdat,
	uint16_t                  buf_size,
	uint8_t                  *pbuffer)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (buf_size < VL53L1_DEBUG_RESULTS_I2C_SIZE_BYTES)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	VL53L1_i2c_encode_uint16_t(
		pdat->phasecal_result__reference_phase,
		2,
		pbuffer +   0);
	*(pbuffer +   2) =
		pdat->phasecal_result__vcsel_start & 0x7F;
	*(pbuffer +   3) =
		pdat->ref_spad_char_result__num_actual_ref_spads & 0x3F;
	*(pbuffer +   4) =
		pdat->ref_spad_char_result__ref_location & 0x3;
	*(pbuffer +   5) =
		pdat->vhv_result__coldboot_status & 0x1;
	*(pbuffer +   6) =
		pdat->vhv_result__search_result & 0x3F;
	*(pbuffer +   7) =
		pdat->vhv_result__latest_setting & 0x3F;
	VL53L1_i2c_encode_uint16_t(
		pdat->result__osc_calibrate_val & 0x3FF,
		2,
		pbuffer +   8);
	*(pbuffer +  10) =
		pdat->ana_config__powerdown_go1 & 0x3;
	*(pbuffer +  11) =
		pdat->ana_config__ref_bg_ctrl & 0x3;
	*(pbuffer +  12) =
		pdat->ana_config__regdvdd1v2_ctrl & 0xF;
	*(pbuffer +  13) =
		pdat->ana_config__osc_slow_ctrl & 0x7;
	*(pbuffer +  14) =
		pdat->test_mode__status & 0x1;
	*(pbuffer +  15) =
		pdat->firmware__system_status & 0x3;
	*(pbuffer +  16) =
		pdat->firmware__mode_status;
	*(pbuffer +  17) =
		pdat->firmware__secondary_mode_status;
	VL53L1_i2c_encode_uint16_t(
		pdat->firmware__cal_repeat_rate_counter & 0xFFF,
		2,
		pbuffer +  18);
	VL53L1_i2c_encode_uint16_t(
		pdat->gph__system__thresh_high,
		2,
		pbuffer +  22);
	VL53L1_i2c_encode_uint16_t(
		pdat->gph__system__thresh_low,
		2,
		pbuffer +  24);
	*(pbuffer +  26) =
		pdat->gph__system__enable_xtalk_per_quadrant & 0x1;
	*(pbuffer +  27) =
		pdat->gph__spare_0 & 0x7;
	*(pbuffer +  28) =
		pdat->gph__sd_config__woi_sd0;
	*(pbuffer +  29) =
		pdat->gph__sd_config__woi_sd1;
	*(pbuffer +  30) =
		pdat->gph__sd_config__initial_phase_sd0 & 0x7F;
	*(pbuffer +  31) =
		pdat->gph__sd_config__initial_phase_sd1 & 0x7F;
	*(pbuffer +  32) =
		pdat->gph__sd_config__first_order_select & 0x3;
	*(pbuffer +  33) =
		pdat->gph__sd_config__quantifier & 0xF;
	*(pbuffer +  34) =
		pdat->gph__roi_config__user_roi_centre_spad;
	*(pbuffer +  35) =
		pdat->gph__roi_config__user_roi_requested_global_xy_size;
	*(pbuffer +  36) =
		pdat->gph__system__sequence_config;
	*(pbuffer +  37) =
		pdat->gph__gph_id & 0x1;
	*(pbuffer +  38) =
		pdat->system__interrupt_set & 0x3;
	*(pbuffer +  39) =
		pdat->interrupt_manager__enables & 0x1F;
	*(pbuffer +  40) =
		pdat->interrupt_manager__clear & 0x1F;
	*(pbuffer +  41) =
		pdat->interrupt_manager__status & 0x1F;
	*(pbuffer +  42) =
		pdat->mcu_to_host_bank__wr_access_en & 0x1;
	*(pbuffer +  43) =
		pdat->power_management__go1_reset_status & 0x1;
	*(pbuffer +  44) =
		pdat->pad_startup_mode__value_ro & 0x3;
	*(pbuffer +  45) =
		pdat->pad_startup_mode__value_ctrl & 0x3F;
	VL53L1_i2c_encode_uint32_t(
		pdat->pll_period_us & 0x3FFFF,
		4,
		pbuffer +  46);
	VL53L1_i2c_encode_uint32_t(
		pdat->interrupt_scheduler__data_out,
		4,
		pbuffer +  50);
	*(pbuffer +  54) =
		pdat->nvm_bist__complete & 0x1;
	*(pbuffer +  55) =
		pdat->nvm_bist__status & 0x1;
	LOG_FUNCTION_END(status);


	return status;
}


VL53L1_Error VL53L1_i2c_decode_debug_results(
	uint16_t                   buf_size,
	uint8_t                   *pbuffer,
	VL53L1_debug_results_t    *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (buf_size < VL53L1_DEBUG_RESULTS_I2C_SIZE_BYTES)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	pdat->phasecal_result__reference_phase =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +   0));
	pdat->phasecal_result__vcsel_start =
		(*(pbuffer +   2)) & 0x7F;
	pdat->ref_spad_char_result__num_actual_ref_spads =
		(*(pbuffer +   3)) & 0x3F;
	pdat->ref_spad_char_result__ref_location =
		(*(pbuffer +   4)) & 0x3;
	pdat->vhv_result__coldboot_status =
		(*(pbuffer +   5)) & 0x1;
	pdat->vhv_result__search_result =
		(*(pbuffer +   6)) & 0x3F;
	pdat->vhv_result__latest_setting =
		(*(pbuffer +   7)) & 0x3F;
	pdat->result__osc_calibrate_val =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +   8)) & 0x3FF;
	pdat->ana_config__powerdown_go1 =
		(*(pbuffer +  10)) & 0x3;
	pdat->ana_config__ref_bg_ctrl =
		(*(pbuffer +  11)) & 0x3;
	pdat->ana_config__regdvdd1v2_ctrl =
		(*(pbuffer +  12)) & 0xF;
	pdat->ana_config__osc_slow_ctrl =
		(*(pbuffer +  13)) & 0x7;
	pdat->test_mode__status =
		(*(pbuffer +  14)) & 0x1;
	pdat->firmware__system_status =
		(*(pbuffer +  15)) & 0x3;
	pdat->firmware__mode_status =
		(*(pbuffer +  16));
	pdat->firmware__secondary_mode_status =
		(*(pbuffer +  17));
	pdat->firmware__cal_repeat_rate_counter =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  18)) & 0xFFF;
	pdat->gph__system__thresh_high =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  22));
	pdat->gph__system__thresh_low =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  24));
	pdat->gph__system__enable_xtalk_per_quadrant =
		(*(pbuffer +  26)) & 0x1;
	pdat->gph__spare_0 =
		(*(pbuffer +  27)) & 0x7;
	pdat->gph__sd_config__woi_sd0 =
		(*(pbuffer +  28));
	pdat->gph__sd_config__woi_sd1 =
		(*(pbuffer +  29));
	pdat->gph__sd_config__initial_phase_sd0 =
		(*(pbuffer +  30)) & 0x7F;
	pdat->gph__sd_config__initial_phase_sd1 =
		(*(pbuffer +  31)) & 0x7F;
	pdat->gph__sd_config__first_order_select =
		(*(pbuffer +  32)) & 0x3;
	pdat->gph__sd_config__quantifier =
		(*(pbuffer +  33)) & 0xF;
	pdat->gph__roi_config__user_roi_centre_spad =
		(*(pbuffer +  34));
	pdat->gph__roi_config__user_roi_requested_global_xy_size =
		(*(pbuffer +  35));
	pdat->gph__system__sequence_config =
		(*(pbuffer +  36));
	pdat->gph__gph_id =
		(*(pbuffer +  37)) & 0x1;
	pdat->system__interrupt_set =
		(*(pbuffer +  38)) & 0x3;
	pdat->interrupt_manager__enables =
		(*(pbuffer +  39)) & 0x1F;
	pdat->interrupt_manager__clear =
		(*(pbuffer +  40)) & 0x1F;
	pdat->interrupt_manager__status =
		(*(pbuffer +  41)) & 0x1F;
	pdat->mcu_to_host_bank__wr_access_en =
		(*(pbuffer +  42)) & 0x1;
	pdat->power_management__go1_reset_status =
		(*(pbuffer +  43)) & 0x1;
	pdat->pad_startup_mode__value_ro =
		(*(pbuffer +  44)) & 0x3;
	pdat->pad_startup_mode__value_ctrl =
		(*(pbuffer +  45)) & 0x3F;
	pdat->pll_period_us =
		(VL53L1_i2c_decode_uint32_t(4, pbuffer +  46)) & 0x3FFFF;
	pdat->interrupt_scheduler__data_out =
		(VL53L1_i2c_decode_uint32_t(4, pbuffer +  50));
	pdat->nvm_bist__complete =
		(*(pbuffer +  54)) & 0x1;
	pdat->nvm_bist__status =
		(*(pbuffer +  55)) & 0x1;

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_set_debug_results(
	VL53L1_DEV                 Dev,
	VL53L1_debug_results_t    *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_DEBUG_RESULTS_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_encode_debug_results(
			pdat,
			VL53L1_DEBUG_RESULTS_I2C_SIZE_BYTES,
			comms_buffer);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_disable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_WriteMulti(
			Dev,
			VL53L1_PHASECAL_RESULT__REFERENCE_PHASE,
			comms_buffer,
			VL53L1_DEBUG_RESULTS_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_enable_firmware(Dev);

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_get_debug_results(
	VL53L1_DEV                 Dev,
	VL53L1_debug_results_t    *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_DEBUG_RESULTS_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_ReadMulti(
			Dev,
			VL53L1_PHASECAL_RESULT__REFERENCE_PHASE,
			comms_buffer,
			VL53L1_DEBUG_RESULTS_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_decode_debug_results(
			VL53L1_DEBUG_RESULTS_I2C_SIZE_BYTES,
			comms_buffer,
			pdat);

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_i2c_encode_nvm_copy_data(
	VL53L1_nvm_copy_data_t   *pdat,
	uint16_t                  buf_size,
	uint8_t                  *pbuffer)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (buf_size < VL53L1_NVM_COPY_DATA_I2C_SIZE_BYTES)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	*(pbuffer +   0) =
		pdat->identification__model_id;
	*(pbuffer +   1) =
		pdat->identification__module_type;
	*(pbuffer +   2) =
		pdat->identification__revision_id;
	VL53L1_i2c_encode_uint16_t(
		pdat->identification__module_id,
		2,
		pbuffer +   3);
	*(pbuffer +   5) =
		pdat->ana_config__fast_osc__trim_max & 0x7F;
	*(pbuffer +   6) =
		pdat->ana_config__fast_osc__freq_set & 0x7;
	*(pbuffer +   7) =
		pdat->ana_config__vcsel_trim & 0x7;
	*(pbuffer +   8) =
		pdat->ana_config__vcsel_selion & 0x3F;
	*(pbuffer +   9) =
		pdat->ana_config__vcsel_selion_max & 0x3F;
	*(pbuffer +  10) =
		pdat->protected_laser_safety__lock_bit & 0x1;
	*(pbuffer +  11) =
		pdat->laser_safety__key & 0x7F;
	*(pbuffer +  12) =
		pdat->laser_safety__key_ro & 0x1;
	*(pbuffer +  13) =
		pdat->laser_safety__clip & 0x3F;
	*(pbuffer +  14) =
		pdat->laser_safety__mult & 0x3F;
	*(pbuffer +  15) =
		pdat->global_config__spad_enables_rtn_0;
	*(pbuffer +  16) =
		pdat->global_config__spad_enables_rtn_1;
	*(pbuffer +  17) =
		pdat->global_config__spad_enables_rtn_2;
	*(pbuffer +  18) =
		pdat->global_config__spad_enables_rtn_3;
	*(pbuffer +  19) =
		pdat->global_config__spad_enables_rtn_4;
	*(pbuffer +  20) =
		pdat->global_config__spad_enables_rtn_5;
	*(pbuffer +  21) =
		pdat->global_config__spad_enables_rtn_6;
	*(pbuffer +  22) =
		pdat->global_config__spad_enables_rtn_7;
	*(pbuffer +  23) =
		pdat->global_config__spad_enables_rtn_8;
	*(pbuffer +  24) =
		pdat->global_config__spad_enables_rtn_9;
	*(pbuffer +  25) =
		pdat->global_config__spad_enables_rtn_10;
	*(pbuffer +  26) =
		pdat->global_config__spad_enables_rtn_11;
	*(pbuffer +  27) =
		pdat->global_config__spad_enables_rtn_12;
	*(pbuffer +  28) =
		pdat->global_config__spad_enables_rtn_13;
	*(pbuffer +  29) =
		pdat->global_config__spad_enables_rtn_14;
	*(pbuffer +  30) =
		pdat->global_config__spad_enables_rtn_15;
	*(pbuffer +  31) =
		pdat->global_config__spad_enables_rtn_16;
	*(pbuffer +  32) =
		pdat->global_config__spad_enables_rtn_17;
	*(pbuffer +  33) =
		pdat->global_config__spad_enables_rtn_18;
	*(pbuffer +  34) =
		pdat->global_config__spad_enables_rtn_19;
	*(pbuffer +  35) =
		pdat->global_config__spad_enables_rtn_20;
	*(pbuffer +  36) =
		pdat->global_config__spad_enables_rtn_21;
	*(pbuffer +  37) =
		pdat->global_config__spad_enables_rtn_22;
	*(pbuffer +  38) =
		pdat->global_config__spad_enables_rtn_23;
	*(pbuffer +  39) =
		pdat->global_config__spad_enables_rtn_24;
	*(pbuffer +  40) =
		pdat->global_config__spad_enables_rtn_25;
	*(pbuffer +  41) =
		pdat->global_config__spad_enables_rtn_26;
	*(pbuffer +  42) =
		pdat->global_config__spad_enables_rtn_27;
	*(pbuffer +  43) =
		pdat->global_config__spad_enables_rtn_28;
	*(pbuffer +  44) =
		pdat->global_config__spad_enables_rtn_29;
	*(pbuffer +  45) =
		pdat->global_config__spad_enables_rtn_30;
	*(pbuffer +  46) =
		pdat->global_config__spad_enables_rtn_31;
	*(pbuffer +  47) =
		pdat->roi_config__mode_roi_centre_spad;
	*(pbuffer +  48) =
		pdat->roi_config__mode_roi_xy_size;
	LOG_FUNCTION_END(status);


	return status;
}


VL53L1_Error VL53L1_i2c_decode_nvm_copy_data(
	uint16_t                   buf_size,
	uint8_t                   *pbuffer,
	VL53L1_nvm_copy_data_t    *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (buf_size < VL53L1_NVM_COPY_DATA_I2C_SIZE_BYTES)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	pdat->identification__model_id =
		(*(pbuffer +   0));
	pdat->identification__module_type =
		(*(pbuffer +   1));
	pdat->identification__revision_id =
		(*(pbuffer +   2));
	pdat->identification__module_id =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +   3));
	pdat->ana_config__fast_osc__trim_max =
		(*(pbuffer +   5)) & 0x7F;
	pdat->ana_config__fast_osc__freq_set =
		(*(pbuffer +   6)) & 0x7;
	pdat->ana_config__vcsel_trim =
		(*(pbuffer +   7)) & 0x7;
	pdat->ana_config__vcsel_selion =
		(*(pbuffer +   8)) & 0x3F;
	pdat->ana_config__vcsel_selion_max =
		(*(pbuffer +   9)) & 0x3F;
	pdat->protected_laser_safety__lock_bit =
		(*(pbuffer +  10)) & 0x1;
	pdat->laser_safety__key =
		(*(pbuffer +  11)) & 0x7F;
	pdat->laser_safety__key_ro =
		(*(pbuffer +  12)) & 0x1;
	pdat->laser_safety__clip =
		(*(pbuffer +  13)) & 0x3F;
	pdat->laser_safety__mult =
		(*(pbuffer +  14)) & 0x3F;
	pdat->global_config__spad_enables_rtn_0 =
		(*(pbuffer +  15));
	pdat->global_config__spad_enables_rtn_1 =
		(*(pbuffer +  16));
	pdat->global_config__spad_enables_rtn_2 =
		(*(pbuffer +  17));
	pdat->global_config__spad_enables_rtn_3 =
		(*(pbuffer +  18));
	pdat->global_config__spad_enables_rtn_4 =
		(*(pbuffer +  19));
	pdat->global_config__spad_enables_rtn_5 =
		(*(pbuffer +  20));
	pdat->global_config__spad_enables_rtn_6 =
		(*(pbuffer +  21));
	pdat->global_config__spad_enables_rtn_7 =
		(*(pbuffer +  22));
	pdat->global_config__spad_enables_rtn_8 =
		(*(pbuffer +  23));
	pdat->global_config__spad_enables_rtn_9 =
		(*(pbuffer +  24));
	pdat->global_config__spad_enables_rtn_10 =
		(*(pbuffer +  25));
	pdat->global_config__spad_enables_rtn_11 =
		(*(pbuffer +  26));
	pdat->global_config__spad_enables_rtn_12 =
		(*(pbuffer +  27));
	pdat->global_config__spad_enables_rtn_13 =
		(*(pbuffer +  28));
	pdat->global_config__spad_enables_rtn_14 =
		(*(pbuffer +  29));
	pdat->global_config__spad_enables_rtn_15 =
		(*(pbuffer +  30));
	pdat->global_config__spad_enables_rtn_16 =
		(*(pbuffer +  31));
	pdat->global_config__spad_enables_rtn_17 =
		(*(pbuffer +  32));
	pdat->global_config__spad_enables_rtn_18 =
		(*(pbuffer +  33));
	pdat->global_config__spad_enables_rtn_19 =
		(*(pbuffer +  34));
	pdat->global_config__spad_enables_rtn_20 =
		(*(pbuffer +  35));
	pdat->global_config__spad_enables_rtn_21 =
		(*(pbuffer +  36));
	pdat->global_config__spad_enables_rtn_22 =
		(*(pbuffer +  37));
	pdat->global_config__spad_enables_rtn_23 =
		(*(pbuffer +  38));
	pdat->global_config__spad_enables_rtn_24 =
		(*(pbuffer +  39));
	pdat->global_config__spad_enables_rtn_25 =
		(*(pbuffer +  40));
	pdat->global_config__spad_enables_rtn_26 =
		(*(pbuffer +  41));
	pdat->global_config__spad_enables_rtn_27 =
		(*(pbuffer +  42));
	pdat->global_config__spad_enables_rtn_28 =
		(*(pbuffer +  43));
	pdat->global_config__spad_enables_rtn_29 =
		(*(pbuffer +  44));
	pdat->global_config__spad_enables_rtn_30 =
		(*(pbuffer +  45));
	pdat->global_config__spad_enables_rtn_31 =
		(*(pbuffer +  46));
	pdat->roi_config__mode_roi_centre_spad =
		(*(pbuffer +  47));
	pdat->roi_config__mode_roi_xy_size =
		(*(pbuffer +  48));

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_set_nvm_copy_data(
	VL53L1_DEV                 Dev,
	VL53L1_nvm_copy_data_t    *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_NVM_COPY_DATA_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_encode_nvm_copy_data(
			pdat,
			VL53L1_NVM_COPY_DATA_I2C_SIZE_BYTES,
			comms_buffer);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_disable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_WriteMulti(
			Dev,
			VL53L1_IDENTIFICATION__MODEL_ID,
			comms_buffer,
			VL53L1_NVM_COPY_DATA_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_enable_firmware(Dev);

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_get_nvm_copy_data(
	VL53L1_DEV                 Dev,
	VL53L1_nvm_copy_data_t    *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_NVM_COPY_DATA_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_ReadMulti(
			Dev,
			VL53L1_IDENTIFICATION__MODEL_ID,
			comms_buffer,
			VL53L1_NVM_COPY_DATA_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_decode_nvm_copy_data(
			VL53L1_NVM_COPY_DATA_I2C_SIZE_BYTES,
			comms_buffer,
			pdat);

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_i2c_encode_prev_shadow_system_results(
	VL53L1_prev_shadow_system_results_t *pdat,
	uint16_t                  buf_size,
	uint8_t                  *pbuffer)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (buf_size < VL53L1_PREV_SHADOW_SYSTEM_RESULTS_I2C_SIZE_BYTES)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	*(pbuffer +   0) =
		pdat->prev_shadow_result__interrupt_status & 0x3F;
	*(pbuffer +   1) =
		pdat->prev_shadow_result__range_status;
	*(pbuffer +   2) =
		pdat->prev_shadow_result__report_status & 0xF;
	*(pbuffer +   3) =
		pdat->prev_shadow_result__stream_count;
	VL53L1_i2c_encode_uint16_t(
		pdat->prev_shadow_result__dss_actual_effective_spads_sd0,
		2,
		pbuffer +   4);
	VL53L1_i2c_encode_uint16_t(
		pdat->prev_shadow_result__peak_signal_count_rate_mcps_sd0,
		2,
		pbuffer +   6);
	VL53L1_i2c_encode_uint16_t(
		pdat->prev_shadow_result__ambient_count_rate_mcps_sd0,
		2,
		pbuffer +   8);
	VL53L1_i2c_encode_uint16_t(
		pdat->prev_shadow_result__sigma_sd0,
		2,
		pbuffer +  10);
	VL53L1_i2c_encode_uint16_t(
		pdat->prev_shadow_result__phase_sd0,
		2,
		pbuffer +  12);
	VL53L1_i2c_encode_uint16_t(
	pdat->prev_shadow_result__final_crosstalk_corrected_range_mm_sd0,
	2,
	pbuffer +  14);
	VL53L1_i2c_encode_uint16_t(
	pdat->psr__peak_signal_count_rate_crosstalk_corrected_mcps_sd0,
	2,
	pbuffer +  16);
	VL53L1_i2c_encode_uint16_t(
		pdat->prev_shadow_result__mm_inner_actual_effective_spads_sd0,
		2,
		pbuffer +  18);
	VL53L1_i2c_encode_uint16_t(
		pdat->prev_shadow_result__mm_outer_actual_effective_spads_sd0,
		2,
		pbuffer +  20);
	VL53L1_i2c_encode_uint16_t(
		pdat->prev_shadow_result__avg_signal_count_rate_mcps_sd0,
		2,
		pbuffer +  22);
	VL53L1_i2c_encode_uint16_t(
		pdat->prev_shadow_result__dss_actual_effective_spads_sd1,
		2,
		pbuffer +  24);
	VL53L1_i2c_encode_uint16_t(
		pdat->prev_shadow_result__peak_signal_count_rate_mcps_sd1,
		2,
		pbuffer +  26);
	VL53L1_i2c_encode_uint16_t(
		pdat->prev_shadow_result__ambient_count_rate_mcps_sd1,
		2,
		pbuffer +  28);
	VL53L1_i2c_encode_uint16_t(
		pdat->prev_shadow_result__sigma_sd1,
		2,
		pbuffer +  30);
	VL53L1_i2c_encode_uint16_t(
		pdat->prev_shadow_result__phase_sd1,
		2,
		pbuffer +  32);
	VL53L1_i2c_encode_uint16_t(
	pdat->prev_shadow_result__final_crosstalk_corrected_range_mm_sd1,
	2,
	pbuffer +  34);
	VL53L1_i2c_encode_uint16_t(
		pdat->prev_shadow_result__spare_0_sd1,
		2,
		pbuffer +  36);
	VL53L1_i2c_encode_uint16_t(
		pdat->prev_shadow_result__spare_1_sd1,
		2,
		pbuffer +  38);
	VL53L1_i2c_encode_uint16_t(
		pdat->prev_shadow_result__spare_2_sd1,
		2,
		pbuffer +  40);
	VL53L1_i2c_encode_uint16_t(
		pdat->prev_shadow_result__spare_3_sd1,
		2,
		pbuffer +  42);
	LOG_FUNCTION_END(status);


	return status;
}


VL53L1_Error VL53L1_i2c_decode_prev_shadow_system_results(
	uint16_t                   buf_size,
	uint8_t                   *pbuffer,
	VL53L1_prev_shadow_system_results_t  *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (buf_size < VL53L1_PREV_SHADOW_SYSTEM_RESULTS_I2C_SIZE_BYTES)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	pdat->prev_shadow_result__interrupt_status =
		(*(pbuffer +   0)) & 0x3F;
	pdat->prev_shadow_result__range_status =
		(*(pbuffer +   1));
	pdat->prev_shadow_result__report_status =
		(*(pbuffer +   2)) & 0xF;
	pdat->prev_shadow_result__stream_count =
		(*(pbuffer +   3));
	pdat->prev_shadow_result__dss_actual_effective_spads_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +   4));
	pdat->prev_shadow_result__peak_signal_count_rate_mcps_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +   6));
	pdat->prev_shadow_result__ambient_count_rate_mcps_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +   8));
	pdat->prev_shadow_result__sigma_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  10));
	pdat->prev_shadow_result__phase_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  12));
	pdat->prev_shadow_result__final_crosstalk_corrected_range_mm_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  14));
	pdat->psr__peak_signal_count_rate_crosstalk_corrected_mcps_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  16));
	pdat->prev_shadow_result__mm_inner_actual_effective_spads_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  18));
	pdat->prev_shadow_result__mm_outer_actual_effective_spads_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  20));
	pdat->prev_shadow_result__avg_signal_count_rate_mcps_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  22));
	pdat->prev_shadow_result__dss_actual_effective_spads_sd1 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  24));
	pdat->prev_shadow_result__peak_signal_count_rate_mcps_sd1 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  26));
	pdat->prev_shadow_result__ambient_count_rate_mcps_sd1 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  28));
	pdat->prev_shadow_result__sigma_sd1 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  30));
	pdat->prev_shadow_result__phase_sd1 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  32));
	pdat->prev_shadow_result__final_crosstalk_corrected_range_mm_sd1 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  34));
	pdat->prev_shadow_result__spare_0_sd1 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  36));
	pdat->prev_shadow_result__spare_1_sd1 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  38));
	pdat->prev_shadow_result__spare_2_sd1 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  40));
	pdat->prev_shadow_result__spare_3_sd1 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  42));

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_set_prev_shadow_system_results(
	VL53L1_DEV                 Dev,
	VL53L1_prev_shadow_system_results_t  *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_PREV_SHADOW_SYSTEM_RESULTS_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_encode_prev_shadow_system_results(
			pdat,
			VL53L1_PREV_SHADOW_SYSTEM_RESULTS_I2C_SIZE_BYTES,
			comms_buffer);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_disable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_WriteMulti(
			Dev,
			VL53L1_PREV_SHADOW_RESULT__INTERRUPT_STATUS,
			comms_buffer,
			VL53L1_PREV_SHADOW_SYSTEM_RESULTS_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_enable_firmware(Dev);

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_get_prev_shadow_system_results(
	VL53L1_DEV                 Dev,
	VL53L1_prev_shadow_system_results_t  *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_PREV_SHADOW_SYSTEM_RESULTS_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_disable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_ReadMulti(
			Dev,
			VL53L1_PREV_SHADOW_RESULT__INTERRUPT_STATUS,
			comms_buffer,
			VL53L1_PREV_SHADOW_SYSTEM_RESULTS_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_enable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_decode_prev_shadow_system_results(
			VL53L1_PREV_SHADOW_SYSTEM_RESULTS_I2C_SIZE_BYTES,
			comms_buffer,
			pdat);

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_i2c_encode_prev_shadow_core_results(
	VL53L1_prev_shadow_core_results_t *pdat,
	uint16_t                  buf_size,
	uint8_t                  *pbuffer)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (buf_size < VL53L1_PREV_SHADOW_CORE_RESULTS_I2C_SIZE_BYTES)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	VL53L1_i2c_encode_uint32_t(
		pdat->prev_shadow_result_core__ambient_window_events_sd0,
		4,
		pbuffer +   0);
	VL53L1_i2c_encode_uint32_t(
		pdat->prev_shadow_result_core__ranging_total_events_sd0,
		4,
		pbuffer +   4);
	VL53L1_i2c_encode_int32_t(
		pdat->prev_shadow_result_core__signal_total_events_sd0,
		4,
		pbuffer +   8);
	VL53L1_i2c_encode_uint32_t(
		pdat->prev_shadow_result_core__total_periods_elapsed_sd0,
		4,
		pbuffer +  12);
	VL53L1_i2c_encode_uint32_t(
		pdat->prev_shadow_result_core__ambient_window_events_sd1,
		4,
		pbuffer +  16);
	VL53L1_i2c_encode_uint32_t(
		pdat->prev_shadow_result_core__ranging_total_events_sd1,
		4,
		pbuffer +  20);
	VL53L1_i2c_encode_int32_t(
		pdat->prev_shadow_result_core__signal_total_events_sd1,
		4,
		pbuffer +  24);
	VL53L1_i2c_encode_uint32_t(
		pdat->prev_shadow_result_core__total_periods_elapsed_sd1,
		4,
		pbuffer +  28);
	*(pbuffer +  32) =
		pdat->prev_shadow_result_core__spare_0;
	LOG_FUNCTION_END(status);


	return status;
}


VL53L1_Error VL53L1_i2c_decode_prev_shadow_core_results(
	uint16_t                   buf_size,
	uint8_t                   *pbuffer,
	VL53L1_prev_shadow_core_results_t  *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (buf_size < VL53L1_PREV_SHADOW_CORE_RESULTS_I2C_SIZE_BYTES)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	pdat->prev_shadow_result_core__ambient_window_events_sd0 =
		(VL53L1_i2c_decode_uint32_t(4, pbuffer +   0));
	pdat->prev_shadow_result_core__ranging_total_events_sd0 =
		(VL53L1_i2c_decode_uint32_t(4, pbuffer +   4));
	pdat->prev_shadow_result_core__signal_total_events_sd0 =
		(VL53L1_i2c_decode_int32_t(4, pbuffer +   8));
	pdat->prev_shadow_result_core__total_periods_elapsed_sd0 =
		(VL53L1_i2c_decode_uint32_t(4, pbuffer +  12));
	pdat->prev_shadow_result_core__ambient_window_events_sd1 =
		(VL53L1_i2c_decode_uint32_t(4, pbuffer +  16));
	pdat->prev_shadow_result_core__ranging_total_events_sd1 =
		(VL53L1_i2c_decode_uint32_t(4, pbuffer +  20));
	pdat->prev_shadow_result_core__signal_total_events_sd1 =
		(VL53L1_i2c_decode_int32_t(4, pbuffer +  24));
	pdat->prev_shadow_result_core__total_periods_elapsed_sd1 =
		(VL53L1_i2c_decode_uint32_t(4, pbuffer +  28));
	pdat->prev_shadow_result_core__spare_0 =
		(*(pbuffer +  32));

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_set_prev_shadow_core_results(
	VL53L1_DEV                 Dev,
	VL53L1_prev_shadow_core_results_t  *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_PREV_SHADOW_CORE_RESULTS_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_encode_prev_shadow_core_results(
			pdat,
			VL53L1_PREV_SHADOW_CORE_RESULTS_I2C_SIZE_BYTES,
			comms_buffer);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_disable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_WriteMulti(
		Dev,
		VL53L1_PREV_SHADOW_RESULT_CORE__AMBIENT_WINDOW_EVENTS_SD0,
		comms_buffer,
		VL53L1_PREV_SHADOW_CORE_RESULTS_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_enable_firmware(Dev);

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_get_prev_shadow_core_results(
	VL53L1_DEV                 Dev,
	VL53L1_prev_shadow_core_results_t  *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_PREV_SHADOW_CORE_RESULTS_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_disable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_ReadMulti(
		Dev,
		VL53L1_PREV_SHADOW_RESULT_CORE__AMBIENT_WINDOW_EVENTS_SD0,
		comms_buffer,
		VL53L1_PREV_SHADOW_CORE_RESULTS_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_enable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_decode_prev_shadow_core_results(
			VL53L1_PREV_SHADOW_CORE_RESULTS_I2C_SIZE_BYTES,
			comms_buffer,
			pdat);

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_i2c_encode_patch_debug(
	VL53L1_patch_debug_t     *pdat,
	uint16_t                  buf_size,
	uint8_t                  *pbuffer)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (buf_size < VL53L1_PATCH_DEBUG_I2C_SIZE_BYTES)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	*(pbuffer +   0) =
		pdat->result__debug_status;
	*(pbuffer +   1) =
		pdat->result__debug_stage;
	LOG_FUNCTION_END(status);


	return status;
}


VL53L1_Error VL53L1_i2c_decode_patch_debug(
	uint16_t                   buf_size,
	uint8_t                   *pbuffer,
	VL53L1_patch_debug_t      *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (buf_size < VL53L1_PATCH_DEBUG_I2C_SIZE_BYTES)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	pdat->result__debug_status =
		(*(pbuffer +   0));
	pdat->result__debug_stage =
		(*(pbuffer +   1));

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_set_patch_debug(
	VL53L1_DEV                 Dev,
	VL53L1_patch_debug_t      *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_PATCH_DEBUG_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_encode_patch_debug(
			pdat,
			VL53L1_PATCH_DEBUG_I2C_SIZE_BYTES,
			comms_buffer);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_disable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_WriteMulti(
			Dev,
			VL53L1_RESULT__DEBUG_STATUS,
			comms_buffer,
			VL53L1_PATCH_DEBUG_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_enable_firmware(Dev);

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_get_patch_debug(
	VL53L1_DEV                 Dev,
	VL53L1_patch_debug_t      *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_PATCH_DEBUG_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_disable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_ReadMulti(
			Dev,
			VL53L1_RESULT__DEBUG_STATUS,
			comms_buffer,
			VL53L1_PATCH_DEBUG_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_enable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_decode_patch_debug(
			VL53L1_PATCH_DEBUG_I2C_SIZE_BYTES,
			comms_buffer,
			pdat);

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_i2c_encode_gph_general_config(
	VL53L1_gph_general_config_t *pdat,
	uint16_t                  buf_size,
	uint8_t                  *pbuffer)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (buf_size < VL53L1_GPH_GENERAL_CONFIG_I2C_SIZE_BYTES)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	VL53L1_i2c_encode_uint16_t(
		pdat->gph__system__thresh_rate_high,
		2,
		pbuffer +   0);
	VL53L1_i2c_encode_uint16_t(
		pdat->gph__system__thresh_rate_low,
		2,
		pbuffer +   2);
	*(pbuffer +   4) =
		pdat->gph__system__interrupt_config_gpio;
	LOG_FUNCTION_END(status);


	return status;
}


VL53L1_Error VL53L1_i2c_decode_gph_general_config(
	uint16_t                   buf_size,
	uint8_t                   *pbuffer,
	VL53L1_gph_general_config_t  *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (buf_size < VL53L1_GPH_GENERAL_CONFIG_I2C_SIZE_BYTES)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	pdat->gph__system__thresh_rate_high =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +   0));
	pdat->gph__system__thresh_rate_low =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +   2));
	pdat->gph__system__interrupt_config_gpio =
		(*(pbuffer +   4));

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_set_gph_general_config(
	VL53L1_DEV                 Dev,
	VL53L1_gph_general_config_t  *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_GPH_GENERAL_CONFIG_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_encode_gph_general_config(
			pdat,
			VL53L1_GPH_GENERAL_CONFIG_I2C_SIZE_BYTES,
			comms_buffer);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_disable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_WriteMulti(
			Dev,
			VL53L1_GPH__SYSTEM__THRESH_RATE_HIGH,
			comms_buffer,
			VL53L1_GPH_GENERAL_CONFIG_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_enable_firmware(Dev);

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_get_gph_general_config(
	VL53L1_DEV                 Dev,
	VL53L1_gph_general_config_t  *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_GPH_GENERAL_CONFIG_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_disable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_ReadMulti(
			Dev,
			VL53L1_GPH__SYSTEM__THRESH_RATE_HIGH,
			comms_buffer,
			VL53L1_GPH_GENERAL_CONFIG_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_enable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_decode_gph_general_config(
			VL53L1_GPH_GENERAL_CONFIG_I2C_SIZE_BYTES,
			comms_buffer,
			pdat);

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_i2c_encode_gph_static_config(
	VL53L1_gph_static_config_t *pdat,
	uint16_t                  buf_size,
	uint8_t                  *pbuffer)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (buf_size < VL53L1_GPH_STATIC_CONFIG_I2C_SIZE_BYTES)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	*(pbuffer +   0) =
		pdat->gph__dss_config__roi_mode_control & 0x7;
	VL53L1_i2c_encode_uint16_t(
		pdat->gph__dss_config__manual_effective_spads_select,
		2,
		pbuffer +   1);
	*(pbuffer +   3) =
		pdat->gph__dss_config__manual_block_select;
	*(pbuffer +   4) =
		pdat->gph__dss_config__max_spads_limit;
	*(pbuffer +   5) =
		pdat->gph__dss_config__min_spads_limit;
	LOG_FUNCTION_END(status);


	return status;
}


VL53L1_Error VL53L1_i2c_decode_gph_static_config(
	uint16_t                   buf_size,
	uint8_t                   *pbuffer,
	VL53L1_gph_static_config_t  *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (buf_size < VL53L1_GPH_STATIC_CONFIG_I2C_SIZE_BYTES)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	pdat->gph__dss_config__roi_mode_control =
		(*(pbuffer +   0)) & 0x7;
	pdat->gph__dss_config__manual_effective_spads_select =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +   1));
	pdat->gph__dss_config__manual_block_select =
		(*(pbuffer +   3));
	pdat->gph__dss_config__max_spads_limit =
		(*(pbuffer +   4));
	pdat->gph__dss_config__min_spads_limit =
		(*(pbuffer +   5));

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_set_gph_static_config(
	VL53L1_DEV                 Dev,
	VL53L1_gph_static_config_t  *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_GPH_STATIC_CONFIG_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_encode_gph_static_config(
			pdat,
			VL53L1_GPH_STATIC_CONFIG_I2C_SIZE_BYTES,
			comms_buffer);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_disable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_WriteMulti(
			Dev,
			VL53L1_GPH__DSS_CONFIG__ROI_MODE_CONTROL,
			comms_buffer,
			VL53L1_GPH_STATIC_CONFIG_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_enable_firmware(Dev);

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_get_gph_static_config(
	VL53L1_DEV                 Dev,
	VL53L1_gph_static_config_t  *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_GPH_STATIC_CONFIG_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_disable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_ReadMulti(
			Dev,
			VL53L1_GPH__DSS_CONFIG__ROI_MODE_CONTROL,
			comms_buffer,
			VL53L1_GPH_STATIC_CONFIG_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_enable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_decode_gph_static_config(
			VL53L1_GPH_STATIC_CONFIG_I2C_SIZE_BYTES,
			comms_buffer,
			pdat);

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_i2c_encode_gph_timing_config(
	VL53L1_gph_timing_config_t *pdat,
	uint16_t                  buf_size,
	uint8_t                  *pbuffer)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (buf_size < VL53L1_GPH_TIMING_CONFIG_I2C_SIZE_BYTES)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	*(pbuffer +   0) =
		pdat->gph__mm_config__timeout_macrop_a_hi & 0xF;
	*(pbuffer +   1) =
		pdat->gph__mm_config__timeout_macrop_a_lo;
	*(pbuffer +   2) =
		pdat->gph__mm_config__timeout_macrop_b_hi & 0xF;
	*(pbuffer +   3) =
		pdat->gph__mm_config__timeout_macrop_b_lo;
	*(pbuffer +   4) =
		pdat->gph__range_config__timeout_macrop_a_hi & 0xF;
	*(pbuffer +   5) =
		pdat->gph__range_config__timeout_macrop_a_lo;
	*(pbuffer +   6) =
		pdat->gph__range_config__vcsel_period_a & 0x3F;
	*(pbuffer +   7) =
		pdat->gph__range_config__vcsel_period_b & 0x3F;
	*(pbuffer +   8) =
		pdat->gph__range_config__timeout_macrop_b_hi & 0xF;
	*(pbuffer +   9) =
		pdat->gph__range_config__timeout_macrop_b_lo;
	VL53L1_i2c_encode_uint16_t(
		pdat->gph__range_config__sigma_thresh,
		2,
		pbuffer +  10);
	VL53L1_i2c_encode_uint16_t(
		pdat->gph__range_config__min_count_rate_rtn_limit_mcps,
		2,
		pbuffer +  12);
	*(pbuffer +  14) =
		pdat->gph__range_config__valid_phase_low;
	*(pbuffer +  15) =
		pdat->gph__range_config__valid_phase_high;
	LOG_FUNCTION_END(status);


	return status;
}


VL53L1_Error VL53L1_i2c_decode_gph_timing_config(
	uint16_t                   buf_size,
	uint8_t                   *pbuffer,
	VL53L1_gph_timing_config_t  *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (buf_size < VL53L1_GPH_TIMING_CONFIG_I2C_SIZE_BYTES)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	pdat->gph__mm_config__timeout_macrop_a_hi =
		(*(pbuffer +   0)) & 0xF;
	pdat->gph__mm_config__timeout_macrop_a_lo =
		(*(pbuffer +   1));
	pdat->gph__mm_config__timeout_macrop_b_hi =
		(*(pbuffer +   2)) & 0xF;
	pdat->gph__mm_config__timeout_macrop_b_lo =
		(*(pbuffer +   3));
	pdat->gph__range_config__timeout_macrop_a_hi =
		(*(pbuffer +   4)) & 0xF;
	pdat->gph__range_config__timeout_macrop_a_lo =
		(*(pbuffer +   5));
	pdat->gph__range_config__vcsel_period_a =
		(*(pbuffer +   6)) & 0x3F;
	pdat->gph__range_config__vcsel_period_b =
		(*(pbuffer +   7)) & 0x3F;
	pdat->gph__range_config__timeout_macrop_b_hi =
		(*(pbuffer +   8)) & 0xF;
	pdat->gph__range_config__timeout_macrop_b_lo =
		(*(pbuffer +   9));
	pdat->gph__range_config__sigma_thresh =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  10));
	pdat->gph__range_config__min_count_rate_rtn_limit_mcps =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  12));
	pdat->gph__range_config__valid_phase_low =
		(*(pbuffer +  14));
	pdat->gph__range_config__valid_phase_high =
		(*(pbuffer +  15));

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_set_gph_timing_config(
	VL53L1_DEV                 Dev,
	VL53L1_gph_timing_config_t  *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_GPH_TIMING_CONFIG_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_encode_gph_timing_config(
			pdat,
			VL53L1_GPH_TIMING_CONFIG_I2C_SIZE_BYTES,
			comms_buffer);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_disable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_WriteMulti(
			Dev,
			VL53L1_GPH__MM_CONFIG__TIMEOUT_MACROP_A_HI,
			comms_buffer,
			VL53L1_GPH_TIMING_CONFIG_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_enable_firmware(Dev);

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_get_gph_timing_config(
	VL53L1_DEV                 Dev,
	VL53L1_gph_timing_config_t  *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_GPH_TIMING_CONFIG_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_disable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_ReadMulti(
			Dev,
			VL53L1_GPH__MM_CONFIG__TIMEOUT_MACROP_A_HI,
			comms_buffer,
			VL53L1_GPH_TIMING_CONFIG_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_enable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_decode_gph_timing_config(
			VL53L1_GPH_TIMING_CONFIG_I2C_SIZE_BYTES,
			comms_buffer,
			pdat);

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_i2c_encode_fw_internal(
	VL53L1_fw_internal_t     *pdat,
	uint16_t                  buf_size,
	uint8_t                  *pbuffer)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (buf_size < VL53L1_FW_INTERNAL_I2C_SIZE_BYTES)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	*(pbuffer +   0) =
		pdat->firmware__internal_stream_count_div;
	*(pbuffer +   1) =
		pdat->firmware__internal_stream_counter_val;
	LOG_FUNCTION_END(status);


	return status;
}


VL53L1_Error VL53L1_i2c_decode_fw_internal(
	uint16_t                   buf_size,
	uint8_t                   *pbuffer,
	VL53L1_fw_internal_t      *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (buf_size < VL53L1_FW_INTERNAL_I2C_SIZE_BYTES)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	pdat->firmware__internal_stream_count_div =
		(*(pbuffer +   0));
	pdat->firmware__internal_stream_counter_val =
		(*(pbuffer +   1));

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_set_fw_internal(
	VL53L1_DEV                 Dev,
	VL53L1_fw_internal_t      *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_FW_INTERNAL_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_encode_fw_internal(
			pdat,
			VL53L1_FW_INTERNAL_I2C_SIZE_BYTES,
			comms_buffer);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_disable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_WriteMulti(
			Dev,
			VL53L1_FIRMWARE__INTERNAL_STREAM_COUNT_DIV,
			comms_buffer,
			VL53L1_FW_INTERNAL_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_enable_firmware(Dev);

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_get_fw_internal(
	VL53L1_DEV                 Dev,
	VL53L1_fw_internal_t      *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_FW_INTERNAL_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_disable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_ReadMulti(
			Dev,
			VL53L1_FIRMWARE__INTERNAL_STREAM_COUNT_DIV,
			comms_buffer,
			VL53L1_FW_INTERNAL_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_enable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_decode_fw_internal(
			VL53L1_FW_INTERNAL_I2C_SIZE_BYTES,
			comms_buffer,
			pdat);

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_i2c_encode_patch_results(
	VL53L1_patch_results_t   *pdat,
	uint16_t                  buf_size,
	uint8_t                  *pbuffer)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (buf_size < VL53L1_PATCH_RESULTS_I2C_SIZE_BYTES)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	*(pbuffer +   0) =
		pdat->dss_calc__roi_ctrl & 0x3;
	*(pbuffer +   1) =
		pdat->dss_calc__spare_1;
	*(pbuffer +   2) =
		pdat->dss_calc__spare_2;
	*(pbuffer +   3) =
		pdat->dss_calc__spare_3;
	*(pbuffer +   4) =
		pdat->dss_calc__spare_4;
	*(pbuffer +   5) =
		pdat->dss_calc__spare_5;
	*(pbuffer +   6) =
		pdat->dss_calc__spare_6;
	*(pbuffer +   7) =
		pdat->dss_calc__spare_7;
	*(pbuffer +   8) =
		pdat->dss_calc__user_roi_spad_en_0;
	*(pbuffer +   9) =
		pdat->dss_calc__user_roi_spad_en_1;
	*(pbuffer +  10) =
		pdat->dss_calc__user_roi_spad_en_2;
	*(pbuffer +  11) =
		pdat->dss_calc__user_roi_spad_en_3;
	*(pbuffer +  12) =
		pdat->dss_calc__user_roi_spad_en_4;
	*(pbuffer +  13) =
		pdat->dss_calc__user_roi_spad_en_5;
	*(pbuffer +  14) =
		pdat->dss_calc__user_roi_spad_en_6;
	*(pbuffer +  15) =
		pdat->dss_calc__user_roi_spad_en_7;
	*(pbuffer +  16) =
		pdat->dss_calc__user_roi_spad_en_8;
	*(pbuffer +  17) =
		pdat->dss_calc__user_roi_spad_en_9;
	*(pbuffer +  18) =
		pdat->dss_calc__user_roi_spad_en_10;
	*(pbuffer +  19) =
		pdat->dss_calc__user_roi_spad_en_11;
	*(pbuffer +  20) =
		pdat->dss_calc__user_roi_spad_en_12;
	*(pbuffer +  21) =
		pdat->dss_calc__user_roi_spad_en_13;
	*(pbuffer +  22) =
		pdat->dss_calc__user_roi_spad_en_14;
	*(pbuffer +  23) =
		pdat->dss_calc__user_roi_spad_en_15;
	*(pbuffer +  24) =
		pdat->dss_calc__user_roi_spad_en_16;
	*(pbuffer +  25) =
		pdat->dss_calc__user_roi_spad_en_17;
	*(pbuffer +  26) =
		pdat->dss_calc__user_roi_spad_en_18;
	*(pbuffer +  27) =
		pdat->dss_calc__user_roi_spad_en_19;
	*(pbuffer +  28) =
		pdat->dss_calc__user_roi_spad_en_20;
	*(pbuffer +  29) =
		pdat->dss_calc__user_roi_spad_en_21;
	*(pbuffer +  30) =
		pdat->dss_calc__user_roi_spad_en_22;
	*(pbuffer +  31) =
		pdat->dss_calc__user_roi_spad_en_23;
	*(pbuffer +  32) =
		pdat->dss_calc__user_roi_spad_en_24;
	*(pbuffer +  33) =
		pdat->dss_calc__user_roi_spad_en_25;
	*(pbuffer +  34) =
		pdat->dss_calc__user_roi_spad_en_26;
	*(pbuffer +  35) =
		pdat->dss_calc__user_roi_spad_en_27;
	*(pbuffer +  36) =
		pdat->dss_calc__user_roi_spad_en_28;
	*(pbuffer +  37) =
		pdat->dss_calc__user_roi_spad_en_29;
	*(pbuffer +  38) =
		pdat->dss_calc__user_roi_spad_en_30;
	*(pbuffer +  39) =
		pdat->dss_calc__user_roi_spad_en_31;
	*(pbuffer +  40) =
		pdat->dss_calc__user_roi_0;
	*(pbuffer +  41) =
		pdat->dss_calc__user_roi_1;
	*(pbuffer +  42) =
		pdat->dss_calc__mode_roi_0;
	*(pbuffer +  43) =
		pdat->dss_calc__mode_roi_1;
	*(pbuffer +  44) =
		pdat->sigma_estimator_calc__spare_0;
	VL53L1_i2c_encode_uint16_t(
		pdat->vhv_result__peak_signal_rate_mcps,
		2,
		pbuffer +  46);
	VL53L1_i2c_encode_uint32_t(
		pdat->vhv_result__signal_total_events_ref,
		4,
		pbuffer +  48);
	VL53L1_i2c_encode_uint16_t(
		pdat->phasecal_result__phase_output_ref,
		2,
		pbuffer +  52);
	VL53L1_i2c_encode_uint16_t(
		pdat->dss_result__total_rate_per_spad,
		2,
		pbuffer +  54);
	*(pbuffer +  56) =
		pdat->dss_result__enabled_blocks;
	VL53L1_i2c_encode_uint16_t(
		pdat->dss_result__num_requested_spads,
		2,
		pbuffer +  58);
	VL53L1_i2c_encode_uint16_t(
		pdat->mm_result__inner_intersection_rate,
		2,
		pbuffer +  62);
	VL53L1_i2c_encode_uint16_t(
		pdat->mm_result__outer_complement_rate,
		2,
		pbuffer +  64);
	VL53L1_i2c_encode_uint16_t(
		pdat->mm_result__total_offset,
		2,
		pbuffer +  66);
	VL53L1_i2c_encode_uint32_t(
		pdat->xtalk_calc__xtalk_for_enabled_spads & 0xFFFFFF,
		4,
		pbuffer +  68);
	VL53L1_i2c_encode_uint32_t(
		pdat->xtalk_result__avg_xtalk_user_roi_kcps & 0xFFFFFF,
		4,
		pbuffer +  72);
	VL53L1_i2c_encode_uint32_t(
		pdat->xtalk_result__avg_xtalk_mm_inner_roi_kcps & 0xFFFFFF,
		4,
		pbuffer +  76);
	VL53L1_i2c_encode_uint32_t(
		pdat->xtalk_result__avg_xtalk_mm_outer_roi_kcps & 0xFFFFFF,
		4,
		pbuffer +  80);
	VL53L1_i2c_encode_uint32_t(
		pdat->range_result__accum_phase,
		4,
		pbuffer +  84);
	VL53L1_i2c_encode_uint16_t(
		pdat->range_result__offset_corrected_range,
		2,
		pbuffer +  88);
	LOG_FUNCTION_END(status);


	return status;
}


VL53L1_Error VL53L1_i2c_decode_patch_results(
	uint16_t                   buf_size,
	uint8_t                   *pbuffer,
	VL53L1_patch_results_t    *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (buf_size < VL53L1_PATCH_RESULTS_I2C_SIZE_BYTES)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	pdat->dss_calc__roi_ctrl =
		(*(pbuffer +   0)) & 0x3;
	pdat->dss_calc__spare_1 =
		(*(pbuffer +   1));
	pdat->dss_calc__spare_2 =
		(*(pbuffer +   2));
	pdat->dss_calc__spare_3 =
		(*(pbuffer +   3));
	pdat->dss_calc__spare_4 =
		(*(pbuffer +   4));
	pdat->dss_calc__spare_5 =
		(*(pbuffer +   5));
	pdat->dss_calc__spare_6 =
		(*(pbuffer +   6));
	pdat->dss_calc__spare_7 =
		(*(pbuffer +   7));
	pdat->dss_calc__user_roi_spad_en_0 =
		(*(pbuffer +   8));
	pdat->dss_calc__user_roi_spad_en_1 =
		(*(pbuffer +   9));
	pdat->dss_calc__user_roi_spad_en_2 =
		(*(pbuffer +  10));
	pdat->dss_calc__user_roi_spad_en_3 =
		(*(pbuffer +  11));
	pdat->dss_calc__user_roi_spad_en_4 =
		(*(pbuffer +  12));
	pdat->dss_calc__user_roi_spad_en_5 =
		(*(pbuffer +  13));
	pdat->dss_calc__user_roi_spad_en_6 =
		(*(pbuffer +  14));
	pdat->dss_calc__user_roi_spad_en_7 =
		(*(pbuffer +  15));
	pdat->dss_calc__user_roi_spad_en_8 =
		(*(pbuffer +  16));
	pdat->dss_calc__user_roi_spad_en_9 =
		(*(pbuffer +  17));
	pdat->dss_calc__user_roi_spad_en_10 =
		(*(pbuffer +  18));
	pdat->dss_calc__user_roi_spad_en_11 =
		(*(pbuffer +  19));
	pdat->dss_calc__user_roi_spad_en_12 =
		(*(pbuffer +  20));
	pdat->dss_calc__user_roi_spad_en_13 =
		(*(pbuffer +  21));
	pdat->dss_calc__user_roi_spad_en_14 =
		(*(pbuffer +  22));
	pdat->dss_calc__user_roi_spad_en_15 =
		(*(pbuffer +  23));
	pdat->dss_calc__user_roi_spad_en_16 =
		(*(pbuffer +  24));
	pdat->dss_calc__user_roi_spad_en_17 =
		(*(pbuffer +  25));
	pdat->dss_calc__user_roi_spad_en_18 =
		(*(pbuffer +  26));
	pdat->dss_calc__user_roi_spad_en_19 =
		(*(pbuffer +  27));
	pdat->dss_calc__user_roi_spad_en_20 =
		(*(pbuffer +  28));
	pdat->dss_calc__user_roi_spad_en_21 =
		(*(pbuffer +  29));
	pdat->dss_calc__user_roi_spad_en_22 =
		(*(pbuffer +  30));
	pdat->dss_calc__user_roi_spad_en_23 =
		(*(pbuffer +  31));
	pdat->dss_calc__user_roi_spad_en_24 =
		(*(pbuffer +  32));
	pdat->dss_calc__user_roi_spad_en_25 =
		(*(pbuffer +  33));
	pdat->dss_calc__user_roi_spad_en_26 =
		(*(pbuffer +  34));
	pdat->dss_calc__user_roi_spad_en_27 =
		(*(pbuffer +  35));
	pdat->dss_calc__user_roi_spad_en_28 =
		(*(pbuffer +  36));
	pdat->dss_calc__user_roi_spad_en_29 =
		(*(pbuffer +  37));
	pdat->dss_calc__user_roi_spad_en_30 =
		(*(pbuffer +  38));
	pdat->dss_calc__user_roi_spad_en_31 =
		(*(pbuffer +  39));
	pdat->dss_calc__user_roi_0 =
		(*(pbuffer +  40));
	pdat->dss_calc__user_roi_1 =
		(*(pbuffer +  41));
	pdat->dss_calc__mode_roi_0 =
		(*(pbuffer +  42));
	pdat->dss_calc__mode_roi_1 =
		(*(pbuffer +  43));
	pdat->sigma_estimator_calc__spare_0 =
		(*(pbuffer +  44));
	pdat->vhv_result__peak_signal_rate_mcps =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  46));
	pdat->vhv_result__signal_total_events_ref =
		(VL53L1_i2c_decode_uint32_t(4, pbuffer +  48));
	pdat->phasecal_result__phase_output_ref =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  52));
	pdat->dss_result__total_rate_per_spad =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  54));
	pdat->dss_result__enabled_blocks =
		(*(pbuffer +  56));
	pdat->dss_result__num_requested_spads =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  58));
	pdat->mm_result__inner_intersection_rate =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  62));
	pdat->mm_result__outer_complement_rate =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  64));
	pdat->mm_result__total_offset =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  66));
	pdat->xtalk_calc__xtalk_for_enabled_spads =
		(VL53L1_i2c_decode_uint32_t(4, pbuffer +  68)) & 0xFFFFFF;
	pdat->xtalk_result__avg_xtalk_user_roi_kcps =
		(VL53L1_i2c_decode_uint32_t(4, pbuffer +  72)) & 0xFFFFFF;
	pdat->xtalk_result__avg_xtalk_mm_inner_roi_kcps =
		(VL53L1_i2c_decode_uint32_t(4, pbuffer +  76)) & 0xFFFFFF;
	pdat->xtalk_result__avg_xtalk_mm_outer_roi_kcps =
		(VL53L1_i2c_decode_uint32_t(4, pbuffer +  80)) & 0xFFFFFF;
	pdat->range_result__accum_phase =
		(VL53L1_i2c_decode_uint32_t(4, pbuffer +  84));
	pdat->range_result__offset_corrected_range =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  88));

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_set_patch_results(
	VL53L1_DEV                 Dev,
	VL53L1_patch_results_t    *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_PATCH_RESULTS_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_encode_patch_results(
			pdat,
			VL53L1_PATCH_RESULTS_I2C_SIZE_BYTES,
			comms_buffer);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_disable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_WriteMulti(
			Dev,
			VL53L1_DSS_CALC__ROI_CTRL,
			comms_buffer,
			VL53L1_PATCH_RESULTS_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_enable_firmware(Dev);

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_get_patch_results(
	VL53L1_DEV                 Dev,
	VL53L1_patch_results_t    *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_PATCH_RESULTS_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_disable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_ReadMulti(
			Dev,
			VL53L1_DSS_CALC__ROI_CTRL,
			comms_buffer,
			VL53L1_PATCH_RESULTS_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_enable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_decode_patch_results(
			VL53L1_PATCH_RESULTS_I2C_SIZE_BYTES,
			comms_buffer,
			pdat);

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_i2c_encode_shadow_system_results(
	VL53L1_shadow_system_results_t *pdat,
	uint16_t                  buf_size,
	uint8_t                  *pbuffer)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (buf_size < VL53L1_SHADOW_SYSTEM_RESULTS_I2C_SIZE_BYTES)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	*(pbuffer +   0) =
		pdat->shadow_phasecal_result__vcsel_start;
	*(pbuffer +   2) =
		pdat->shadow_result__interrupt_status & 0x3F;
	*(pbuffer +   3) =
		pdat->shadow_result__range_status;
	*(pbuffer +   4) =
		pdat->shadow_result__report_status & 0xF;
	*(pbuffer +   5) =
		pdat->shadow_result__stream_count;
	VL53L1_i2c_encode_uint16_t(
		pdat->shadow_result__dss_actual_effective_spads_sd0,
		2,
		pbuffer +   6);
	VL53L1_i2c_encode_uint16_t(
		pdat->shadow_result__peak_signal_count_rate_mcps_sd0,
		2,
		pbuffer +   8);
	VL53L1_i2c_encode_uint16_t(
		pdat->shadow_result__ambient_count_rate_mcps_sd0,
		2,
		pbuffer +  10);
	VL53L1_i2c_encode_uint16_t(
		pdat->shadow_result__sigma_sd0,
		2,
		pbuffer +  12);
	VL53L1_i2c_encode_uint16_t(
		pdat->shadow_result__phase_sd0,
		2,
		pbuffer +  14);
	VL53L1_i2c_encode_uint16_t(
		pdat->shadow_result__final_crosstalk_corrected_range_mm_sd0,
		2,
		pbuffer +  16);
	VL53L1_i2c_encode_uint16_t(
		pdat->shr__peak_signal_count_rate_crosstalk_corrected_mcps_sd0,
		2,
		pbuffer +  18);
	VL53L1_i2c_encode_uint16_t(
		pdat->shadow_result__mm_inner_actual_effective_spads_sd0,
		2,
		pbuffer +  20);
	VL53L1_i2c_encode_uint16_t(
		pdat->shadow_result__mm_outer_actual_effective_spads_sd0,
		2,
		pbuffer +  22);
	VL53L1_i2c_encode_uint16_t(
		pdat->shadow_result__avg_signal_count_rate_mcps_sd0,
		2,
		pbuffer +  24);
	VL53L1_i2c_encode_uint16_t(
		pdat->shadow_result__dss_actual_effective_spads_sd1,
		2,
		pbuffer +  26);
	VL53L1_i2c_encode_uint16_t(
		pdat->shadow_result__peak_signal_count_rate_mcps_sd1,
		2,
		pbuffer +  28);
	VL53L1_i2c_encode_uint16_t(
		pdat->shadow_result__ambient_count_rate_mcps_sd1,
		2,
		pbuffer +  30);
	VL53L1_i2c_encode_uint16_t(
		pdat->shadow_result__sigma_sd1,
		2,
		pbuffer +  32);
	VL53L1_i2c_encode_uint16_t(
		pdat->shadow_result__phase_sd1,
		2,
		pbuffer +  34);
	VL53L1_i2c_encode_uint16_t(
		pdat->shadow_result__final_crosstalk_corrected_range_mm_sd1,
		2,
		pbuffer +  36);
	VL53L1_i2c_encode_uint16_t(
		pdat->shadow_result__spare_0_sd1,
		2,
		pbuffer +  38);
	VL53L1_i2c_encode_uint16_t(
		pdat->shadow_result__spare_1_sd1,
		2,
		pbuffer +  40);
	VL53L1_i2c_encode_uint16_t(
		pdat->shadow_result__spare_2_sd1,
		2,
		pbuffer +  42);
	*(pbuffer +  44) =
		pdat->shadow_result__spare_3_sd1;
	*(pbuffer +  45) =
		pdat->shadow_result__thresh_info;
	*(pbuffer +  80) =
		pdat->shadow_phasecal_result__reference_phase_hi;
	*(pbuffer +  81) =
		pdat->shadow_phasecal_result__reference_phase_lo;
	LOG_FUNCTION_END(status);


	return status;
}


VL53L1_Error VL53L1_i2c_decode_shadow_system_results(
	uint16_t                   buf_size,
	uint8_t                   *pbuffer,
	VL53L1_shadow_system_results_t  *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (buf_size < VL53L1_SHADOW_SYSTEM_RESULTS_I2C_SIZE_BYTES)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	pdat->shadow_phasecal_result__vcsel_start =
		(*(pbuffer +   0));
	pdat->shadow_result__interrupt_status =
		(*(pbuffer +   2)) & 0x3F;
	pdat->shadow_result__range_status =
		(*(pbuffer +   3));
	pdat->shadow_result__report_status =
		(*(pbuffer +   4)) & 0xF;
	pdat->shadow_result__stream_count =
		(*(pbuffer +   5));
	pdat->shadow_result__dss_actual_effective_spads_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +   6));
	pdat->shadow_result__peak_signal_count_rate_mcps_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +   8));
	pdat->shadow_result__ambient_count_rate_mcps_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  10));
	pdat->shadow_result__sigma_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  12));
	pdat->shadow_result__phase_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  14));
	pdat->shadow_result__final_crosstalk_corrected_range_mm_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  16));
	pdat->shr__peak_signal_count_rate_crosstalk_corrected_mcps_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  18));
	pdat->shadow_result__mm_inner_actual_effective_spads_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  20));
	pdat->shadow_result__mm_outer_actual_effective_spads_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  22));
	pdat->shadow_result__avg_signal_count_rate_mcps_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  24));
	pdat->shadow_result__dss_actual_effective_spads_sd1 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  26));
	pdat->shadow_result__peak_signal_count_rate_mcps_sd1 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  28));
	pdat->shadow_result__ambient_count_rate_mcps_sd1 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  30));
	pdat->shadow_result__sigma_sd1 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  32));
	pdat->shadow_result__phase_sd1 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  34));
	pdat->shadow_result__final_crosstalk_corrected_range_mm_sd1 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  36));
	pdat->shadow_result__spare_0_sd1 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  38));
	pdat->shadow_result__spare_1_sd1 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  40));
	pdat->shadow_result__spare_2_sd1 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  42));
	pdat->shadow_result__spare_3_sd1 =
		(*(pbuffer +  44));
	pdat->shadow_result__thresh_info =
		(*(pbuffer +  45));
	pdat->shadow_phasecal_result__reference_phase_hi =
		(*(pbuffer +  80));
	pdat->shadow_phasecal_result__reference_phase_lo =
		(*(pbuffer +  81));

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_set_shadow_system_results(
	VL53L1_DEV                 Dev,
	VL53L1_shadow_system_results_t  *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_SHADOW_SYSTEM_RESULTS_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_encode_shadow_system_results(
			pdat,
			VL53L1_SHADOW_SYSTEM_RESULTS_I2C_SIZE_BYTES,
			comms_buffer);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_disable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_WriteMulti(
			Dev,
			VL53L1_SHADOW_PHASECAL_RESULT__VCSEL_START,
			comms_buffer,
			VL53L1_SHADOW_SYSTEM_RESULTS_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_enable_firmware(Dev);

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_get_shadow_system_results(
	VL53L1_DEV                 Dev,
	VL53L1_shadow_system_results_t  *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_SHADOW_SYSTEM_RESULTS_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_disable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_ReadMulti(
			Dev,
			VL53L1_SHADOW_PHASECAL_RESULT__VCSEL_START,
			comms_buffer,
			VL53L1_SHADOW_SYSTEM_RESULTS_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_enable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_decode_shadow_system_results(
			VL53L1_SHADOW_SYSTEM_RESULTS_I2C_SIZE_BYTES,
			comms_buffer,
			pdat);

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_i2c_encode_shadow_core_results(
	VL53L1_shadow_core_results_t *pdat,
	uint16_t                  buf_size,
	uint8_t                  *pbuffer)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (buf_size < VL53L1_SHADOW_CORE_RESULTS_I2C_SIZE_BYTES)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	VL53L1_i2c_encode_uint32_t(
		pdat->shadow_result_core__ambient_window_events_sd0,
		4,
		pbuffer +   0);
	VL53L1_i2c_encode_uint32_t(
		pdat->shadow_result_core__ranging_total_events_sd0,
		4,
		pbuffer +   4);
	VL53L1_i2c_encode_int32_t(
		pdat->shadow_result_core__signal_total_events_sd0,
		4,
		pbuffer +   8);
	VL53L1_i2c_encode_uint32_t(
		pdat->shadow_result_core__total_periods_elapsed_sd0,
		4,
		pbuffer +  12);
	VL53L1_i2c_encode_uint32_t(
		pdat->shadow_result_core__ambient_window_events_sd1,
		4,
		pbuffer +  16);
	VL53L1_i2c_encode_uint32_t(
		pdat->shadow_result_core__ranging_total_events_sd1,
		4,
		pbuffer +  20);
	VL53L1_i2c_encode_int32_t(
		pdat->shadow_result_core__signal_total_events_sd1,
		4,
		pbuffer +  24);
	VL53L1_i2c_encode_uint32_t(
		pdat->shadow_result_core__total_periods_elapsed_sd1,
		4,
		pbuffer +  28);
	*(pbuffer +  32) =
		pdat->shadow_result_core__spare_0;
	LOG_FUNCTION_END(status);


	return status;
}


VL53L1_Error VL53L1_i2c_decode_shadow_core_results(
	uint16_t                   buf_size,
	uint8_t                   *pbuffer,
	VL53L1_shadow_core_results_t  *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (buf_size < VL53L1_SHADOW_CORE_RESULTS_I2C_SIZE_BYTES)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	pdat->shadow_result_core__ambient_window_events_sd0 =
		(VL53L1_i2c_decode_uint32_t(4, pbuffer +   0));
	pdat->shadow_result_core__ranging_total_events_sd0 =
		(VL53L1_i2c_decode_uint32_t(4, pbuffer +   4));
	pdat->shadow_result_core__signal_total_events_sd0 =
		(VL53L1_i2c_decode_int32_t(4, pbuffer +   8));
	pdat->shadow_result_core__total_periods_elapsed_sd0 =
		(VL53L1_i2c_decode_uint32_t(4, pbuffer +  12));
	pdat->shadow_result_core__ambient_window_events_sd1 =
		(VL53L1_i2c_decode_uint32_t(4, pbuffer +  16));
	pdat->shadow_result_core__ranging_total_events_sd1 =
		(VL53L1_i2c_decode_uint32_t(4, pbuffer +  20));
	pdat->shadow_result_core__signal_total_events_sd1 =
		(VL53L1_i2c_decode_int32_t(4, pbuffer +  24));
	pdat->shadow_result_core__total_periods_elapsed_sd1 =
		(VL53L1_i2c_decode_uint32_t(4, pbuffer +  28));
	pdat->shadow_result_core__spare_0 =
		(*(pbuffer +  32));

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_set_shadow_core_results(
	VL53L1_DEV                 Dev,
	VL53L1_shadow_core_results_t  *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_SHADOW_CORE_RESULTS_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_encode_shadow_core_results(
			pdat,
			VL53L1_SHADOW_CORE_RESULTS_I2C_SIZE_BYTES,
			comms_buffer);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_disable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_WriteMulti(
			Dev,
			VL53L1_SHADOW_RESULT_CORE__AMBIENT_WINDOW_EVENTS_SD0,
			comms_buffer,
			VL53L1_SHADOW_CORE_RESULTS_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_enable_firmware(Dev);

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_get_shadow_core_results(
	VL53L1_DEV                 Dev,
	VL53L1_shadow_core_results_t  *pdat)
{


	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_SHADOW_CORE_RESULTS_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_disable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_ReadMulti(
			Dev,
			VL53L1_SHADOW_RESULT_CORE__AMBIENT_WINDOW_EVENTS_SD0,
			comms_buffer,
			VL53L1_SHADOW_CORE_RESULTS_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_enable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_decode_shadow_core_results(
			VL53L1_SHADOW_CORE_RESULTS_I2C_SIZE_BYTES,
			comms_buffer,
			pdat);

	LOG_FUNCTION_END(status);

	return status;
}


