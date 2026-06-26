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
#include "vl53l1_register_map.h"
#include "vl53l1_api_debug.h"
#include "vl53l1_nvm_structs.h"
#include "vl53l1_nvm_debug.h"

// #undef LOG_FUNCTION_START
// #define LOG_FUNCTION_START(fmt, ...) \
// 	_LOG_FUNCTION_START(VL53L1_TRACE_MODULE_NVM, fmt, __VA_ARGS__)
// #undef LOG_FUNCTION_END
// #define LOG_FUNCTION_END(status, ...) \
// 	_LOG_FUNCTION_END(VL53L1_TRACE_MODULE_NVM, status, __VA_ARGS__)
// #undef LOG_FUNCTION_END_FMT
// #define LOG_FUNCTION_END_FMT(status, fmt, ...) \
// 	_LOG_FUNCTION_END_FMT(VL53L1_TRACE_MODULE_NVM,\
// 		status, fmt, __VA_ARGS__)

// #define trace_print(level, ...) \
// 	_LOG_TRACE_PRINT(trace_flags, \
// 	level, VL53L1_TRACE_FUNCTION_NONE, __VA_ARGS__)

#ifdef VL53L1_LOG_ENABLE

void VL53L1_print_nvm_raw_data(
	uint8_t                       *pnvm_raw_data,
	uint32_t                       trace_flags)
{


	int i = 0;

	LOG_FUNCTION_START("");

	for (i = 0 ; i < VL53L1_NVM_SIZE_IN_BYTES ; i++) {
		if (i % 4 == 0)
			trace_print(
				VL53L1_TRACE_LEVEL_INFO,
				"\n    NVM Addr 0x%02X : 0x",
				i/4);

		trace_print(
			VL53L1_TRACE_LEVEL_INFO,
			"%02X",
			*pnvm_raw_data++);
	}

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"\n");

	LOG_FUNCTION_END(0);
}


void VL53L1_print_decoded_nvm_data(
	VL53L1_decoded_nvm_data_t *pdat,
	char                      *pprefix,
	uint32_t                   trace_flags)
{
	char  fp_text[VL53L1_MAX_STRING_LENGTH];
	char  pre_text[VL53L1_MAX_STRING_LENGTH];
	char *ppre_text = &(pre_text[0]);

	uint8_t i = 0;

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__identification_model_id",
		pdat->nvm__identification_model_id);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__identification_module_type",
		pdat->nvm__identification_module_type);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__identification_revision_id",
		pdat->nvm__identification_revision_id);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__identification_module_id",
		pdat->nvm__identification_module_id);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__i2c_valid",
		pdat->nvm__i2c_valid);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__i2c_device_address_ews",
		pdat->nvm__i2c_device_address_ews);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->nvm__ews__fast_osc_frequency,
		12,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"nvm__ews__fast_osc_frequency",
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__ews__fast_osc_trim_max",
		pdat->nvm__ews__fast_osc_trim_max);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__ews__fast_osc_freq_set",
		pdat->nvm__ews__fast_osc_freq_set);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__ews__slow_osc_calibration",
		pdat->nvm__ews__slow_osc_calibration);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->nvm__fmt__fast_osc_frequency,
		12,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"nvm__fmt__fast_osc_frequency",
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__fmt__fast_osc_trim_max",
		pdat->nvm__fmt__fast_osc_trim_max);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__fmt__fast_osc_freq_set",
		pdat->nvm__fmt__fast_osc_freq_set);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__fmt__slow_osc_calibration",
		pdat->nvm__fmt__slow_osc_calibration);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__vhv_config_unlock",
		pdat->nvm__vhv_config_unlock);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__ref_selvddpix",
		pdat->nvm__ref_selvddpix);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__ref_selvquench",
		pdat->nvm__ref_selvquench);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__regavdd1v2_sel",
		pdat->nvm__regavdd1v2_sel);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__regdvdd1v2_sel",
		pdat->nvm__regdvdd1v2_sel);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__vhv_timeout__macrop",
		pdat->nvm__vhv_timeout__macrop);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__vhv_loop_bound",
		pdat->nvm__vhv_loop_bound);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__vhv_count_threshold",
		pdat->nvm__vhv_count_threshold);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__vhv_offset",
		pdat->nvm__vhv_offset);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__vhv_init_enable",
		pdat->nvm__vhv_init_enable);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__vhv_init_value",
		pdat->nvm__vhv_init_value);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__laser_safety_vcsel_trim_ll",
		pdat->nvm__laser_safety_vcsel_trim_ll);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__laser_safety_vcsel_selion_ll",
		pdat->nvm__laser_safety_vcsel_selion_ll);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__laser_safety_vcsel_selion_max_ll",
		pdat->nvm__laser_safety_vcsel_selion_max_ll);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__laser_safety_mult_ll",
		pdat->nvm__laser_safety_mult_ll);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__laser_safety_clip_ll",
		pdat->nvm__laser_safety_clip_ll);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__laser_safety_vcsel_trim_ld",
		pdat->nvm__laser_safety_vcsel_trim_ld);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__laser_safety_vcsel_selion_ld",
		pdat->nvm__laser_safety_vcsel_selion_ld);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__laser_safety_vcsel_selion_max_ld",
		pdat->nvm__laser_safety_vcsel_selion_max_ld);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__laser_safety_mult_ld",
		pdat->nvm__laser_safety_mult_ld);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__laser_safety_clip_ld",
		pdat->nvm__laser_safety_clip_ld);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__laser_safety_lock_byte",
		pdat->nvm__laser_safety_lock_byte);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__laser_safety_unlock_byte",
		pdat->nvm__laser_safety_unlock_byte);



	for (i = 0 ; i < VL53L1_RTN_SPAD_BUFFER_SIZE ; i++) {
		sprintf(
			ppre_text,
			"%snvm__ews__spad_enables_rtn[%u]",
			pprefix, i);

		trace_print(
			VL53L1_TRACE_LEVEL_INFO,
			"%s = %u\n",
			ppre_text,
			pdat->nvm__ews__spad_enables_rtn[i]);
	}

	for (i = 0 ; i < VL53L1_REF_SPAD_BUFFER_SIZE ; i++) {
		sprintf(
			ppre_text,
			"%snvm__ews__spad_enables_ref__loc1[%u]",
			pprefix, i);

		trace_print(
			VL53L1_TRACE_LEVEL_INFO,
			"%s = %u\n",
			ppre_text,
			pdat->nvm__ews__spad_enables_ref__loc1[i]);
	}

	for (i = 0 ; i < VL53L1_REF_SPAD_BUFFER_SIZE ; i++) {
		sprintf(
			ppre_text,
			"%snvm__ews__spad_enables_ref__loc2[%u]",
			pprefix, i);

		trace_print(
			VL53L1_TRACE_LEVEL_INFO,
			"%s = %u\n",
			ppre_text,
			pdat->nvm__ews__spad_enables_ref__loc2[i]);
	}

	for (i = 0 ; i < VL53L1_REF_SPAD_BUFFER_SIZE ; i++) {
		sprintf(
			ppre_text,
			"%snvm__ews__spad_enables_ref__loc3[%u]",
			pprefix, i);

		trace_print(
			VL53L1_TRACE_LEVEL_INFO,
			"%s = %u\n",
			ppre_text,
			pdat->nvm__ews__spad_enables_ref__loc3[i]);
	}




	for (i = 0 ; i < VL53L1_RTN_SPAD_BUFFER_SIZE ; i++) {
		sprintf(
			ppre_text,
			"%snvm__fmt__spad_enables_rtn[%u]",
			pprefix, i);

		trace_print(
			VL53L1_TRACE_LEVEL_INFO,
			"%s = %u\n",
			ppre_text,
			pdat->nvm__fmt__spad_enables_rtn[i]);
	}

	for (i = 0 ; i < VL53L1_REF_SPAD_BUFFER_SIZE ; i++) {
		sprintf(
			ppre_text,
			"%snvm__fmt__spad_enables_ref__loc1[%u]",
			pprefix, i);

		trace_print(
			VL53L1_TRACE_LEVEL_INFO,
			"%s = %u\n",
			ppre_text,
			pdat->nvm__fmt__spad_enables_ref__loc1[i]);
	}

	for (i = 0 ; i < VL53L1_REF_SPAD_BUFFER_SIZE ; i++) {
		sprintf(
			ppre_text,
			"%snvm__fmt__spad_enables_ref__loc2[%u]",
			pprefix, i);

		trace_print(
			VL53L1_TRACE_LEVEL_INFO,
			"%s = %u\n",
			ppre_text,
			pdat->nvm__fmt__spad_enables_ref__loc2[i]);
	}

	for (i = 0 ; i < VL53L1_REF_SPAD_BUFFER_SIZE ; i++) {
		sprintf(
			ppre_text,
			"%snvm__fmt__spad_enables_ref__loc3[%u]",
			pprefix, i);

		trace_print(
			VL53L1_TRACE_LEVEL_INFO,
			"%s = %u\n",
			ppre_text,
			pdat->nvm__fmt__spad_enables_ref__loc3[i]);
	}

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__fmt__roi_config__mode_roi_centre_spad",
		pdat->nvm__fmt__roi_config__mode_roi_centre_spad);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__fmt__roi_config__mode_roi_x_size",
		pdat->nvm__fmt__roi_config__mode_roi_x_size);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__fmt__roi_config__mode_roi_y_size",
		pdat->nvm__fmt__roi_config__mode_roi_y_size);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__fmt__ref_spad_apply__num_requested_ref_spad",
		pdat->nvm__fmt__ref_spad_apply__num_requested_ref_spad);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__fmt__ref_spad_man__ref_location",
		pdat->nvm__fmt__ref_spad_man__ref_location);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %d\n",
		pprefix,
		"nvm__fmt__mm_config__inner_offset_mm",
		pdat->nvm__fmt__mm_config__inner_offset_mm);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %d\n",
		pprefix,
		"nvm__fmt__mm_config__outer_offset_mm",
		pdat->nvm__fmt__mm_config__outer_offset_mm);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->nvm__fmt__algo_part_to_part_range_offset_mm,
		2,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"nvm__fmt__algo_part_to_part_range_offset_mm",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
	(int32_t)(
	pdat->nvm__fmt__algo__crosstalk_compensation_plane_offset_kcps),
	9,
	VL53L1_MAX_STRING_LENGTH,
	fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"nvm__fmt__algo__crosstalk_compensation_plane_offset_kcps",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
	(int32_t)(
	pdat->nvm__fmt__algo__crosstalk_compensation_x_plane_gradient_kcps),
	11,
	VL53L1_MAX_STRING_LENGTH,
	fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"nvm__fmt__algo__crosstalk_compensation_x_plane_gradient_kcps",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
	(int32_t)(
	pdat->nvm__fmt__algo__crosstalk_compensation_y_plane_gradient_kcps),
	11,
	VL53L1_MAX_STRING_LENGTH,
	fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"nvm__fmt__algo__crosstalk_compensation_y_plane_gradient_kcps",
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__fmt__spare__host_config__nvm_config_spare_0",
		pdat->nvm__fmt__spare__host_config__nvm_config_spare_0);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__fmt__spare__host_config__nvm_config_spare_1",
		pdat->nvm__fmt__spare__host_config__nvm_config_spare_1);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__customer_space_programmed",
		pdat->nvm__customer_space_programmed);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__cust__i2c_device_address",
		pdat->nvm__cust__i2c_device_address);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__cust__ref_spad_apply__num_requested_ref_spad",
		pdat->nvm__cust__ref_spad_apply__num_requested_ref_spad);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__cust__ref_spad_man__ref_location",
		pdat->nvm__cust__ref_spad_man__ref_location);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %d\n",
		pprefix,
		"nvm__cust__mm_config__inner_offset_mm",
		pdat->nvm__cust__mm_config__inner_offset_mm);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %d\n",
		pprefix,
		"nvm__cust__mm_config__outer_offset_mm",
		pdat->nvm__cust__mm_config__outer_offset_mm);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->nvm__cust__algo_part_to_part_range_offset_mm,
		2,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"nvm__cust__algo_part_to_part_range_offset_mm",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
	(uint32_t)(
	pdat->nvm__cust__algo__crosstalk_compensation_plane_offset_kcps),
	9,
	VL53L1_MAX_STRING_LENGTH,
	fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"nvm__cust__algo__crosstalk_compensation_plane_offset_kcps",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
	(int32_t)(
	pdat->nvm__cust__algo__crosstalk_compensation_x_plane_gradient_kcps),
	11,
	VL53L1_MAX_STRING_LENGTH,
	fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"nvm__cust__algo__crosstalk_compensation_x_plane_gradient_kcps",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
	(int32_t)(
	pdat->nvm__cust__algo__crosstalk_compensation_y_plane_gradient_kcps),
	11,
	VL53L1_MAX_STRING_LENGTH,
	fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"nvm__cust__algo__crosstalk_compensation_y_plane_gradient_kcps",
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__cust__spare__host_config__nvm_config_spare_0",
		pdat->nvm__cust__spare__host_config__nvm_config_spare_0);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__cust__spare__host_config__nvm_config_spare_1",
		pdat->nvm__cust__spare__host_config__nvm_config_spare_1);



	sprintf(
		ppre_text,
		"%sfmt_optical_centre.", pprefix);

	VL53L1_print_optical_centre(
		&(pdat->fmt_optical_centre),
		ppre_text,
		VL53L1_TRACE_MODULE_NVM_DATA);



	sprintf(
		ppre_text,
		"%sfmt_peak_rate_map.", pprefix);

	VL53L1_print_cal_peak_rate_map(
		&(pdat->fmt_peak_rate_map),
		ppre_text,
		VL53L1_TRACE_MODULE_NVM_DATA);



	sprintf(
		ppre_text,
		"%sfmt_add_offset_data.",
		pprefix);

	VL53L1_print_additional_offset_cal_data(
		&(pdat->fmt_add_offset_data),
		ppre_text,
		VL53L1_TRACE_MODULE_NVM_DATA);



	for (i = 0 ; i < VL53L1_NVM_MAX_FMT_RANGE_DATA ; i++) {
		sprintf(
			ppre_text,
			"%sfmt_range_data[%u].",
			pprefix, i);

		VL53L1_print_decoded_nvm_fmt_range_data(
			&(pdat->fmt_range_data[i]),
			ppre_text,
			trace_flags);
	}

	sprintf(
		ppre_text,
		"%sfmt_info.",
		pprefix);

	VL53L1_print_decoded_nvm_fmt_info(
		&(pdat->fmt_info),
		ppre_text,
		trace_flags);

	sprintf(
		ppre_text,
		"%sews_info.",
		pprefix);

	VL53L1_print_decoded_nvm_ews_info(
		&(pdat->ews_info),
		ppre_text,
		trace_flags);
}


void VL53L1_print_decoded_nvm_fmt_range_data(
	VL53L1_decoded_nvm_fmt_range_data_t *pdat,
	char                                *pprefix,
	uint32_t                             trace_flags)
{
	char  fp_text[VL53L1_MAX_STRING_LENGTH];

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->result__actual_effective_rtn_spads,
		8,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"result__actual_effective_rtn_spads",
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"ref_spad_array__num_requested_ref_spads",
		pdat->ref_spad_array__num_requested_ref_spads);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"ref_spad_array__ref_location",
		pdat->ref_spad_array__ref_location);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->result__peak_signal_count_rate_rtn_mcps,
		7,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"result__peak_signal_count_rate_rtn_mcps",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->result__ambient_count_rate_rtn_mcps,
		7,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"result__ambient_count_rate_rtn_mcps",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->result__peak_signal_count_rate_ref_mcps,
		7,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"result__peak_signal_count_rate_ref_mcps",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->result__ambient_count_rate_ref_mcps,
		7,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"result__ambient_count_rate_ref_mcps",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdat->measured_distance_mm,
		4,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"measured_distance_mm",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
		(uint32_t)pdat->measured_distance_stdev_mm,
		4,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"measured_distance_stdev_mm",
		fp_text);
}


void VL53L1_print_decoded_nvm_fmt_info(
	VL53L1_decoded_nvm_fmt_info_t *pdat,
	char                          *pprefix,
	uint32_t                       trace_flags)
{
	trace_print(VL53L1_TRACE_LEVEL_INFO,
		"%s%s = \"%s\"\n",
		pprefix,
		"nvm__fmt__fgc",
		pdat->nvm__fmt__fgc);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__fmt__test_program_major",
		pdat->nvm__fmt__test_program_major);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__fmt__test_program_minor",
		pdat->nvm__fmt__test_program_minor);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__fmt__map_major",
		pdat->nvm__fmt__map_major);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__fmt__map_minor",
		pdat->nvm__fmt__map_minor);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__fmt__year",
		pdat->nvm__fmt__year);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__fmt__month",
		pdat->nvm__fmt__month);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__fmt__day",
		pdat->nvm__fmt__day);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__fmt__module_date_phase",
		pdat->nvm__fmt__module_date_phase);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__fmt__time",
		pdat->nvm__fmt__time);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__fmt__tester_id",
		pdat->nvm__fmt__tester_id);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__fmt__site_id",
		pdat->nvm__fmt__site_id);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__ews__test_program_major",
		pdat->nvm__ews__test_program_major);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__ews__test_program_minor",
		pdat->nvm__ews__test_program_minor);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__ews__probe_card_major",
		pdat->nvm__ews__probe_card_major);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__ews__probe_card_minor",
		pdat->nvm__ews__probe_card_minor);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__ews__tester_id",
		pdat->nvm__ews__tester_id);
}


void VL53L1_print_decoded_nvm_ews_info(
	VL53L1_decoded_nvm_ews_info_t *pdat,
	char                          *pprefix,
	uint32_t                       trace_flags)
{
	trace_print(VL53L1_TRACE_LEVEL_INFO,
		"%s%s = \"%s\"\n",
		pprefix,
		"nvm__ews__lot",
		pdat->nvm__ews__lot);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__ews__wafer",
		pdat->nvm__ews__wafer);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__ews__xcoord",
		pdat->nvm__ews__xcoord);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"nvm__ews__ycoord",
		pdat->nvm__ews__ycoord);
}

#endif


