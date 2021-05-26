#ifndef __LINUX_HS_MODEM_RATE_H_
#define __LINUX_HS_MODEM_RATE_H_
/* ----------------------------
 *           includes
 * ----------------------------
 */
#include <linux/types.h>
/* ----------------------------
 *            types
 * ----------------------------
 */

typedef struct modem_rx_param_t
{
	uint32_t rx_frame_bytes;
	uint32_t rx_freq_conv;
	uint32_t rx_cic_ratio;
	uint32_t rx_cic_scale;
	uint32_t rx_resamp_frac;
	uint32_t rx_resamp_int;
	uint32_t rxs_num_taps;
	uint32_t rxs_spread;
	uint32_t rxs_cor_delay;
	uint32_t rxs_dyn_scl_to;
	uint32_t rxs_win_delay;
	uint32_t rxs_det_win;
	uint32_t rxd_det_offset;
	uint32_t rxd_dss_factor;
	uint32_t rxd_dat_sym;
	uint32_t rxd_tcc_block;
	uint32_t rxd_cod_blk_len;
} modem_rx_param_t;

typedef struct modem_tx_param_t
{
	uint32_t tx_frame_bytes;
	uint32_t tx_frame_len;
	uint32_t tx_enc_blk_size;
	uint32_t tx_dss_factor;
	uint32_t tx_shap_int_clk;
	uint32_t tx_resamp_frac;
	uint32_t tx_resamp_int;
	uint32_t tx_cic_int_rate;
	uint32_t tx_cic_scale;
} modem_tx_param_t;
 
void hsModem_rx_rate_default(modem_rx_param_t* param);
void hsModem_tx_rate_default(modem_tx_param_t* param);

#endif /* __LINUX_HS_MODEM_RATE_H_ */
