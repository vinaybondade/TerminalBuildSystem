/* ----------------------------
 *           includes
 * ----------------------------
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/io.h>
#include "hs_modem_rate.h"
#include "generalDef.h"
#include "hs_modem_hw.h"

void hsModem_rx_rate_default(modem_rx_param_t* param)
{
	param->rx_frame_bytes = 74;
	param->rxs_spread = 16;
	param->rxs_cor_delay = 31;
	param->rxs_dyn_scl_to = 255;
	param->rxs_win_delay = 8191;
	param->rxs_det_win = 31;
	param->rxd_det_offset = 8194;
	param->rxd_dss_factor = 15;
	param->rxd_dat_sym = 1787;
	param->rxd_tcc_block = 592;
	param->rxd_cod_blk_len = 1788;
	param->rx_cic_ratio = 7;
	param->rx_cic_scale = 9;
	param->rx_resamp_frac = 0xf4000000;
	param->rx_resamp_int = 1;
	param->rxs_num_taps = 3;
}

void hsModem_tx_rate_default(modem_tx_param_t* param)
{
	param->tx_frame_bytes = 74;
	param->tx_frame_len = 73;
	param->tx_enc_blk_size = 592;
	param->tx_dss_factor = 15;
	param->tx_shap_int_clk = 37;
	param->tx_resamp_frac = 0x83126e97;
	param->tx_resamp_int = 0;
	param->tx_cic_int_rate = 7;
	param->tx_cic_scale = 0;
}