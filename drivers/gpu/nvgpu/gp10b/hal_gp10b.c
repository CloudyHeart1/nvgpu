/*
 * GP10B Tegra HAL interface
 *
 * Copyright (c) 2014-2019, NVIDIA CORPORATION.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#include <nvgpu/debug.h>
#include <nvgpu/bug.h>
#include <nvgpu/enabled.h>
#include <nvgpu/ptimer.h>
#include <nvgpu/ctxsw_trace.h>
#include <nvgpu/error_notifier.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/debugger.h>
#include <nvgpu/channel.h>
#include <nvgpu/runlist.h>
#include <nvgpu/tsg.h>
#include <nvgpu/perfbuf.h>
#include <nvgpu/cyclestats_snapshot.h>
#include <nvgpu/fuse.h>
#include <nvgpu/regops.h>

#include "common/bus/bus_gk20a.h"
#include "common/clock_gating/gp10b_gating_reglist.h"
#include "common/ptimer/ptimer_gk20a.h"
#include "common/bus/bus_gm20b.h"
#include "common/bus/bus_gp10b.h"
#include "common/priv_ring/priv_ring_gm20b.h"
#include "common/priv_ring/priv_ring_gp10b.h"
#include "common/fb/fb_gm20b.h"
#include "common/fb/fb_gp10b.h"
#include "common/netlist/netlist_gp10b.h"
#include "common/gr/ctxsw_prog/ctxsw_prog_gm20b.h"
#include "common/gr/ctxsw_prog/ctxsw_prog_gp10b.h"
#include "common/gr/config/gr_config_gm20b.h"
#include "common/therm/therm_gm20b.h"
#include "common/therm/therm_gp10b.h"
#include "common/ltc/ltc_gm20b.h"
#include "common/ltc/ltc_gp10b.h"
#include "common/fuse/fuse_gm20b.h"
#include "common/fuse/fuse_gp10b.h"
#include "common/mc/mc_gm20b.h"
#include "common/mc/mc_gp10b.h"
#include "common/perf/perf_gm20b.h"
#include "common/pmu/pmu_gk20a.h"
#include "common/pmu/pmu_gm20b.h"
#include "common/pmu/pmu_gp10b.h"
#include "common/pmu/acr_gm20b.h"
#include "common/pmu/acr_gp10b.h"
#include "common/falcon/falcon_gk20a.h"
#include "common/top/top_gm20b.h"
#include "common/top/top_gp10b.h"
#include "common/sync/syncpt_cmdbuf_gk20a.h"
#include "common/sync/sema_cmdbuf_gk20a.h"
#include "common/regops/regops_gp10b.h"
#include "common/fifo/runlist_gk20a.h"

#include "gk20a/fifo_gk20a.h"
#include "gk20a/fecs_trace_gk20a.h"
#include "gk20a/mm_gk20a.h"
#include "gk20a/gr_gk20a.h"

#include "gp10b/gr_gp10b.h"
#include "gp10b/fecs_trace_gp10b.h"
#include "gp10b/mm_gp10b.h"
#include "gp10b/ce_gp10b.h"
#include "gp10b/fifo_gp10b.h"
#include "gp10b/ecc_gp10b.h"
#include "gp10b/clk_arb_gp10b.h"

#include "gm20b/gr_gm20b.h"
#include "gm20b/fifo_gm20b.h"
#include "gm20b/clk_gm20b.h"
#include "gm20b/mm_gm20b.h"

#include "gp10b.h"
#include "hal_gp10b.h"

#include <nvgpu/hw/gp10b/hw_proj_gp10b.h>
#include <nvgpu/hw/gp10b/hw_top_gp10b.h>
#include <nvgpu/hw/gp10b/hw_pram_gp10b.h>
#include <nvgpu/hw/gp10b/hw_pwr_gp10b.h>
#include <nvgpu/hw/gp10b/hw_gr_gp10b.h>

u32 gp10b_get_litter_value(struct gk20a *g, int value)
{
	u32 ret = 0;
	switch (value) {
	case GPU_LIT_NUM_GPCS:
		ret = proj_scal_litter_num_gpcs_v();
		break;
	case GPU_LIT_NUM_PES_PER_GPC:
		ret = proj_scal_litter_num_pes_per_gpc_v();
		break;
	case GPU_LIT_NUM_ZCULL_BANKS:
		ret = proj_scal_litter_num_zcull_banks_v();
		break;
	case GPU_LIT_NUM_TPC_PER_GPC:
		ret = proj_scal_litter_num_tpc_per_gpc_v();
		break;
	case GPU_LIT_NUM_SM_PER_TPC:
		ret = proj_scal_litter_num_sm_per_tpc_v();
		break;
	case GPU_LIT_NUM_FBPS:
		ret = proj_scal_litter_num_fbps_v();
		break;
	case GPU_LIT_GPC_BASE:
		ret = proj_gpc_base_v();
		break;
	case GPU_LIT_GPC_STRIDE:
		ret = proj_gpc_stride_v();
		break;
	case GPU_LIT_GPC_SHARED_BASE:
		ret = proj_gpc_shared_base_v();
		break;
	case GPU_LIT_TPC_IN_GPC_BASE:
		ret = proj_tpc_in_gpc_base_v();
		break;
	case GPU_LIT_TPC_IN_GPC_STRIDE:
		ret = proj_tpc_in_gpc_stride_v();
		break;
	case GPU_LIT_TPC_IN_GPC_SHARED_BASE:
		ret = proj_tpc_in_gpc_shared_base_v();
		break;
	case GPU_LIT_PPC_IN_GPC_BASE:
		ret = proj_ppc_in_gpc_base_v();
		break;
	case GPU_LIT_PPC_IN_GPC_STRIDE:
		ret = proj_ppc_in_gpc_stride_v();
		break;
	case GPU_LIT_PPC_IN_GPC_SHARED_BASE:
		ret = proj_ppc_in_gpc_shared_base_v();
		break;
	case GPU_LIT_ROP_BASE:
		ret = proj_rop_base_v();
		break;
	case GPU_LIT_ROP_STRIDE:
		ret = proj_rop_stride_v();
		break;
	case GPU_LIT_ROP_SHARED_BASE:
		ret = proj_rop_shared_base_v();
		break;
	case GPU_LIT_HOST_NUM_ENGINES:
		ret = proj_host_num_engines_v();
		break;
	case GPU_LIT_HOST_NUM_PBDMA:
		ret = proj_host_num_pbdma_v();
		break;
	case GPU_LIT_LTC_STRIDE:
		ret = proj_ltc_stride_v();
		break;
	case GPU_LIT_LTS_STRIDE:
		ret = proj_lts_stride_v();
		break;
	/* Even though GP10B doesn't have an FBPA unit, the HW reports one,
	 * and the microcode as a result leaves space in the context buffer
	 * for one, so make sure SW accounts for this also.
	 */
	case GPU_LIT_NUM_FBPAS:
		ret = proj_scal_litter_num_fbpas_v();
		break;
	/* Hardcode FBPA values other than NUM_FBPAS to 0. */
	case GPU_LIT_FBPA_STRIDE:
	case GPU_LIT_FBPA_BASE:
	case GPU_LIT_FBPA_SHARED_BASE:
		ret = 0;
		break;
	case GPU_LIT_TWOD_CLASS:
		ret = FERMI_TWOD_A;
		break;
	case GPU_LIT_THREED_CLASS:
		ret = PASCAL_A;
		break;
	case GPU_LIT_COMPUTE_CLASS:
		ret = PASCAL_COMPUTE_A;
		break;
	case GPU_LIT_GPFIFO_CLASS:
		ret = PASCAL_CHANNEL_GPFIFO_A;
		break;
	case GPU_LIT_I2M_CLASS:
		ret = KEPLER_INLINE_TO_MEMORY_B;
		break;
	case GPU_LIT_DMA_COPY_CLASS:
		ret = PASCAL_DMA_COPY_A;
		break;
	case GPU_LIT_GPC_PRIV_STRIDE:
		ret = proj_gpc_priv_stride_v();
		break;
	default:
		nvgpu_err(g, "Missing definition %d", value);
		BUG();
		break;
	}

	return ret;
}

static const struct gpu_ops gp10b_ops = {
	.ltc = {
		.determine_L2_size_bytes = gp10b_determine_L2_size_bytes,
		.set_zbc_color_entry = gm20b_ltc_set_zbc_color_entry,
		.set_zbc_depth_entry = gm20b_ltc_set_zbc_depth_entry,
		.init_cbc = gm20b_ltc_init_cbc,
		.init_fs_state = gp10b_ltc_init_fs_state,
		.init_comptags = gp10b_ltc_init_comptags,
		.cbc_ctrl = gp10b_ltc_cbc_ctrl,
		.isr = gp10b_ltc_isr,
		.cbc_fix_config = gm20b_ltc_cbc_fix_config,
		.flush = gm20b_flush_ltc,
		.set_enabled = gp10b_ltc_set_enabled,
		.pri_is_ltc_addr = gm20b_ltc_pri_is_ltc_addr,
		.is_ltcs_ltss_addr = gm20b_ltc_is_ltcs_ltss_addr,
		.is_ltcn_ltss_addr = gm20b_ltc_is_ltcn_ltss_addr,
		.split_lts_broadcast_addr = gm20b_ltc_split_lts_broadcast_addr,
		.split_ltc_broadcast_addr = gm20b_ltc_split_ltc_broadcast_addr,
	},
	.ce2 = {
		.isr_stall = gp10b_ce_isr,
		.isr_nonstall = gp10b_ce_nonstall_isr,
	},
	.gr = {
		.get_patch_slots = gr_gk20a_get_patch_slots,
		.init_gpc_mmu = gr_gm20b_init_gpc_mmu,
		.bundle_cb_defaults = gr_gm20b_bundle_cb_defaults,
		.cb_size_default = gr_gp10b_cb_size_default,
		.calc_global_ctx_buffer_size =
			gr_gp10b_calc_global_ctx_buffer_size,
		.commit_global_attrib_cb = gr_gp10b_commit_global_attrib_cb,
		.commit_global_bundle_cb = gr_gp10b_commit_global_bundle_cb,
		.commit_global_cb_manager = gr_gp10b_commit_global_cb_manager,
		.commit_global_pagepool = gr_gp10b_commit_global_pagepool,
		.handle_sw_method = gr_gp10b_handle_sw_method,
		.set_alpha_circular_buffer_size =
			gr_gp10b_set_alpha_circular_buffer_size,
		.set_circular_buffer_size = gr_gp10b_set_circular_buffer_size,
		.enable_hww_exceptions = gr_gk20a_enable_hww_exceptions,
		.is_valid_class = gr_gp10b_is_valid_class,
		.is_valid_gfx_class = gr_gp10b_is_valid_gfx_class,
		.is_valid_compute_class = gr_gp10b_is_valid_compute_class,
		.get_sm_dsm_perf_regs = gr_gm20b_get_sm_dsm_perf_regs,
		.get_sm_dsm_perf_ctrl_regs = gr_gm20b_get_sm_dsm_perf_ctrl_regs,
		.init_fs_state = gr_gp10b_init_fs_state,
		.set_hww_esr_report_mask = gr_gm20b_set_hww_esr_report_mask,
		.fecs_falcon_base_addr = gr_gk20a_fecs_falcon_base_addr,
		.gpccs_falcon_base_addr = gr_gk20a_gpccs_falcon_base_addr,
		.falcon_load_ucode = gr_gm20b_load_ctxsw_ucode_segments,
		.load_ctxsw_ucode = gr_gk20a_load_ctxsw_ucode,
		.set_gpc_tpc_mask = gr_gp10b_set_gpc_tpc_mask,
		.alloc_obj_ctx = gk20a_alloc_obj_ctx,
		.bind_ctxsw_zcull = gr_gk20a_bind_ctxsw_zcull,
		.get_zcull_info = gr_gk20a_get_zcull_info,
		.is_tpc_addr = gr_gm20b_is_tpc_addr,
		.get_tpc_num = gr_gm20b_get_tpc_num,
		.detect_sm_arch = gr_gm20b_detect_sm_arch,
		.add_zbc_color = gr_gp10b_add_zbc_color,
		.add_zbc_depth = gr_gp10b_add_zbc_depth,
		.get_gpcs_swdx_dss_zbc_c_format_reg =
			gr_gp10b_get_gpcs_swdx_dss_zbc_c_format_reg,
		.get_gpcs_swdx_dss_zbc_z_format_reg =
			gr_gp10b_get_gpcs_swdx_dss_zbc_z_format_reg,
		.zbc_set_table = gk20a_gr_zbc_set_table,
		.zbc_query_table = gr_gk20a_query_zbc,
		.pmu_save_zbc = gk20a_pmu_save_zbc,
		.add_zbc = gr_gk20a_add_zbc,
		.pagepool_default_size = gr_gp10b_pagepool_default_size,
		.init_ctx_state = gr_gp10b_init_ctx_state,
		.alloc_gr_ctx = gr_gk20a_alloc_gr_ctx,
		.free_gr_ctx = gr_gk20a_free_gr_ctx,
		.init_ctxsw_preemption_mode =
			gr_gp10b_init_ctxsw_preemption_mode,
		.update_ctxsw_preemption_mode =
			gr_gp10b_update_ctxsw_preemption_mode,
		.dump_gr_regs = gr_gp10b_dump_gr_status_regs,
		.update_pc_sampling = gr_gm20b_update_pc_sampling,
		.get_fbp_en_mask = gr_gm20b_get_fbp_en_mask,
		.get_max_ltc_per_fbp = gr_gm20b_get_max_ltc_per_fbp,
		.get_max_lts_per_ltc = gr_gm20b_get_max_lts_per_ltc,
		.get_rop_l2_en_mask = gr_gm20b_rop_l2_en_mask,
		.get_max_fbps_count = gr_gm20b_get_max_fbps_count,
		.init_sm_dsm_reg_info = gr_gm20b_init_sm_dsm_reg_info,
		.wait_empty = gr_gp10b_wait_empty,
		.init_cyclestats = gr_gm20b_init_cyclestats,
		.set_sm_debug_mode = gr_gk20a_set_sm_debug_mode,
		.bpt_reg_info = gr_gm20b_bpt_reg_info,
		.get_access_map = gr_gp10b_get_access_map,
		.handle_fecs_error = gr_gp10b_handle_fecs_error,
		.handle_sm_exception = gr_gp10b_handle_sm_exception,
		.handle_tex_exception = gr_gp10b_handle_tex_exception,
		.enable_gpc_exceptions = gk20a_gr_enable_gpc_exceptions,
		.enable_exceptions = gk20a_gr_enable_exceptions,
		.get_lrf_tex_ltc_dram_override = get_ecc_override_val,
		.update_smpc_ctxsw_mode = gr_gk20a_update_smpc_ctxsw_mode,
		.update_hwpm_ctxsw_mode = gr_gk20a_update_hwpm_ctxsw_mode,
		.record_sm_error_state = gm20b_gr_record_sm_error_state,
		.clear_sm_error_state = gm20b_gr_clear_sm_error_state,
		.suspend_contexts = gr_gp10b_suspend_contexts,
		.resume_contexts = gr_gk20a_resume_contexts,
		.get_preemption_mode_flags = gr_gp10b_get_preemption_mode_flags,
		.init_sm_id_table = gr_gk20a_init_sm_id_table,
		.load_smid_config = gr_gp10b_load_smid_config,
		.program_sm_id_numbering = gr_gm20b_program_sm_id_numbering,
		.setup_rop_mapping = gr_gk20a_setup_rop_mapping,
		.program_zcull_mapping = gr_gk20a_program_zcull_mapping,
		.commit_global_timeslice = gr_gk20a_commit_global_timeslice,
		.commit_inst = gr_gk20a_commit_inst,
		.load_tpc_mask = gr_gm20b_load_tpc_mask,
		.trigger_suspend = gr_gk20a_trigger_suspend,
		.wait_for_pause = gr_gk20a_wait_for_pause,
		.resume_from_pause = gr_gk20a_resume_from_pause,
		.clear_sm_errors = gr_gk20a_clear_sm_errors,
		.tpc_enabled_exceptions = gr_gk20a_tpc_enabled_exceptions,
		.get_esr_sm_sel = gk20a_gr_get_esr_sm_sel,
		.sm_debugger_attached = gk20a_gr_sm_debugger_attached,
		.suspend_single_sm = gk20a_gr_suspend_single_sm,
		.suspend_all_sms = gk20a_gr_suspend_all_sms,
		.resume_single_sm = gk20a_gr_resume_single_sm,
		.resume_all_sms = gk20a_gr_resume_all_sms,
		.get_sm_hww_warp_esr = gp10b_gr_get_sm_hww_warp_esr,
		.get_sm_hww_global_esr = gk20a_gr_get_sm_hww_global_esr,
		.get_sm_no_lock_down_hww_global_esr_mask =
			gk20a_gr_get_sm_no_lock_down_hww_global_esr_mask,
		.lock_down_sm = gk20a_gr_lock_down_sm,
		.wait_for_sm_lock_down = gk20a_gr_wait_for_sm_lock_down,
		.clear_sm_hww = gm20b_gr_clear_sm_hww,
		.init_ovr_sm_dsm_perf =  gk20a_gr_init_ovr_sm_dsm_perf,
		.get_ovr_perf_regs = gk20a_gr_get_ovr_perf_regs,
		.disable_rd_coalesce = gm20a_gr_disable_rd_coalesce,
		.set_boosted_ctx = gr_gp10b_set_boosted_ctx,
		.set_preemption_mode = gr_gp10b_set_preemption_mode,
		.set_czf_bypass = gr_gp10b_set_czf_bypass,
		.init_czf_bypass = gr_gp10b_init_czf_bypass,
		.pre_process_sm_exception = gr_gp10b_pre_process_sm_exception,
		.set_preemption_buffer_va = gr_gp10b_set_preemption_buffer_va,
		.init_preemption_state = gr_gp10b_init_preemption_state,
		.set_bes_crop_debug3 = gr_gp10b_set_bes_crop_debug3,
		.set_ctxsw_preemption_mode = gr_gp10b_set_ctxsw_preemption_mode,
		.init_ecc = gp10b_ecc_init,
		.init_gfxp_wfi_timeout_count =
				gr_gp10b_init_gfxp_wfi_timeout_count,
		.get_max_gfxp_wfi_timeout_count =
				gr_gp10b_get_max_gfxp_wfi_timeout_count,
		.fecs_host_int_enable = gr_gk20a_fecs_host_int_enable,
		.handle_notify_pending = gk20a_gr_handle_notify_pending,
		.handle_semaphore_pending = gk20a_gr_handle_semaphore_pending,
		.add_ctxsw_reg_pm_fbpa = gr_gk20a_add_ctxsw_reg_pm_fbpa,
		.add_ctxsw_reg_perf_pma = gr_gk20a_add_ctxsw_reg_perf_pma,
		.decode_priv_addr = gr_gk20a_decode_priv_addr,
		.create_priv_addr_table = gr_gk20a_create_priv_addr_table,
		.get_pmm_per_chiplet_offset =
			gr_gm20b_get_pmm_per_chiplet_offset,
		.split_fbpa_broadcast_addr = gr_gk20a_split_fbpa_broadcast_addr,
		.fecs_ctxsw_mailbox_size = gr_fecs_ctxsw_mailbox__size_1_v,
		.alloc_global_ctx_buffers = gr_gk20a_alloc_global_ctx_buffers,
		.commit_global_ctx_buffers = gr_gk20a_commit_global_ctx_buffers,
		.get_offset_in_gpccs_segment =
			gr_gk20a_get_offset_in_gpccs_segment,
		.set_debug_mode = gm20b_gr_set_debug_mode,
		.dump_gr_falcon_stats = gk20a_fecs_dump_falcon_stats,
		.get_fecs_ctx_state_store_major_rev_id =
			gk20a_gr_get_fecs_ctx_state_store_major_rev_id,
		.init_gfxp_rtv_cb = NULL,
		.commit_gfxp_rtv_cb = NULL,
		.log_mme_exception = NULL,
		.ctxsw_prog = {
			.hw_get_fecs_header_size =
				gm20b_ctxsw_prog_hw_get_fecs_header_size,
			.hw_get_gpccs_header_size =
				gm20b_ctxsw_prog_hw_get_gpccs_header_size,
			.hw_get_extended_buffer_segments_size_in_bytes =
				gm20b_ctxsw_prog_hw_get_extended_buffer_segments_size_in_bytes,
			.hw_extended_marker_size_in_bytes =
				gm20b_ctxsw_prog_hw_extended_marker_size_in_bytes,
			.hw_get_perf_counter_control_register_stride =
				gm20b_ctxsw_prog_hw_get_perf_counter_control_register_stride,
			.get_main_image_ctx_id =
				gm20b_ctxsw_prog_get_main_image_ctx_id,
			.get_patch_count = gm20b_ctxsw_prog_get_patch_count,
			.set_patch_count = gm20b_ctxsw_prog_set_patch_count,
			.set_patch_addr = gm20b_ctxsw_prog_set_patch_addr,
			.set_zcull_ptr = gm20b_ctxsw_prog_set_zcull_ptr,
			.set_zcull = gm20b_ctxsw_prog_set_zcull,
			.set_zcull_mode_no_ctxsw =
				gm20b_ctxsw_prog_set_zcull_mode_no_ctxsw,
			.is_zcull_mode_separate_buffer =
				gm20b_ctxsw_prog_is_zcull_mode_separate_buffer,
			.set_pm_ptr = gm20b_ctxsw_prog_set_pm_ptr,
			.set_pm_mode = gm20b_ctxsw_prog_set_pm_mode,
			.set_pm_smpc_mode = gm20b_ctxsw_prog_set_pm_smpc_mode,
			.set_pm_mode_no_ctxsw =
				gm20b_ctxsw_prog_set_pm_mode_no_ctxsw,
			.set_pm_mode_ctxsw = gm20b_ctxsw_prog_set_pm_mode_ctxsw,
			.hw_get_pm_mode_no_ctxsw =
				gm20b_ctxsw_prog_hw_get_pm_mode_no_ctxsw,
			.hw_get_pm_mode_ctxsw = gm20b_ctxsw_prog_hw_get_pm_mode_ctxsw,
			.init_ctxsw_hdr_data = gp10b_ctxsw_prog_init_ctxsw_hdr_data,
			.set_compute_preemption_mode_cta =
				gp10b_ctxsw_prog_set_compute_preemption_mode_cta,
			.set_compute_preemption_mode_cilp =
				gp10b_ctxsw_prog_set_compute_preemption_mode_cilp,
			.set_graphics_preemption_mode_gfxp =
				gp10b_ctxsw_prog_set_graphics_preemption_mode_gfxp,
			.set_cde_enabled = gm20b_ctxsw_prog_set_cde_enabled,
			.set_pc_sampling = gm20b_ctxsw_prog_set_pc_sampling,
			.set_priv_access_map_config_mode =
				gm20b_ctxsw_prog_set_priv_access_map_config_mode,
			.set_priv_access_map_addr =
				gm20b_ctxsw_prog_set_priv_access_map_addr,
			.disable_verif_features =
				gm20b_ctxsw_prog_disable_verif_features,
			.check_main_image_header_magic =
				gm20b_ctxsw_prog_check_main_image_header_magic,
			.check_local_header_magic =
				gm20b_ctxsw_prog_check_local_header_magic,
			.get_num_gpcs = gm20b_ctxsw_prog_get_num_gpcs,
			.get_num_tpcs = gm20b_ctxsw_prog_get_num_tpcs,
			.get_extended_buffer_size_offset =
				gm20b_ctxsw_prog_get_extended_buffer_size_offset,
			.get_ppc_info = gm20b_ctxsw_prog_get_ppc_info,
			.get_local_priv_register_ctl_offset =
				gm20b_ctxsw_prog_get_local_priv_register_ctl_offset,
			.hw_get_ts_tag_invalid_timestamp =
				gm20b_ctxsw_prog_hw_get_ts_tag_invalid_timestamp,
			.hw_get_ts_tag = gm20b_ctxsw_prog_hw_get_ts_tag,
			.hw_record_ts_timestamp =
				gm20b_ctxsw_prog_hw_record_ts_timestamp,
			.hw_get_ts_record_size_in_bytes =
				gm20b_ctxsw_prog_hw_get_ts_record_size_in_bytes,
			.is_ts_valid_record = gm20b_ctxsw_prog_is_ts_valid_record,
			.get_ts_buffer_aperture_mask =
				gm20b_ctxsw_prog_get_ts_buffer_aperture_mask,
			.set_ts_num_records = gm20b_ctxsw_prog_set_ts_num_records,
			.set_ts_buffer_ptr = gm20b_ctxsw_prog_set_ts_buffer_ptr,
			.set_pmu_options_boost_clock_frequencies =
				gp10b_ctxsw_prog_set_pmu_options_boost_clock_frequencies,
			.set_full_preemption_ptr =
				gp10b_ctxsw_prog_set_full_preemption_ptr,
			.dump_ctxsw_stats = gp10b_ctxsw_prog_dump_ctxsw_stats,
		},
		.config = {
			.get_gpc_tpc_mask = gm20b_gr_config_get_gpc_tpc_mask,
			.get_tpc_count_in_gpc =
				gm20b_gr_config_get_tpc_count_in_gpc,
			.get_zcull_count_in_gpc =
				gm20b_gr_config_get_zcull_count_in_gpc,
			.get_pes_tpc_mask = gm20b_gr_config_get_pes_tpc_mask,
			.get_pd_dist_skip_table_size =
				gm20b_gr_config_get_pd_dist_skip_table_size,
		}
	},
	.fb = {
		.init_hw = gm20b_fb_init_hw,
		.init_fs_state = fb_gm20b_init_fs_state,
		.set_mmu_page_size = NULL,
		.set_use_full_comp_tag_line =
			gm20b_fb_set_use_full_comp_tag_line,
		.mmu_ctrl = gm20b_fb_mmu_ctrl,
		.mmu_debug_ctrl = gm20b_fb_mmu_debug_ctrl,
		.mmu_debug_wr = gm20b_fb_mmu_debug_wr,
		.mmu_debug_rd = gm20b_fb_mmu_debug_rd,
		.compression_page_size = gp10b_fb_compression_page_size,
		.compressible_page_size = gp10b_fb_compressible_page_size,
		.compression_align_mask = gm20b_fb_compression_align_mask,
		.vpr_info_fetch = gm20b_fb_vpr_info_fetch,
		.dump_vpr_info = gm20b_fb_dump_vpr_info,
		.dump_wpr_info = gm20b_fb_dump_wpr_info,
		.read_wpr_info = gm20b_fb_read_wpr_info,
		.is_debug_mode_enabled = gm20b_fb_debug_mode_enabled,
		.set_debug_mode = gm20b_fb_set_debug_mode,
		.tlb_invalidate = gm20b_fb_tlb_invalidate,
		.mem_unlock = NULL,
	},
	.clock_gating = {
		.slcg_bus_load_gating_prod =
			gp10b_slcg_bus_load_gating_prod,
		.slcg_ce2_load_gating_prod =
			gp10b_slcg_ce2_load_gating_prod,
		.slcg_chiplet_load_gating_prod =
			gp10b_slcg_chiplet_load_gating_prod,
		.slcg_ctxsw_firmware_load_gating_prod =
			gp10b_slcg_ctxsw_firmware_load_gating_prod,
		.slcg_fb_load_gating_prod =
			gp10b_slcg_fb_load_gating_prod,
		.slcg_fifo_load_gating_prod =
			gp10b_slcg_fifo_load_gating_prod,
		.slcg_gr_load_gating_prod =
			gr_gp10b_slcg_gr_load_gating_prod,
		.slcg_ltc_load_gating_prod =
			ltc_gp10b_slcg_ltc_load_gating_prod,
		.slcg_perf_load_gating_prod =
			gp10b_slcg_perf_load_gating_prod,
		.slcg_priring_load_gating_prod =
			gp10b_slcg_priring_load_gating_prod,
		.slcg_pmu_load_gating_prod =
			gp10b_slcg_pmu_load_gating_prod,
		.slcg_therm_load_gating_prod =
			gp10b_slcg_therm_load_gating_prod,
		.slcg_xbar_load_gating_prod =
			gp10b_slcg_xbar_load_gating_prod,
		.blcg_bus_load_gating_prod =
			gp10b_blcg_bus_load_gating_prod,
		.blcg_ce_load_gating_prod =
			gp10b_blcg_ce_load_gating_prod,
		.blcg_ctxsw_firmware_load_gating_prod =
			gp10b_blcg_ctxsw_firmware_load_gating_prod,
		.blcg_fb_load_gating_prod =
			gp10b_blcg_fb_load_gating_prod,
		.blcg_fifo_load_gating_prod =
			gp10b_blcg_fifo_load_gating_prod,
		.blcg_gr_load_gating_prod =
			gp10b_blcg_gr_load_gating_prod,
		.blcg_ltc_load_gating_prod =
			gp10b_blcg_ltc_load_gating_prod,
		.blcg_pwr_csb_load_gating_prod =
			gp10b_blcg_pwr_csb_load_gating_prod,
		.blcg_pmu_load_gating_prod =
			gp10b_blcg_pmu_load_gating_prod,
		.blcg_xbar_load_gating_prod =
			gp10b_blcg_xbar_load_gating_prod,
		.pg_gr_load_gating_prod =
			gr_gp10b_pg_gr_load_gating_prod,
	},
	.fifo = {
		.init_fifo_setup_hw = gk20a_init_fifo_setup_hw,
		.bind_channel = channel_gm20b_bind,
		.unbind_channel = gk20a_fifo_channel_unbind,
		.disable_channel = gk20a_fifo_disable_channel,
		.enable_channel = gk20a_fifo_enable_channel,
		.alloc_inst = gk20a_fifo_alloc_inst,
		.free_inst = gk20a_fifo_free_inst,
		.setup_ramfc = channel_gp10b_setup_ramfc,
		.default_timeslice_us = gk20a_fifo_default_timeslice_us,
		.setup_userd = gk20a_fifo_setup_userd,
		.userd_gp_get = gk20a_fifo_userd_gp_get,
		.userd_gp_put = gk20a_fifo_userd_gp_put,
		.userd_pb_get = gk20a_fifo_userd_pb_get,
		.pbdma_acquire_val = gk20a_fifo_pbdma_acquire_val,
		.preempt_channel = gk20a_fifo_preempt_channel,
		.preempt_tsg = gk20a_fifo_preempt_tsg,
		.enable_tsg = gk20a_enable_tsg,
		.disable_tsg = gk20a_disable_tsg,
		.tsg_verify_channel_status = gk20a_fifo_tsg_unbind_channel_verify_status,
		.tsg_verify_status_ctx_reload = gm20b_fifo_tsg_verify_status_ctx_reload,
		.trigger_mmu_fault = gm20b_fifo_trigger_mmu_fault,
		.get_mmu_fault_info = gp10b_fifo_get_mmu_fault_info,
		.get_mmu_fault_desc = gp10b_fifo_get_mmu_fault_desc,
		.get_mmu_fault_client_desc = gp10b_fifo_get_mmu_fault_client_desc,
		.get_mmu_fault_gpc_desc = gm20b_fifo_get_mmu_fault_gpc_desc,
		.wait_engine_idle = gk20a_fifo_wait_engine_idle,
		.get_num_fifos = gm20b_fifo_get_num_fifos,
		.get_pbdma_signature = gp10b_fifo_get_pbdma_signature,
		.tsg_set_timeslice = gk20a_fifo_tsg_set_timeslice,
		.force_reset_ch = gk20a_fifo_force_reset_ch,
		.init_engine_info = gm20b_fifo_init_engine_info,
		.get_engines_mask_on_id = gk20a_fifo_engines_on_id,
		.is_fault_engine_subid_gpc = gk20a_is_fault_engine_subid_gpc,
		.dump_pbdma_status = gk20a_dump_pbdma_status,
		.dump_eng_status = gk20a_dump_eng_status,
		.dump_channel_status_ramfc = gk20a_dump_channel_status_ramfc,
		.capture_channel_ram_dump = gk20a_capture_channel_ram_dump,
		.intr_0_error_mask = gk20a_fifo_intr_0_error_mask,
		.is_preempt_pending = gk20a_fifo_is_preempt_pending,
		.init_pbdma_intr_descs = gp10b_fifo_init_pbdma_intr_descs,
		.reset_enable_hw = gk20a_init_fifo_reset_enable_hw,
		.teardown_ch_tsg = gk20a_fifo_teardown_ch_tsg,
		.handle_sched_error = gk20a_fifo_handle_sched_error,
		.handle_pbdma_intr_0 = gk20a_fifo_handle_pbdma_intr_0,
		.handle_pbdma_intr_1 = gk20a_fifo_handle_pbdma_intr_1,
		.tsg_bind_channel = gk20a_tsg_bind_channel,
		.tsg_unbind_channel = gk20a_fifo_tsg_unbind_channel,
		.post_event_id = gk20a_tsg_event_id_post_event,
		.ch_abort_clean_up = gk20a_channel_abort_clean_up,
		.check_tsg_ctxsw_timeout = nvgpu_tsg_check_ctxsw_timeout,
		.check_ch_ctxsw_timeout = nvgpu_channel_check_ctxsw_timeout,
		.channel_suspend = gk20a_channel_suspend,
		.channel_resume = gk20a_channel_resume,
		.set_error_notifier = nvgpu_set_error_notifier,
		.setup_sw = gk20a_init_fifo_setup_sw,
		.resetup_ramfc = gp10b_fifo_resetup_ramfc,
		.set_sm_exception_type_mask = gk20a_tsg_set_sm_exception_type_mask,
		.runlist_busy_engines = gk20a_fifo_runlist_busy_engines,
		.find_pbdma_for_runlist = gk20a_fifo_find_pbdma_for_runlist,
		.init_ce_engine_info = gp10b_fifo_init_ce_engine_info,
		.read_pbdma_data = gk20a_fifo_read_pbdma_data,
		.reset_pbdma_header = gk20a_fifo_reset_pbdma_header,
	},
	.sync = {
#ifdef CONFIG_TEGRA_GK20A_NVHOST
		.alloc_syncpt_buf = gk20a_alloc_syncpt_buf,
		.free_syncpt_buf = gk20a_free_syncpt_buf,
		.add_syncpt_wait_cmd = gk20a_add_syncpt_wait_cmd,
		.get_syncpt_incr_per_release =
				gk20a_get_syncpt_incr_per_release,
		.get_syncpt_wait_cmd_size = gk20a_get_syncpt_wait_cmd_size,
		.add_syncpt_incr_cmd = gk20a_add_syncpt_incr_cmd,
		.get_syncpt_incr_cmd_size = gk20a_get_syncpt_incr_cmd_size,
		.get_sync_ro_map = NULL,
#endif
		.get_sema_wait_cmd_size = gk20a_get_sema_wait_cmd_size,
		.get_sema_incr_cmd_size = gk20a_get_sema_incr_cmd_size,
		.add_sema_cmd = gk20a_add_sema_cmd,
	},
	.runlist = {
		.reschedule_runlist = gk20a_fifo_reschedule_runlist,
		.reschedule_preempt_next_locked = gk20a_fifo_reschedule_preempt_next,
		.update_for_channel = gk20a_runlist_update_for_channel,
		.reload = gk20a_runlist_reload,
		.set_runlist_interleave = gk20a_fifo_set_runlist_interleave,
		.eng_runlist_base_size = gk20a_fifo_runlist_base_size,
		.runlist_entry_size = gk20a_fifo_runlist_entry_size,
		.get_tsg_runlist_entry = gk20a_get_tsg_runlist_entry,
		.get_ch_runlist_entry = gk20a_get_ch_runlist_entry,
		.runlist_hw_submit = gk20a_fifo_runlist_hw_submit,
		.runlist_wait_pending = gk20a_fifo_runlist_wait_pending,
		.runlist_write_state = gk20a_fifo_runlist_write_state,
	},
	.netlist = {
		.get_netlist_name = gp10b_netlist_get_name,
		.is_fw_defined = gp10b_netlist_is_firmware_defined,
	},
#ifdef CONFIG_GK20A_CTXSW_TRACE
	.fecs_trace = {
		.alloc_user_buffer = gk20a_ctxsw_dev_ring_alloc,
		.free_user_buffer = gk20a_ctxsw_dev_ring_free,
		.mmap_user_buffer = gk20a_ctxsw_dev_mmap_buffer,
		.init = gk20a_fecs_trace_init,
		.deinit = gk20a_fecs_trace_deinit,
		.enable = gk20a_fecs_trace_enable,
		.disable = gk20a_fecs_trace_disable,
		.is_enabled = gk20a_fecs_trace_is_enabled,
		.reset = gk20a_fecs_trace_reset,
		.flush = gp10b_fecs_trace_flush,
		.poll = gk20a_fecs_trace_poll,
		.bind_channel = gk20a_fecs_trace_bind_channel,
		.unbind_channel = gk20a_fecs_trace_unbind_channel,
		.max_entries = gk20a_gr_max_entries,
		.get_buffer_full_mailbox_val =
			gk20a_fecs_trace_get_buffer_full_mailbox_val,
	},
#endif /* CONFIG_GK20A_CTXSW_TRACE */
	.mm = {
		.gmmu_map = gk20a_locked_gmmu_map,
		.gmmu_unmap = gk20a_locked_gmmu_unmap,
		.vm_bind_channel = gk20a_vm_bind_channel,
		.fb_flush = gk20a_mm_fb_flush,
		.l2_invalidate = gk20a_mm_l2_invalidate,
		.l2_flush = gk20a_mm_l2_flush,
		.cbc_clean = gk20a_mm_cbc_clean,
		.set_big_page_size = gm20b_mm_set_big_page_size,
		.get_big_page_sizes = gm20b_mm_get_big_page_sizes,
		.get_default_big_page_size = gp10b_mm_get_default_big_page_size,
		.gpu_phys_addr = gm20b_gpu_phys_addr,
		.get_iommu_bit = gp10b_mm_get_iommu_bit,
		.get_mmu_levels = gp10b_mm_get_mmu_levels,
		.init_pdb = gp10b_mm_init_pdb,
		.init_mm_setup_hw = gk20a_init_mm_setup_hw,
		.is_bar1_supported = gm20b_mm_is_bar1_supported,
		.alloc_inst_block = gk20a_alloc_inst_block,
		.init_inst_block = gk20a_init_inst_block,
		.mmu_fault_pending = gk20a_fifo_mmu_fault_pending,
		.init_bar2_vm = gp10b_init_bar2_vm,
		.remove_bar2_vm = gp10b_remove_bar2_vm,
		.get_kind_invalid = gm20b_get_kind_invalid,
		.get_kind_pitch = gm20b_get_kind_pitch,
		.bar1_map_userd = gk20a_mm_bar1_map_userd,
	},
	.pramin = {
		.data032_r = pram_data032_r,
	},
	.therm = {
		.init_therm_setup_hw = gp10b_init_therm_setup_hw,
		.init_elcg_mode = gm20b_therm_init_elcg_mode,
		.init_blcg_mode = gm20b_therm_init_blcg_mode,
		.elcg_init_idle_filters = gp10b_elcg_init_idle_filters,
	},
	.pmu = {
		.falcon_base_addr = gk20a_pmu_falcon_base_addr,
		.pmu_setup_elpg = gp10b_pmu_setup_elpg,
		.pmu_get_queue_head = pwr_pmu_queue_head_r,
		.pmu_get_queue_head_size = pwr_pmu_queue_head__size_1_v,
		.pmu_get_queue_tail = pwr_pmu_queue_tail_r,
		.pmu_get_queue_tail_size = pwr_pmu_queue_tail__size_1_v,
		.pmu_queue_head = gk20a_pmu_queue_head,
		.pmu_queue_tail = gk20a_pmu_queue_tail,
		.pmu_msgq_tail = gk20a_pmu_msgq_tail,
		.pmu_mutex_size = pwr_pmu_mutex__size_1_v,
		.pmu_mutex_acquire = gk20a_pmu_mutex_acquire,
		.pmu_mutex_release = gk20a_pmu_mutex_release,
		.pmu_is_interrupted = gk20a_pmu_is_interrupted,
		.pmu_isr = gk20a_pmu_isr,
		.pmu_init_perfmon_counter = gk20a_pmu_init_perfmon_counter,
		.pmu_pg_idle_counter_config = gk20a_pmu_pg_idle_counter_config,
		.pmu_read_idle_counter = gk20a_pmu_read_idle_counter,
		.pmu_reset_idle_counter = gk20a_pmu_reset_idle_counter,
		.pmu_read_idle_intr_status = gk20a_pmu_read_idle_intr_status,
		.pmu_clear_idle_intr_status = gk20a_pmu_clear_idle_intr_status,
		.pmu_dump_elpg_stats = gk20a_pmu_dump_elpg_stats,
		.pmu_dump_falcon_stats = gk20a_pmu_dump_falcon_stats,
		.pmu_enable_irq = gk20a_pmu_enable_irq,
		.write_dmatrfbase = gp10b_write_dmatrfbase,
		.pmu_elpg_statistics = gp10b_pmu_elpg_statistics,
		.pmu_init_perfmon = nvgpu_pmu_init_perfmon,
		.pmu_perfmon_start_sampling = nvgpu_pmu_perfmon_start_sampling,
		.pmu_perfmon_stop_sampling = nvgpu_pmu_perfmon_stop_sampling,
		.pmu_pg_init_param = gp10b_pg_gr_init,
		.pmu_pg_supported_engines_list = gk20a_pmu_pg_engines_list,
		.pmu_pg_engines_feature_list = gk20a_pmu_pg_feature_list,
		.dump_secure_fuses = pmu_dump_security_fuses_gm20b,
		.reset_engine = gk20a_pmu_engine_reset,
		.is_engine_in_reset = gk20a_pmu_is_engine_in_reset,
		.get_irqdest = gk20a_pmu_get_irqdest,
		.is_debug_mode_enabled = gm20b_pmu_is_debug_mode_en,
	},
	.clk_arb = {
		.get_arbiter_clk_domains = gp10b_get_arbiter_clk_domains,
		.get_arbiter_f_points = gp10b_get_arbiter_f_points,
		.get_arbiter_clk_range = gp10b_get_arbiter_clk_range,
		.get_arbiter_clk_default = gp10b_get_arbiter_clk_default,
		.arbiter_clk_init = gp10b_init_clk_arbiter,
		.clk_arb_run_arbiter_cb = gp10b_clk_arb_run_arbiter_cb,
		.clk_arb_cleanup = gp10b_clk_arb_cleanup,
	},
	.regops = {
		.exec_regops = exec_regops_gk20a,
		.get_global_whitelist_ranges =
			gp10b_get_global_whitelist_ranges,
		.get_global_whitelist_ranges_count =
			gp10b_get_global_whitelist_ranges_count,
		.get_context_whitelist_ranges =
			gp10b_get_context_whitelist_ranges,
		.get_context_whitelist_ranges_count =
			gp10b_get_context_whitelist_ranges_count,
		.get_runcontrol_whitelist = gp10b_get_runcontrol_whitelist,
		.get_runcontrol_whitelist_count =
			gp10b_get_runcontrol_whitelist_count,
		.get_qctl_whitelist = gp10b_get_qctl_whitelist,
		.get_qctl_whitelist_count = gp10b_get_qctl_whitelist_count,
	},
	.mc = {
		.intr_mask = mc_gp10b_intr_mask,
		.intr_enable = mc_gp10b_intr_enable,
		.intr_unit_config = mc_gp10b_intr_unit_config,
		.isr_stall = mc_gp10b_isr_stall,
		.intr_stall = mc_gp10b_intr_stall,
		.intr_stall_pause = mc_gp10b_intr_stall_pause,
		.intr_stall_resume = mc_gp10b_intr_stall_resume,
		.intr_nonstall = mc_gp10b_intr_nonstall,
		.intr_nonstall_pause = mc_gp10b_intr_nonstall_pause,
		.intr_nonstall_resume = mc_gp10b_intr_nonstall_resume,
		.isr_nonstall = gm20b_mc_isr_nonstall,
		.enable = gm20b_mc_enable,
		.disable = gm20b_mc_disable,
		.reset = gm20b_mc_reset,
		.is_intr1_pending = mc_gp10b_is_intr1_pending,
		.log_pending_intrs = mc_gp10b_log_pending_intrs,
		.reset_mask = gm20b_mc_reset_mask,
		.is_enabled = gm20b_mc_is_enabled,
		.fb_reset = gm20b_mc_fb_reset,
		.ltc_isr = mc_gp10b_ltc_isr,
	},
	.debug = {
		.show_dump = gk20a_debug_show_dump,
	},
#ifdef NVGPU_DEBUGGER
	.debugger = {
		.post_events = nvgpu_dbg_gpu_post_events,
		.dbg_set_powergate = nvgpu_dbg_set_powergate,
		.check_and_set_global_reservation =
			nvgpu_check_and_set_global_reservation,
		.check_and_set_context_reservation =
			nvgpu_check_and_set_context_reservation,
		.release_profiler_reservation =
			nvgpu_release_profiler_reservation,
	},
#endif
	.perf = {
		.enable_membuf = gm20b_perf_enable_membuf,
		.disable_membuf = gm20b_perf_disable_membuf,
		.membuf_reset_streaming = gm20b_perf_membuf_reset_streaming,
		.get_membuf_pending_bytes = gm20b_perf_get_membuf_pending_bytes,
		.set_membuf_handled_bytes = gm20b_perf_set_membuf_handled_bytes,
		.get_membuf_overflow_status =
			gm20b_perf_get_membuf_overflow_status,
	},
	.perfbuf = {
		.perfbuf_enable = nvgpu_perfbuf_enable_locked,
		.perfbuf_disable = nvgpu_perfbuf_disable_locked,
	},
	.bus = {
		.init_hw = gk20a_bus_init_hw,
		.isr = gk20a_bus_isr,
		.bar1_bind = gm20b_bus_bar1_bind,
		.bar2_bind = gp10b_bus_bar2_bind,
		.set_bar0_window = gk20a_bus_set_bar0_window,
	},
	.ptimer = {
		.isr = gk20a_ptimer_isr,
		.read_ptimer = gk20a_read_ptimer,
		.get_timestamps_zipper = nvgpu_get_timestamps_zipper,
	},
#if defined(CONFIG_GK20A_CYCLE_STATS)
	.css = {
		.enable_snapshot = nvgpu_css_enable_snapshot,
		.disable_snapshot = nvgpu_css_disable_snapshot,
		.check_data_available = nvgpu_css_check_data_available,
		.set_handled_snapshots = nvgpu_css_set_handled_snapshots,
		.allocate_perfmon_ids = nvgpu_css_allocate_perfmon_ids,
		.release_perfmon_ids = nvgpu_css_release_perfmon_ids,
		.get_overflow_status = nvgpu_css_get_overflow_status,
		.get_pending_snapshots = nvgpu_css_get_pending_snapshots,
	},
#endif
	.falcon = {
		.falcon_hal_sw_init = gk20a_falcon_hal_sw_init,
		.falcon_hal_sw_free = gk20a_falcon_hal_sw_free,
	},
	.priv_ring = {
		.enable_priv_ring = gm20b_priv_ring_enable,
		.isr = gp10b_priv_ring_isr,
		.decode_error_code = gp10b_priv_ring_decode_error_code,
		.set_ppriv_timeout_settings =
			gm20b_priv_set_timeout_settings,
		.enum_ltc = gm20b_priv_ring_enum_ltc,
	},
	.fuse = {
		.check_priv_security = gp10b_fuse_check_priv_security,
		.is_opt_ecc_enable = gp10b_fuse_is_opt_ecc_enable,
		.is_opt_feature_override_disable =
			gp10b_fuse_is_opt_feature_override_disable,
		.fuse_status_opt_fbio = gm20b_fuse_status_opt_fbio,
		.fuse_status_opt_fbp = gm20b_fuse_status_opt_fbp,
		.fuse_status_opt_rop_l2_fbp = gm20b_fuse_status_opt_rop_l2_fbp,
		.fuse_status_opt_tpc_gpc = gm20b_fuse_status_opt_tpc_gpc,
		.fuse_ctrl_opt_tpc_gpc = gm20b_fuse_ctrl_opt_tpc_gpc,
		.fuse_opt_sec_debug_en = gm20b_fuse_opt_sec_debug_en,
		.fuse_opt_priv_sec_en = gm20b_fuse_opt_priv_sec_en,
		.read_vin_cal_fuse_rev = NULL,
		.read_vin_cal_slope_intercept_fuse = NULL,
		.read_vin_cal_gain_offset_fuse = NULL,
		.read_gcplex_config_fuse =
				nvgpu_tegra_fuse_read_gcplex_config_fuse,
	},
	.acr = {
		.acr_sw_init = nvgpu_gp10b_acr_sw_init,
	},
	.top = {
		.device_info_parse_enum = gm20b_device_info_parse_enum,
		.device_info_parse_data = gp10b_device_info_parse_data,
		.get_num_engine_type_entries =
					gp10b_get_num_engine_type_entries,
		.get_device_info = gp10b_get_device_info,
		.is_engine_gr = gm20b_is_engine_gr,
		.is_engine_ce = gp10b_is_engine_ce,
		.get_ce_inst_id = NULL,
	},
	.chip_init_gpu_characteristics = gp10b_init_gpu_characteristics,
	.get_litter_value = gp10b_get_litter_value,
};

int gp10b_init_hal(struct gk20a *g)
{
	struct gpu_ops *gops = &g->ops;

	gops->ltc = gp10b_ops.ltc;
	gops->ce2 = gp10b_ops.ce2;
	gops->gr = gp10b_ops.gr;
	gops->gr.ctxsw_prog = gp10b_ops.gr.ctxsw_prog;
	gops->gr.config = gp10b_ops.gr.config;
	gops->fb = gp10b_ops.fb;
	gops->clock_gating = gp10b_ops.clock_gating;
	gops->fifo = gp10b_ops.fifo;
	gops->runlist = gp10b_ops.runlist;
	gops->sync = gp10b_ops.sync;
	gops->netlist = gp10b_ops.netlist;
#ifdef CONFIG_GK20A_CTXSW_TRACE
	gops->fecs_trace = gp10b_ops.fecs_trace;
#endif
	gops->mm = gp10b_ops.mm;
	gops->pramin = gp10b_ops.pramin;
	gops->therm = gp10b_ops.therm;
	gops->pmu = gp10b_ops.pmu;
	gops->clk_arb = gp10b_ops.clk_arb;
	gops->regops = gp10b_ops.regops;
	gops->mc = gp10b_ops.mc;
	gops->debug = gp10b_ops.debug;
#ifdef NVGPU_DEBUGGER
	gops->debugger = gp10b_ops.debugger;
#endif
	gops->perf = gp10b_ops.perf;
	gops->perfbuf = gp10b_ops.perfbuf;
	gops->bus = gp10b_ops.bus;
	gops->ptimer = gp10b_ops.ptimer;
#if defined(CONFIG_GK20A_CYCLE_STATS)
	gops->css = gp10b_ops.css;
#endif
	gops->falcon = gp10b_ops.falcon;

	gops->priv_ring = gp10b_ops.priv_ring;

	gops->fuse = gp10b_ops.fuse;
	gops->acr = gp10b_ops.acr;
	gops->top = gp10b_ops.top;

	/* Lone Functions */
	gops->chip_init_gpu_characteristics =
		gp10b_ops.chip_init_gpu_characteristics;
	gops->get_litter_value = gp10b_ops.get_litter_value;
	gops->semaphore_wakeup = gk20a_channel_semaphore_wakeup;

	nvgpu_set_enabled(g, NVGPU_GR_USE_DMA_FOR_FW_BOOTSTRAP, true);
	nvgpu_set_enabled(g, NVGPU_PMU_PSTATE, false);
	nvgpu_set_enabled(g, NVGPU_FECS_TRACE_VA, false);

	/* Read fuses to check if gpu needs to boot in secure/non-secure mode */
	if (gops->fuse.check_priv_security(g) != 0) {
		return -EINVAL; /* Do not boot gpu */
	}

	/* priv security dependent ops */
	if (nvgpu_is_enabled(g, NVGPU_SEC_PRIVSECURITY)) {
		/* Add in ops from gm20b acr */
		gops->pmu.is_pmu_supported = gm20b_is_pmu_supported,
		gops->pmu.pmu_populate_loader_cfg =
			gm20b_pmu_populate_loader_cfg,
		gops->pmu.flcn_populate_bl_dmem_desc =
			gm20b_flcn_populate_bl_dmem_desc,
		gops->pmu.update_lspmu_cmdline_args =
			gm20b_update_lspmu_cmdline_args;
		gops->pmu.setup_apertures = gm20b_pmu_setup_apertures;
		gops->pmu.secured_pmu_start = gm20b_secured_pmu_start;

		gops->pmu.init_wpr_region = gm20b_pmu_init_acr;
		gops->pmu.load_lsfalcon_ucode = gp10b_load_falcon_ucode;

		gops->gr.load_ctxsw_ucode = gr_gm20b_load_ctxsw_ucode;
	} else {
		/* Inherit from gk20a */
		gops->pmu.is_pmu_supported = gk20a_is_pmu_supported,
		gops->pmu.pmu_setup_hw_and_bootstrap =
			gm20b_ns_pmu_setup_hw_and_bootstrap;
		gops->pmu.pmu_nsbootstrap = pmu_bootstrap,

		gops->pmu.load_lsfalcon_ucode = NULL;
		gops->pmu.init_wpr_region = NULL;

		gops->gr.load_ctxsw_ucode = gr_gk20a_load_ctxsw_ucode;
	}

	nvgpu_set_enabled(g, NVGPU_PMU_FECS_BOOTSTRAP_DONE, false);
	g->pmu_lsf_pmu_wpr_init_done = 0;

	g->name = "gp10b";

	return 0;
}
