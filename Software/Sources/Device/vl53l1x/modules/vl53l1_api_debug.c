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
#include "vl53l1_ll_device.h"
#include "vl53l1_register_structs.h"
#include "vl53l1_hist_structs.h"
#include "vl53l1_nvm_structs.h"
#include "vl53l1_nvm.h"
#include "vl53l1_core.h"
#include "vl53l1_api_debug.h"

#ifdef VL53L1_LOG_ENABLE
#include "vl53l1_nvm_debug.h"
#endif

// #undef LOG_FUNCTION_START
// #define LOG_FUNCTION_START(fmt, ...) \
// 	_LOG_FUNCTION_START(VL53L1_TRACE_MODULE_CORE, fmt, __VA_ARGS__)
// #undef LOG_FUNCTION_END
// #define LOG_FUNCTION_END(status, ...) \
// 	_LOG_FUNCTION_END(VL53L1_TRACE_MODULE_CORE, status, __VA_ARGS__)
// #undef LOG_FUNCTION_END_FMT
// #define LOG_FUNCTION_END_FMT(status, fmt, ...) \
// 	_LOG_FUNCTION_END_FMT(VL53L1_TRACE_MODULE_CORE, status, \
// 	fmt, __VA_ARGS__)

// #define trace_print(level, ...) \
// 	_LOG_TRACE_PRINT(trace_flags, \
// 	level, VL53L1_TRACE_FUNCTION_NONE, __VA_ARGS__)


VL53L1_Error VL53L1_decode_calibration_data_buffer(
	uint16_t                   buf_size,
	uint8_t                   *pbuffer,
	VL53L1_calibration_data_t *pdat)
{
	VL53L1_Error  status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (sizeof(VL53L1_calibration_data_t) > buf_size)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	memcpy(pdat, pbuffer, sizeof(VL53L1_calibration_data_t));

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_get_nvm_debug_data(
	VL53L1_DEV                          Dev,
	VL53L1_decoded_nvm_data_t          *pdat)
{


	VL53L1_Error  status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	status = VL53L1_read_nvm(Dev, 0, pdat);

#ifdef VL53L1_LOG_ENABLE
	if (status == VL53L1_ERROR_NONE)
		VL53L1_print_decoded_nvm_data(
			pdat,
			"get_nvm_debug_data():pnvm_info.",
			VL53L1_TRACE_MODULE_NVM_DATA);
#endif

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_get_histogram_debug_data(
	VL53L1_DEV                          Dev,
	VL53L1_histogram_bin_data_t        *pdat)
{


	VL53L1_Error  status = VL53L1_ERROR_NONE;

	VL53L1_LLDriverData_t *pdev = VL53L1DevStructGetLLDriverHandle(Dev);

	LOG_FUNCTION_START("");


	memcpy(
		pdat,
		&(pdev->hist_data),
		sizeof(VL53L1_histogram_bin_data_t));

	LOG_FUNCTION_END(status);

	return status;
}




VL53L1_Error VL53L1_get_additional_data(
	VL53L1_DEV                       Dev,
	VL53L1_additional_data_t        *pdat)
{


	VL53L1_Error  status = VL53L1_ERROR_NONE;

	VL53L1_LLDriverData_t *pdev = VL53L1DevStructGetLLDriverHandle(Dev);

	LOG_FUNCTION_START("");



	pdat->preset_mode             = pdev->preset_mode;
	pdat->zone_preset             = pdev->zone_preset;
	pdat->measurement_mode        = pdev->measurement_mode;
	pdat->offset_calibration_mode = pdev->offset_calibration_mode;
	pdat->offset_correction_mode  = pdev->offset_correction_mode;
	pdat->dmax_mode               = pdev->dmax_mode;

	pdat->phasecal_config_timeout_us  = pdev->phasecal_config_timeout_us;
	pdat->mm_config_timeout_us        = pdev->mm_config_timeout_us;
	pdat->range_config_timeout_us     = pdev->range_config_timeout_us;
	pdat->inter_measurement_period_ms = pdev->inter_measurement_period_ms;
	pdat->dss_config__target_total_rate_mcps =
			pdev->dss_config__target_total_rate_mcps;



	status =
		VL53L1_get_histogram_debug_data(
			Dev,
			&(pdat->VL53L1_p_010));

	LOG_FUNCTION_END(status);

	return status;
}




VL53L1_Error VL53L1_get_xtalk_debug_data(
	VL53L1_DEV                          Dev,
	VL53L1_xtalk_debug_data_t          *pdat)
{


	VL53L1_Error  status = VL53L1_ERROR_NONE;

	VL53L1_LLDriverData_t *pdev = VL53L1DevStructGetLLDriverHandle(Dev);

	LOG_FUNCTION_START("");


	memcpy(
		&(pdat->customer),
		&(pdev->customer),
		sizeof(VL53L1_customer_nvm_managed_t));

	memcpy(
		&(pdat->xtalk_cfg),
		&(pdev->xtalk_cfg),
		sizeof(VL53L1_xtalk_config_t));

	memcpy(
		&(pdat->hist_data),
		&(pdev->hist_data),
		sizeof(VL53L1_histogram_bin_data_t));

	memcpy(
		&(pdat->xtalk_shapes),
		&(pdev->xtalk_shapes),
		sizeof(VL53L1_xtalk_histogram_data_t));

	memcpy(
		&(pdat->xtalk_results),
		&(pdev->xtalk_results),
		sizeof(VL53L1_xtalk_range_results_t));

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_get_offset_debug_data(
	VL53L1_DEV                          Dev,
	VL53L1_offset_debug_data_t         *pdat)
{


	VL53L1_Error  status = VL53L1_ERROR_NONE;

	VL53L1_LLDriverData_t *pdev = VL53L1DevStructGetLLDriverHandle(Dev);

	LOG_FUNCTION_START("");


	memcpy(
		&(pdat->customer),
		&(pdev->customer),
		sizeof(VL53L1_customer_nvm_managed_t));

	memcpy(
		&(pdat->fmt_dmax_cal),
		&(pdev->fmt_dmax_cal),
		sizeof(VL53L1_dmax_calibration_data_t));

	memcpy(
		&(pdat->cust_dmax_cal),
		&(pdev->cust_dmax_cal),
		sizeof(VL53L1_dmax_calibration_data_t));

	memcpy(
		&(pdat->add_off_cal_data),
		&(pdev->add_off_cal_data),
		sizeof(VL53L1_additional_offset_cal_data_t));

	memcpy(
		&(pdat->offset_results),
		&(pdev->offset_results),
		sizeof(VL53L1_offset_range_results_t));

	LOG_FUNCTION_END(status);

	return status;
}

#ifdef VL53L1_LOG_ENABLE

void  VL53L1_signed_fixed_point_sprintf(
	int32_t    signed_fp_value,
	uint8_t    frac_bits,
	uint16_t   buf_size,
	char      *pbuffer)
{


	uint32_t  fp_value      = 0;
	uint32_t  unity_fp_value = 0;
	uint32_t  sign_bit       = 0;
	uint32_t  int_part       = 0;
	uint32_t  frac_part      = 0;
	uint32_t  dec_points     = 0;
	uint32_t  dec_scaler     = 0;
	uint32_t  dec_part       = 0;

	uint64_t  tmp_long_int   = 0;

	char  fmt[VL53L1_MAX_STRING_LENGTH];

	SUPPRESS_UNUSED_WARNING(buf_size);



	sign_bit       =  signed_fp_value >> 31;

	if (sign_bit > 0) {
		fp_value = 0x80000000 -
			(0x7FFFFFFF & (uint32_t)signed_fp_value);
	} else
		fp_value = (uint32_t)signed_fp_value;

	int_part       =  fp_value >> frac_bits;
	unity_fp_value =  0x01 << frac_bits;
	frac_part      =  fp_value & (unity_fp_value-1);


	dec_points =   2;
	dec_scaler = 100;

	while (dec_scaler < unity_fp_value) {
		dec_points++;
		dec_scaler *= 10;
	}


	if (sign_bit > 0)
		sprintf(fmt, "-%%u.%%0%uu", dec_points);
	else
		sprintf(fmt,  "%%u.%%0%uu", dec_points);


	tmp_long_int  = (uint64_t)frac_part * (uint64_t)dec_scaler;
	tmp_long_int += (uint64_t)unity_fp_value/2;

	tmp_long_int = do_division_u(tmp_long_int, (uint64_t)unity_fp_value);

	dec_part = (uint32_t)tmp_long_int;


	sprintf(
		pbuffer,
		fmt,
		int_part,
		dec_part);
}


void VL53L1_print_static_nvm_managed(
	VL53L1_static_nvm_managed_t   *pdat,
	char                          *pprefix,
	uint32_t                       trace_flags)
{


	char  fp_text[VL53L1_MAX_STRING_LENGTH];

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = 0x%02X\n",
		pprefix,
		"i2c_slave__device_address",
		pdat->i2c_slave__device_address);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"ana_config__vhv_ref_sel_vddpix",
		pdat->ana_config__vhv_ref_sel_vddpix);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"ana_config__vhv_ref_sel_vquench",
		pdat->ana_config__vhv_ref_sel_vquench);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"ana_config__reg_avdd1v2_sel",
		pdat->ana_config__reg_avdd1v2_sel);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"ana_config__fast_osc__trim",
		pdat->ana_config__fast_osc__trim);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->osc_measured__fast_osc__frequency,
		12,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"osc_measured__fast_osc__frequency",
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"vhv_config__timeout_macrop_loop_bound",
		pdat->vhv_config__timeout_macrop_loop_bound);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"vhv_config__count_thresh",
		pdat->vhv_config__count_thresh);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"vhv_config__offset",
		pdat->vhv_config__offset);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"vhv_config__init",
		pdat->vhv_config__init);
}


void VL53L1_print_customer_nvm_managed(
	VL53L1_customer_nvm_managed_t *pdat,
	char                          *pprefix,
	uint32_t                       trace_flags)
{


	char  fp_text[VL53L1_MAX_STRING_LENGTH];
	int16_t tmpi16;

	trace_print(VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_ref_0",
		pdat->global_config__spad_enables_ref_0);

	trace_print(VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_ref_1",
		pdat->global_config__spad_enables_ref_1);

	trace_print(VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_ref_2",
		pdat->global_config__spad_enables_ref_2);

	trace_print(VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_ref_3",
		pdat->global_config__spad_enables_ref_3);

	trace_print(VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_ref_4",
		pdat->global_config__spad_enables_ref_4);

	trace_print(VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_ref_5",
		pdat->global_config__spad_enables_ref_5);

	trace_print(VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__ref_en_start_select",
		pdat->global_config__ref_en_start_select);

	trace_print(VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"ref_spad_man__num_requested_ref_spads",
		pdat->ref_spad_man__num_requested_ref_spads);

	trace_print(VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"ref_spad_man__ref_location",
		pdat->ref_spad_man__ref_location);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->algo__crosstalk_compensation_plane_offset_kcps,
		9,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"algo__crosstalk_compensation_plane_offset_kcps",
		fp_text);

	tmpi16 = pdat->algo__crosstalk_compensation_x_plane_gradient_kcps;
	VL53L1_signed_fixed_point_sprintf(
		(int32_t)tmpi16,
		11,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"algo__crosstalk_compensation_x_plane_gradient_kcps",
		fp_text);

	tmpi16 = pdat->algo__crosstalk_compensation_y_plane_gradient_kcps;
	VL53L1_signed_fixed_point_sprintf(
		(int32_t)tmpi16,
		11,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"algo__crosstalk_compensation_y_plane_gradient_kcps",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->ref_spad_char__total_rate_target_mcps,
		7,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"ref_spad_char__total_rate_target_mcps",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->algo__part_to_part_range_offset_mm,
		2,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"algo__part_to_part_range_offset_mm",
		fp_text);

	trace_print(VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %d\n",
		pprefix,
		"mm_config__inner_offset_mm",
		pdat->mm_config__inner_offset_mm);

	trace_print(VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %d\n",
		pprefix,
		"mm_config__outer_offset_mm",
		pdat->mm_config__outer_offset_mm);
}


void VL53L1_print_nvm_copy_data(
	VL53L1_nvm_copy_data_t      *pdat,
	char                        *pprefix,
	uint32_t                     trace_flags)
{


	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"identification__model_id",
		pdat->identification__model_id);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"identification__module_type",
		pdat->identification__module_type);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"identification__revision_id",
		pdat->identification__revision_id);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"identification__module_id",
		pdat->identification__module_id);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"ana_config__fast_osc__trim_max",
		pdat->ana_config__fast_osc__trim_max);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"ana_config__fast_osc__freq_set",
		pdat->ana_config__fast_osc__freq_set);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"ana_config__vcsel_trim",
		pdat->ana_config__vcsel_trim);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"ana_config__vcsel_selion",
		pdat->ana_config__vcsel_selion);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"ana_config__vcsel_selion_max",
		pdat->ana_config__vcsel_selion_max);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"protected_laser_safety__lock_bit",
		pdat->protected_laser_safety__lock_bit);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"laser_safety__key",
		pdat->laser_safety__key);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"laser_safety__key_ro",
		pdat->laser_safety__key_ro);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"laser_safety__clip",
		pdat->laser_safety__clip);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"laser_safety__mult",
		pdat->laser_safety__mult);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_0",
		pdat->global_config__spad_enables_rtn_0);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_1",
		pdat->global_config__spad_enables_rtn_1);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_2",
		pdat->global_config__spad_enables_rtn_2);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_3",
		pdat->global_config__spad_enables_rtn_3);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_4",
		pdat->global_config__spad_enables_rtn_4);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_5",
		pdat->global_config__spad_enables_rtn_5);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_6",
		pdat->global_config__spad_enables_rtn_6);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_7",
		pdat->global_config__spad_enables_rtn_7);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_8",
		pdat->global_config__spad_enables_rtn_8);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_9",
		pdat->global_config__spad_enables_rtn_9);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_10",
		pdat->global_config__spad_enables_rtn_10);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_11",
		pdat->global_config__spad_enables_rtn_11);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_12",
		pdat->global_config__spad_enables_rtn_12);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_13",
		pdat->global_config__spad_enables_rtn_13);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_14",
		pdat->global_config__spad_enables_rtn_14);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_15",
		pdat->global_config__spad_enables_rtn_15);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_16",
		pdat->global_config__spad_enables_rtn_16);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_17",
		pdat->global_config__spad_enables_rtn_17);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_18",
		pdat->global_config__spad_enables_rtn_18);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_19",
		pdat->global_config__spad_enables_rtn_19);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_20",
		pdat->global_config__spad_enables_rtn_20);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_21",
		pdat->global_config__spad_enables_rtn_21);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_22",
		pdat->global_config__spad_enables_rtn_22);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_23",
		pdat->global_config__spad_enables_rtn_23);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_24",
		pdat->global_config__spad_enables_rtn_24);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_25",
		pdat->global_config__spad_enables_rtn_25);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_26",
		pdat->global_config__spad_enables_rtn_26);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_27",
		pdat->global_config__spad_enables_rtn_27);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_28",
		pdat->global_config__spad_enables_rtn_28);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_29",
		pdat->global_config__spad_enables_rtn_29);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_30",
		pdat->global_config__spad_enables_rtn_30);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_31",
		pdat->global_config__spad_enables_rtn_31);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"roi_config__mode_roi_centre_spad",
		pdat->roi_config__mode_roi_centre_spad);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = 0x%02X\n",
		pprefix,
		"roi_config__mode_roi_xy_size",
		pdat->roi_config__mode_roi_xy_size);
}


void VL53L1_print_histogram_bin_data(
	VL53L1_histogram_bin_data_t *pdat,
	char                        *pprefix,
	uint32_t                     trace_flags)
{


	char  fp_text[VL53L1_MAX_STRING_LENGTH];
	char  pre_text[VL53L1_MAX_STRING_LENGTH];
	char *ppre_text = &(pre_text[0]);

	uint8_t  i             = 0;

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"cfg_device_state",
		pdat->cfg_device_state);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"rd_device_state",
		pdat->rd_device_state);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"VL53L1_p_022",
		pdat->VL53L1_p_022);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"VL53L1_p_023",
		pdat->VL53L1_p_023);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"VL53L1_p_024",
		pdat->VL53L1_p_024);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"number_of_ambient_bins",
		pdat->number_of_ambient_bins);

	for (i = 0; i < VL53L1_MAX_BIN_SEQUENCE_LENGTH; i++) {
		sprintf(ppre_text, "%sbin_seq[%u]", pprefix, i);
		trace_print(
			VL53L1_TRACE_LEVEL_INFO,
			"%s = %u\n",
			ppre_text,
			pdat->bin_seq[i]);
	}

	for (i = 0; i < VL53L1_MAX_BIN_SEQUENCE_LENGTH; i++) {
		sprintf(ppre_text, "%sbin_rep[%u]", pprefix, i);
		trace_print(
			VL53L1_TRACE_LEVEL_INFO,
			"%s = %u\n",
			ppre_text,
			pdat->bin_rep[i]);
	}

	for (i = 0; i < pdat->VL53L1_p_024; i++) {
		sprintf(ppre_text, "%sbin_data[%u]", pprefix, i);
		trace_print(
			VL53L1_TRACE_LEVEL_INFO,
			"%s = %d\n",
			ppre_text,
			pdat->bin_data[i]);
	}

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"result__interrupt_status",
		pdat->result__interrupt_status);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"result__range_status",
		pdat->result__range_status);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"result__report_status",
		pdat->result__report_status);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"result__stream_count",
		pdat->result__stream_count);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->result__dss_actual_effective_spads,
		8,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"result__dss_actual_effective_spads",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->phasecal_result__reference_phase,
		11,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"phasecal_result__reference_phase",
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"phasecal_result__vcsel_start",
		pdat->phasecal_result__vcsel_start);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"cal_config__vcsel_start",
		pdat->cal_config__vcsel_start);

	VL53L1_signed_fixed_point_sprintf(
		(uint32_t)pdat->vcsel_width,
		4,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"vcsel_width",
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"VL53L1_p_009",
		pdat->VL53L1_p_009);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->VL53L1_p_019,
		12,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"VL53L1_p_019",
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"total_periods_elapsed",
		pdat->total_periods_elapsed);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"peak_duration_us",
		pdat->peak_duration_us);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"woi_duration_us",
		pdat->woi_duration_us);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"min_bin_value",
		pdat->min_bin_value);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"max_bin_value",
		pdat->max_bin_value);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->zero_distance_phase,
		11,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"zero_distance_phase",
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"number_of_ambient_samples",
		pdat->number_of_ambient_samples);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %d\n",
		pprefix,
		"ambient_events_sum",
		pdat->ambient_events_sum);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %d\n",
		pprefix,
		"VL53L1_p_004",
		pdat->VL53L1_p_004);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = 0x%02X\n",
		pprefix,
		"roi_config__user_roi_centre_spad",
		pdat->roi_config__user_roi_centre_spad);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = 0x%02X\n",
		pprefix,
		"roi_config__user_roi_requested_global_xy_size",
		pdat->roi_config__user_roi_requested_global_xy_size);
}


void VL53L1_print_xtalk_histogram_shape_data(
	VL53L1_xtalk_histogram_shape_t *pdat,
	char                           *pprefix,
	uint32_t                        trace_flags)
{


	char  fp_text[VL53L1_MAX_STRING_LENGTH];
	char  pre_text[VL53L1_MAX_STRING_LENGTH];
	char *ppre_text = &(pre_text[0]);

	uint8_t  i             = 0;

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"VL53L1_p_022",
		pdat->VL53L1_p_022);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"VL53L1_p_023",
		pdat->VL53L1_p_023);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"VL53L1_p_024",
		pdat->VL53L1_p_024);

	for (i = 0; i < pdat->VL53L1_p_024; i++) {

		sprintf(ppre_text, "%sbin_data[%u]", pprefix, i);

		VL53L1_signed_fixed_point_sprintf(
				(int32_t)pdat->bin_data[i],
				10,
				VL53L1_MAX_STRING_LENGTH,
				fp_text);

		trace_print(
			VL53L1_TRACE_LEVEL_INFO,
			"%s = %s\n",
			ppre_text,
			fp_text);
	}

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->phasecal_result__reference_phase,
		11,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"phasecal_result__reference_phase",
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"phasecal_result__vcsel_start",
		pdat->phasecal_result__vcsel_start);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"cal_config__vcsel_start",
		pdat->cal_config__vcsel_start);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->vcsel_width,
		4,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"vcsel_width",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->VL53L1_p_019,
		12,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"VL53L1_p_019",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->zero_distance_phase,
		11,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"zero_distance_phase",
		fp_text);
}


void VL53L1_print_xtalk_histogram_data(
	VL53L1_xtalk_histogram_data_t *pdat,
	char                          *pprefix,
	uint32_t                       trace_flags)
{


	char  pre_text[VL53L1_MAX_STRING_LENGTH];
	char *ppre_text = &(pre_text[0]);


	sprintf(ppre_text, "%sxtalk_shape.", pprefix);
	VL53L1_print_xtalk_histogram_shape_data(
		&(pdat->xtalk_shape),
		ppre_text, trace_flags);


	sprintf(ppre_text, "%sxtalk_hist_removed.", pprefix);
	VL53L1_print_histogram_bin_data(
		&(pdat->xtalk_hist_removed),
		ppre_text, trace_flags);
}


void VL53L1_print_range_data(
	VL53L1_range_data_t *pdat,
	char                *pprefix,
	uint32_t             trace_flags)
{


	char  fp_text[VL53L1_MAX_STRING_LENGTH];

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"range_id",
		pdat->range_id);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"time_stamp",
		pdat->time_stamp);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"VL53L1_p_015",
		pdat->VL53L1_p_015);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"VL53L1_p_022",
		pdat->VL53L1_p_022);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"VL53L1_p_025",
		pdat->VL53L1_p_025);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"VL53L1_p_026",
		pdat->VL53L1_p_026);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"VL53L1_p_016",
		pdat->VL53L1_p_016);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"VL53L1_p_027",
		pdat->VL53L1_p_027);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->width,
		 4,	VL53L1_MAX_STRING_LENGTH, fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"width",
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"VL53L1_p_030",
		pdat->VL53L1_p_030);



	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->fast_osc_frequency,
		12,	VL53L1_MAX_STRING_LENGTH, fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"fast_osc_frequency",
		fp_text);



	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->zero_distance_phase,
		11, VL53L1_MAX_STRING_LENGTH, fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"zero_distance_phase",
		fp_text);



	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->VL53L1_p_006,
		8, VL53L1_MAX_STRING_LENGTH, fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"actual_effective_spad",
		fp_text);


	trace_print(VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"total_periods_elapsed",
		pdat->total_periods_elapsed);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"peak_duration_us",
		pdat->peak_duration_us);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"woi_duration_us",
		pdat->woi_duration_us);

	trace_print(
			VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %d\n",
		pprefix,
			"VL53L1_p_020",
			pdat->VL53L1_p_020);

	trace_print(
			VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %d\n",
		pprefix,
			"VL53L1_p_021",
			pdat->VL53L1_p_021);

	trace_print(
			VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %d\n",
		pprefix,
			"VL53L1_p_013",
			pdat->VL53L1_p_013);



	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->peak_signal_count_rate_mcps,
		7, VL53L1_MAX_STRING_LENGTH, fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"peak_signal_count_rate_mcps",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->avg_signal_count_rate_mcps,
		7, VL53L1_MAX_STRING_LENGTH, fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"avg_signal_count_rate_mcps",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->ambient_count_rate_mcps,
		7, VL53L1_MAX_STRING_LENGTH, fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"ambient_count_rate_mcps",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->total_rate_per_spad_mcps,
		13, VL53L1_MAX_STRING_LENGTH, fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"total_rate_per_spad_mcps",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->VL53L1_p_012,
		11, VL53L1_MAX_STRING_LENGTH, fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"VL53L1_p_012",
		fp_text);



	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->VL53L1_p_005,
		 2,	VL53L1_MAX_STRING_LENGTH, fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"VL53L1_p_005",
		fp_text);



	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->VL53L1_p_028,
		11,	VL53L1_MAX_STRING_LENGTH, fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"VL53L1_p_028",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->VL53L1_p_014,
		11,	VL53L1_MAX_STRING_LENGTH, fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"VL53L1_p_014",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->VL53L1_p_029,
		11,	VL53L1_MAX_STRING_LENGTH, fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"VL53L1_p_029",
		fp_text);



	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %d\n",
		pprefix,
		"min_range_mm",
		pdat->min_range_mm);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %d\n",
		pprefix,
		"median_range_mm",
		pdat->median_range_mm);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %d\n",
		pprefix,
		"max_range_mm",
		pdat->max_range_mm);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"range_status",
		pdat->range_status);
}


void VL53L1_print_range_results(
	VL53L1_range_results_t *pdat,
	char                   *pprefix,
	uint32_t                trace_flags)
{


	char     pre_text[VL53L1_MAX_STRING_LENGTH];
	char    *ppre_text = &(pre_text[0]);

	uint8_t  i = 0;

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"cfg_device_state",
		pdat->cfg_device_state);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"rd_device_state",
		pdat->rd_device_state);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"zone_id",
		pdat->zone_id);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"stream_count",
		pdat->stream_count);

	for (i = 0; i < VL53L1_MAX_AMBIENT_DMAX_VALUES; i++) {
		sprintf(
			ppre_text,
			"%sambient_dmax_mm[%u]",
			pprefix, i);

		trace_print(
			VL53L1_TRACE_LEVEL_INFO,
			"%s = %u\n",
			ppre_text,
			pdat->VL53L1_p_007[i]);
	}

	trace_print(
			VL53L1_TRACE_LEVEL_INFO,
			"%s%s = %u\n",
			pprefix,
			"device_status",
			pdat->device_status);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"wrap_dmax_mm",
		pdat->wrap_dmax_mm);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"max_results",
		pdat->max_results);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"active_results",
		pdat->active_results);

	for (i = 0; i < pdat->active_results; i++) {
		sprintf(ppre_text, "%sdata[%u].", pprefix, i);
		VL53L1_print_range_data(
			&pdat->VL53L1_p_002[i],
			ppre_text, trace_flags);
	}

	sprintf(ppre_text, "%sxmonitor.", pprefix);
	VL53L1_print_range_data(
		&pdat->xmonitor,
		ppre_text, trace_flags);
}


void VL53L1_print_offset_range_results(
	VL53L1_offset_range_results_t *pdat,
	char                          *pprefix,
	uint32_t                       trace_flags)
{


	char  fp_text[VL53L1_MAX_STRING_LENGTH];
	char  pre_text[VL53L1_MAX_STRING_LENGTH];
	char *ppre_text = &(pre_text[0]);

	uint8_t  i = 0;

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"cal_distance_mm",
		pdat->cal_distance_mm);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->cal_reflectance_pc,
		2,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"cal_reflectance_pc",
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"cal_status",
		pdat->cal_status);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"cal_report",
		pdat->cal_report);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"max_results",
		pdat->max_results);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"active_results",
		pdat->active_results);

	for (i = 0; i < pdat->active_results; i++) {
		sprintf(ppre_text, "%sdata[%u].", pprefix, i);
		VL53L1_print_offset_range_data(
			&(pdat->VL53L1_p_002[i]),
			ppre_text, trace_flags);
	}
}


void VL53L1_print_offset_range_data(
	VL53L1_offset_range_data_t *pdat,
	char                       *pprefix,
	uint32_t                    trace_flags)
{


	char  fp_text[VL53L1_MAX_STRING_LENGTH];

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"preset_mode",
		pdat->preset_mode);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"dss_config__roi_mode_control",
		pdat->dss_config__roi_mode_control);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->dss_config__manual_effective_spads_select,
		8,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"dss_config__manual_effective_spads_select",
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"no_of_samples",
		pdat->no_of_samples);


	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->effective_spads,
		8,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"effective_spads",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->peak_rate_mcps,
		7,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"peak_rate_mcps",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->VL53L1_p_005,
		2,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"VL53L1_p_005",
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %d\n",
		pprefix,
		"median_range_mm",
		pdat->median_range_mm);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %d\n",
		pprefix,
		"range_mm_offset",
		pdat->range_mm_offset);
}


void VL53L1_print_cal_peak_rate_map(
	VL53L1_cal_peak_rate_map_t *pdat,
	char                       *pprefix,
	uint32_t                    trace_flags)
{


	char  fp_text[VL53L1_MAX_STRING_LENGTH];
	char  pre_text[VL53L1_MAX_STRING_LENGTH];
	char *ppre_text = &(pre_text[0]);

	uint8_t   i = 0;
	uint8_t   x = 0;
	uint8_t   y = 0;

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->cal_distance_mm,
		2,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"cal_distance_mm",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->cal_reflectance_pc,
		2,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"cal_reflectance_pc",
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"max_samples",
		pdat->max_samples);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"width",
		pdat->width);

	trace_print(
	VL53L1_TRACE_LEVEL_INFO,
	"%s%s = %u\n",
	pprefix,
	"height",
	pdat->height);

	i = 0;
	for (y = 0; y < pdat->height; y++) {
		for (x = 0; x < pdat->width; x++) {

			sprintf(ppre_text, "%speak_rate_mcps[%u]", pprefix, i);

			VL53L1_signed_fixed_point_sprintf(
				(int32_t)pdat->peak_rate_mcps[i],
				7,
				VL53L1_MAX_STRING_LENGTH,
				fp_text);

			trace_print(
				VL53L1_TRACE_LEVEL_INFO,
				"%s = %s\n",
				ppre_text,
				fp_text);

			i++;
		}
	}
}

void VL53L1_print_additional_data(
	VL53L1_additional_data_t *pdat,
	char                     *pprefix,
	uint32_t                 trace_flags)
{



	char  fp_text[VL53L1_MAX_STRING_LENGTH];
	char  pre_text[VL53L1_MAX_STRING_LENGTH];
	char *ppre_text = &(pre_text[0]);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"preset_mode",
		pdat->preset_mode);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"zone_preset",
		pdat->zone_preset);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"measurement_mode",
		pdat->measurement_mode);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"offset_calibration_mode",
		pdat->offset_calibration_mode);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"offset_correction_mode",
		pdat->offset_correction_mode);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"dmax_mode",
		pdat->dmax_mode);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"phasecal_config_timeout_us",
		pdat->phasecal_config_timeout_us);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"mm_config_timeout_us",
		pdat->mm_config_timeout_us);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"range_config_timeout_us",
		pdat->range_config_timeout_us);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"inter_measurement_period_ms",
		pdat->inter_measurement_period_ms);


	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->dss_config__target_total_rate_mcps,
		7,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"dss_config__target_total_rate_mcps",
		fp_text);

	sprintf(ppre_text, "%s VL53L1_p_010.", pprefix);
		VL53L1_print_histogram_bin_data(
			&pdat->VL53L1_p_010,
			ppre_text, trace_flags);


}


void VL53L1_print_additional_offset_cal_data(
	VL53L1_additional_offset_cal_data_t *pdat,
	char                                *pprefix,
	uint32_t                             trace_flags)
{


	char  fp_text[VL53L1_MAX_STRING_LENGTH];

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->result__mm_inner_actual_effective_spads,
		8,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"result__mm_inner_actual_effective_spads",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->result__mm_outer_actual_effective_spads,
		8,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"result__mm_outer_actual_effective_spads",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->result__mm_inner_peak_signal_count_rtn_mcps,
		7,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"result__mm_inner_peak_signal_count_rtn_mcps",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->result__mm_outer_peak_signal_count_rtn_mcps,
		7,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"result__mm_outer_peak_signal_count_rtn_mcps",
		fp_text);
}


void VL53L1_print_gain_calibration_data(
	VL53L1_gain_calibration_data_t *pdat,
	char                           *pprefix,
	uint32_t                        trace_flags)
{


	char  fp_text[VL53L1_MAX_STRING_LENGTH];

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->standard_ranging_gain_factor,
		11,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"standard_ranging_gain_factor",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->histogram_ranging_gain_factor,
		11,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"histogram_ranging_gain_factor",
		fp_text);
}


void VL53L1_print_zone_calibration_data(
	VL53L1_zone_calibration_data_t *pdat,
	char                           *pprefix,
	uint32_t                        trace_flags)
{


	char  fp_text[VL53L1_MAX_STRING_LENGTH];

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"no_of_samples",
		pdat->no_of_samples);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->effective_spads,
		8,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"effective_spads",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->peak_rate_mcps,
		7,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"peak_rate_mcps",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->VL53L1_p_014,
		11,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"VL53L1_p_014",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->VL53L1_p_005,
		2,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"VL53L1_p_005",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->median_range_mm,
		2,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"median_range_mm",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->range_mm_offset,
		2,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"range_mm_offset",
		fp_text);
}


void VL53L1_print_zone_calibration_results(
	VL53L1_zone_calibration_results_t *pdat,
	char                              *pprefix,
	uint32_t                           trace_flags)
{


	char  fp_text[VL53L1_MAX_STRING_LENGTH];
	char  pre_text[VL53L1_MAX_STRING_LENGTH];
	char *ppre_text = &(pre_text[0]);

	uint8_t  i = 0;

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"preset_mode",
		pdat->preset_mode);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"zone_preset",
		pdat->zone_preset);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"cal_distance_mm",
		pdat->cal_distance_mm);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->cal_reflectance_pc,
		2,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"cal_reflectance_pc",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->phasecal_result__reference_phase,
		11,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"phasecal_result__reference_phase",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->zero_distance_phase,
		11,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"zero_distance_phase",
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"cal_status",
		pdat->cal_status);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"max_zones",
		pdat->max_zones);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"active_zones",
		pdat->active_zones);

	for (i = 0; i < pdat->active_zones; i++) {
		sprintf(ppre_text, "%sdata[%u].", pprefix, i);
		VL53L1_print_zone_calibration_data(
			&(pdat->VL53L1_p_002[i]),
			ppre_text, trace_flags);
	}
}

void VL53L1_print_xtalk_range_results(
	VL53L1_xtalk_range_results_t *pdat,
	char                         *pprefix,
	uint32_t                      trace_flags)
{


	char  pre_text[VL53L1_MAX_STRING_LENGTH];
	char *ppre_text = &(pre_text[0]);
	uint8_t  i = 0;

	VL53L1_histogram_bin_data_t *pbin_data;

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"cal_status",
		pdat->cal_status);

	trace_print(
			VL53L1_TRACE_LEVEL_INFO,
			"%s%s = %u\n",
			pprefix,
			"num_of_samples_status",
			pdat->num_of_samples_status);

	trace_print(
			VL53L1_TRACE_LEVEL_INFO,
			"%s%s = %u\n",
			pprefix,
			"zero_samples_status",
			pdat->zero_samples_status);

	trace_print(
			VL53L1_TRACE_LEVEL_INFO,
			"%s%s = %u\n",
			pprefix,
			"max_sigma_status",
			pdat->max_sigma_status);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"max_results",
		pdat->max_results);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"active_results",
		pdat->active_results);

	for (i = 0; i < pdat->active_results; i++) {
		sprintf(ppre_text, "%sdata[%u].", pprefix, i);
		VL53L1_print_xtalk_range_data(
			&(pdat->VL53L1_p_002[i]),
			ppre_text, trace_flags);
	}

	sprintf(ppre_text, "%scentral_histogram_sum.", pprefix);
	VL53L1_print_histogram_bin_data(
		&pdat->central_histogram_sum,
		ppre_text, trace_flags);

	sprintf(ppre_text, "%scentral_histogram_avg.", pprefix);
	VL53L1_print_histogram_bin_data(
		&pdat->central_histogram_avg,
		ppre_text, trace_flags);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"VL53L1_p_015",
		pdat->central_histogram__window_start);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"VL53L1_p_016",
		pdat->central_histogram__window_end);

	pbin_data = &(pdat->histogram_avg_1[0]);

	for (i = 0; i < 5; i++) {
		sprintf(ppre_text, "%shistogram_avg_1[%u].", pprefix, i);
		VL53L1_print_histogram_bin_data(
			pbin_data,
			ppre_text, trace_flags);
			pbin_data++;
	}

	pbin_data = &(pdat->histogram_avg_2[0]);

	for (i = 0; i < 5; i++) {
		sprintf(ppre_text, "%shistogram_avg_2[%u].", pprefix, i);
		VL53L1_print_histogram_bin_data(
			pbin_data,
			ppre_text, trace_flags);
			pbin_data++;
	}

	pbin_data = &(pdat->xtalk_avg[0]);

	for (i = 0; i < 5; i++) {
		sprintf(ppre_text, "%sxtalk_avg[%u].", pprefix, i);
		VL53L1_print_histogram_bin_data(
			pbin_data,
			ppre_text, trace_flags);
			pbin_data++;
	}
}


void VL53L1_print_xtalk_range_data(
	VL53L1_xtalk_range_data_t *pdat,
	char                      *pprefix,
	uint32_t                   trace_flags)
{


	char  fp_text[VL53L1_MAX_STRING_LENGTH];

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"no_of_samples",
		pdat->no_of_samples);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %d\n",
		pprefix,
		"signal_total_events_sum",
		pdat->signal_total_events_sum);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %d\n",
		pprefix,
		"signal_total_events_avg",
		pdat->signal_total_events_avg);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->rate_per_spad_kcps_sum,
		11,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"rate_per_spad_kcps_sum",
		fp_text);


	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->rate_per_spad_kcps_avg,
		11,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"rate_per_spad_kcps_avg",
		fp_text);
}


void VL53L1_print_xtalk_calibration_results(
	VL53L1_xtalk_calibration_results_t *pdat,
	char                               *pprefix,
	uint32_t                            trace_flags)
{


	char  fp_text[VL53L1_MAX_STRING_LENGTH];
	int16_t tmpi16;

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->algo__crosstalk_compensation_plane_offset_kcps,
		9,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"algo__crosstalk_compensation_plane_offset_kcps",
		fp_text);

	tmpi16 = pdat->algo__crosstalk_compensation_x_plane_gradient_kcps;
	VL53L1_signed_fixed_point_sprintf(
		(int32_t)tmpi16,
		11,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"algo__crosstalk_compensation_x_plane_gradient_kcps",
		fp_text);

	tmpi16 = pdat->algo__crosstalk_compensation_y_plane_gradient_kcps;
	VL53L1_signed_fixed_point_sprintf(
		(int32_t)tmpi16,
		11,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"algo__crosstalk_compensation_y_plane_gradient_kcps",
		fp_text);
}


void VL53L1_print_xtalk_config(
	VL53L1_xtalk_config_t *pdat,
	char                  *pprefix,
	uint32_t               trace_flags)
{


	char  fp_text[VL53L1_MAX_STRING_LENGTH];
	int16_t tmpi16;

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->algo__crosstalk_compensation_plane_offset_kcps,
		9,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"algo__crosstalk_compensation_plane_offset_kcps",
		fp_text);

	tmpi16 = pdat->algo__crosstalk_compensation_x_plane_gradient_kcps;
	VL53L1_signed_fixed_point_sprintf(
		(int32_t)tmpi16,
		11,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"algo__crosstalk_compensation_x_plane_gradient_kcps",
		fp_text);

	tmpi16 = pdat->algo__crosstalk_compensation_y_plane_gradient_kcps;
	VL53L1_signed_fixed_point_sprintf(
		(int32_t)tmpi16,
		11,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"algo__crosstalk_compensation_y_plane_gradient_kcps",
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_crosstalk_compensation_enable",
		pdat->global_crosstalk_compensation_enable);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->histogram_mode_crosstalk_margin_kcps,
		9,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"histogram_mode_crosstalk_margin_kcps",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->lite_mode_crosstalk_margin_kcps,
		9,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"lite_mode_crosstalk_margin_kcps",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->crosstalk_range_ignore_threshold_mult,
		5,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"crosstalk_range_ignore_threshold_mult",
		fp_text);


	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->crosstalk_range_ignore_threshold_rate_mcps,
		13,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"crosstalk_range_ignore_threshold_rate_mcps",
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"algo__crosstalk_detect_max_valid_range_mm",
		pdat->algo__crosstalk_detect_max_valid_range_mm);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"algo__crosstalk_detect_min_valid_range_mm",
		pdat->algo__crosstalk_detect_min_valid_range_mm);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->algo__crosstalk_detect_max_valid_rate_kcps,
		7,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"algo__crosstalk_detect_max_valid_rate_kcps",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->algo__crosstalk_detect_max_sigma_mm,
		2,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"algo__crosstalk_detect_max_sigma_mm",
		fp_text);

}


void VL53L1_print_xtalk_extract_config(
	VL53L1_xtalkextract_config_t *pdat,
	char                         *pprefix,
	uint32_t                      trace_flags)
{


	char  fp_text[VL53L1_MAX_STRING_LENGTH];


	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->dss_config__target_total_rate_mcps,
		7,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"dss_config__target_total_rate_mcps",
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"mm_config_timeout_us",
		pdat->mm_config_timeout_us);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"range_config_timeout_us",
		pdat->range_config_timeout_us);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"num_of_samples",
		pdat->num_of_samples);


	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"algo__crosstalk_extract_max_valid_range_mm",
		pdat->algo__crosstalk_extract_max_valid_range_mm);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"algo__crosstalk_extract_min_valid_range_mm",
		pdat->algo__crosstalk_extract_min_valid_range_mm);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->algo__crosstalk_extract_max_valid_rate_kcps,
		9,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"algo__crosstalk_extract_max_valid_rate_kcps",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->algo__crosstalk_extract_max_sigma_mm,
		2,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"algo__crosstalk_extract_max_sigma_mm",
		fp_text);

}


void VL53L1_print_zone_cal_config(
	VL53L1_zonecal_config_t *pdat,
	char                    *pprefix,
	uint32_t                 trace_flags)
{


	char  fp_text[VL53L1_MAX_STRING_LENGTH];

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->dss_config__target_total_rate_mcps,
		7,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"dss_config__target_total_rate_mcps",
		fp_text);


	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"mm_config_timeout_us",
		pdat->mm_config_timeout_us);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"range_config_timeout_us",
		pdat->range_config_timeout_us);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"phasecal_config_timeout_us",
		pdat->phasecal_config_timeout_us);


	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"phasecal_num_of_samples",
		pdat->phasecal_num_of_samples);


	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"zone_num_of_samples",
		pdat->zone_num_of_samples);

}

void VL53L1_print_offset_cal_config(
	VL53L1_offsetcal_config_t *pdat,
	char                      *pprefix,
	uint32_t                   trace_flags)
{


	char  fp_text[VL53L1_MAX_STRING_LENGTH];

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->dss_config__target_total_rate_mcps,
		7,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"dss_config__target_total_rate_mcps",
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"phasecal_config_timeout_us",
		pdat->phasecal_config_timeout_us);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"range_config_timeout_us",
		pdat->range_config_timeout_us);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"pre_num_of_samples",
		pdat->pre_num_of_samples);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"mm1_num_of_samples",
		pdat->mm1_num_of_samples);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"mm2_num_of_samples",
		pdat->mm2_num_of_samples);


}


void VL53L1_print_dmax_calibration_data(
	VL53L1_dmax_calibration_data_t *pdat,
	char                           *pprefix,
	uint32_t                        trace_flags)
{


	char  fp_text[VL53L1_MAX_STRING_LENGTH];

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->ref__actual_effective_spads,
		8,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"ref__actual_effective_spads",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->ref__peak_signal_count_rate_mcps,
		7,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"ref__peak_signal_count_rate_mcps",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->ref__distance_mm,
		4,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"ref__distance_mm",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->ref_reflectance_pc,
		2,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"ref_reflectance_pc",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->coverglass_transmission,
		8,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"coverglass_transmission",
		fp_text);
}


void VL53L1_print_calibration_data(
	VL53L1_calibration_data_t *pdat,
	char                      *pprefix,
	uint32_t                   trace_flags)
{


	char  pre_text[VL53L1_MAX_STRING_LENGTH];
	char *ppre_text = &(pre_text[0]);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = 0x%08X\n",
		pprefix,
		"struct_version",
		pdat->struct_version);

	sprintf(ppre_text, "%scustomer.", pprefix);
	VL53L1_print_customer_nvm_managed(
		&(pdat->customer),
		ppre_text, trace_flags);

	sprintf(ppre_text, "%sfmt_dmax_cal.", pprefix);
	VL53L1_print_dmax_calibration_data(
		&(pdat->fmt_dmax_cal),
		ppre_text, trace_flags);

	sprintf(ppre_text, "%scust_dmax_cal.", pprefix);
	VL53L1_print_dmax_calibration_data(
		&(pdat->cust_dmax_cal),
		ppre_text, trace_flags);

	sprintf(ppre_text, "%sadd_off_cal_data.", pprefix);
	VL53L1_print_additional_offset_cal_data(
		&(pdat->add_off_cal_data),
		ppre_text, trace_flags);

	sprintf(ppre_text, "%soptical_centre.", pprefix);
	VL53L1_print_optical_centre(
		&(pdat->optical_centre),
		ppre_text, trace_flags);

	sprintf(ppre_text, "%sxtalkhisto.", pprefix);
	VL53L1_print_xtalk_histogram_data(
		&(pdat->xtalkhisto),
		ppre_text, trace_flags);

	sprintf(ppre_text, "%sgain_cal.", pprefix);
	VL53L1_print_gain_calibration_data(
		&(pdat->gain_cal),
		ppre_text, trace_flags);

	sprintf(ppre_text, "%scal_peak_rate_map.", pprefix);
	VL53L1_print_cal_peak_rate_map(
		&(pdat->cal_peak_rate_map),
		ppre_text, trace_flags);
}


void VL53L1_print_xtalk_debug_data(
	VL53L1_xtalk_debug_data_t *pdat,
	char                      *pprefix,
	uint32_t                   trace_flags)
{


	char  pre_text[VL53L1_MAX_STRING_LENGTH];
	char *ppre_text = &(pre_text[0]);

	sprintf(ppre_text, "%scustomer.", pprefix);
	VL53L1_print_customer_nvm_managed(
		&(pdat->customer),
		ppre_text, trace_flags);

	sprintf(ppre_text, "%sxtalk_cfg.", pprefix);
	VL53L1_print_xtalk_config(
		&(pdat->xtalk_cfg),
		ppre_text, trace_flags);

	sprintf(ppre_text, "%sxtalk_extract_cfg.", pprefix);
	VL53L1_print_xtalk_extract_config(
		&(pdat->xtalk_extract_cfg),
		ppre_text, trace_flags);

	sprintf(ppre_text, "%shist_data.", pprefix);
	VL53L1_print_histogram_bin_data(
		&(pdat->hist_data),
		ppre_text, trace_flags);

	sprintf(ppre_text, "%sxtalk_shapes.", pprefix);
	VL53L1_print_xtalk_histogram_data(
		&(pdat->xtalk_shapes),
		ppre_text, trace_flags);

	sprintf(ppre_text, "%sgain_cal.", pprefix);
	VL53L1_print_xtalk_range_results(
		&(pdat->xtalk_results),
		ppre_text, trace_flags);
}


void VL53L1_print_offset_debug_data(
	VL53L1_offset_debug_data_t *pdat,
	char                       *pprefix,
	uint32_t                    trace_flags)
{


	char  pre_text[VL53L1_MAX_STRING_LENGTH];
	char *ppre_text = &(pre_text[0]);

	sprintf(ppre_text, "%scustomer.", pprefix);
	VL53L1_print_customer_nvm_managed(
		&(pdat->customer),
		ppre_text, trace_flags);

	sprintf(ppre_text, "%sfmt_dmax_cal.", pprefix);
	VL53L1_print_dmax_calibration_data(
		&(pdat->fmt_dmax_cal),
		ppre_text, trace_flags);

	sprintf(ppre_text, "%scust_dmax_cal.", pprefix);
	VL53L1_print_dmax_calibration_data(
		&(pdat->cust_dmax_cal),
		ppre_text, trace_flags);

	sprintf(ppre_text, "%sadd_off_cal_data.", pprefix);
	VL53L1_print_additional_offset_cal_data(
		&(pdat->add_off_cal_data),
		ppre_text, trace_flags);

	sprintf(ppre_text, "%soffset_results.", pprefix);
	VL53L1_print_offset_range_results(
		&(pdat->offset_results),
		ppre_text, trace_flags);
}


void VL53L1_print_zone_config(
	VL53L1_zone_config_t *pdat,
	char                 *pprefix,
	uint32_t              trace_flags)
{



	char  pre_text[VL53L1_MAX_STRING_LENGTH];
	char *ppre_text = &(pre_text[0]);

	uint8_t  i = 0;

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"max_zones",
		pdat->max_zones);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"active_zones",
		pdat->active_zones);

	for (i = 0; i < pdat->active_zones; i++) {
		sprintf(ppre_text, "%suser_zones[%u].", pprefix, i);
		VL53L1_print_user_zone(
			&pdat->user_zones[i],
			ppre_text,
			trace_flags);
	}
}


void VL53L1_print_optical_centre(
	VL53L1_optical_centre_t  *pdat,
	char                     *pprefix,
	uint32_t                  trace_flags)
{



	char  fp_text[VL53L1_MAX_STRING_LENGTH];

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->x_centre,
		4,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"x_centre",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->y_centre,
		4,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"y_centre",
		fp_text);
}


void VL53L1_print_user_zone(
	VL53L1_user_zone_t   *pdat,
	char                 *pprefix,
	uint32_t              trace_flags)
{



	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"x_centre",
		pdat->x_centre);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"y_centre",
		pdat->y_centre);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"width",
		pdat->width);

	trace_print(VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"height",
		pdat->height);
}


void VL53L1_print_spad_rate_data(
	VL53L1_spad_rate_data_t  *pspad_rates,
	char                     *pprefix,
	uint32_t                  trace_flags)
{



	uint16_t spad_no = 0;
	uint8_t  row     = 0;
	uint8_t  col     = 0;

	char  fp_text[VL53L1_MAX_STRING_LENGTH];

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%8s,%4s,%4s, %s\n",
		pprefix,
		"spad_no",
		"row",
		"col",
		"peak_rate_mcps");

	for (spad_no = 0; spad_no < pspad_rates->no_of_values; spad_no++) {


		VL53L1_decode_row_col(
			(uint8_t)spad_no,
			&row,
			&col);



		VL53L1_signed_fixed_point_sprintf(
			(int32_t)pspad_rates->rate_data[spad_no],
			pspad_rates->fractional_bits,
			VL53L1_MAX_STRING_LENGTH,
			fp_text);



		trace_print(
			VL53L1_TRACE_LEVEL_INFO,
			"%s%8u,%4u,%4u, %s\n",
			pprefix,
			spad_no,
			row,
			col,
			fp_text);
	}
}


void VL53L1_print_spad_rate_map(
	VL53L1_spad_rate_data_t  *pspad_rates,
	char                     *pprefix,
	uint32_t                  trace_flags)
{



	uint8_t  spad_no = 0;
	uint8_t  row     = 0;
	uint8_t  col     = 0;

	char  fp_text[VL53L1_MAX_STRING_LENGTH];


	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%4s",
		pprefix,
		" ");

	for (col = 0;  col < VL53L1_SPAD_ARRAY_WIDTH; col++)
		trace_print(
			VL53L1_TRACE_LEVEL_INFO,
			",%8u",
			col);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"\n");



	for (row = 0;  row < VL53L1_SPAD_ARRAY_HEIGHT; row++) {

		trace_print(
			VL53L1_TRACE_LEVEL_INFO,
			"%s%4u",
			pprefix,
			row);

		for (col = 0;  col < VL53L1_SPAD_ARRAY_HEIGHT; col++) {



			VL53L1_encode_row_col(
				row,
				col,
				&spad_no);



			VL53L1_signed_fixed_point_sprintf(
				(int32_t)pspad_rates->rate_data[spad_no],
				pspad_rates->fractional_bits,
				VL53L1_MAX_STRING_LENGTH,
				fp_text);



			trace_print(
				VL53L1_TRACE_LEVEL_INFO,
				",%8s",
				fp_text);
		}

		trace_print(
			VL53L1_TRACE_LEVEL_INFO,
			"\n");
	}
}


#endif


