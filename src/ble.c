
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_vs.h>

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include "ble.h"

/* NCS 3.3.0 / Zephyr 3.7+: BT_HCI_VS_EXT was merged into BT_HCI_VS. */
BUILD_ASSERT(IS_ENABLED(CONFIG_BT_HCI_VS),
	     "This app requires Zephyr-specific HCI vendor extensions");

static struct bt_conn *default_conn;
static uint16_t default_conn_handle;

/* Advertising data — name moved into ad[] to eliminate scan response radio exchange */
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_HRS_VAL)),
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME,
		sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

#define DEVICE_NAME     CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

/* 2400ms advertising interval — conservative, saves radio-on time.
 * Non-connectable (BT_LE_ADV_OPT_NONE) — no scan window listening between events.
 * If connections are required, change to BT_LE_ADV_OPT_CONN. */
/* Interval set via CONFIG_APP_ADV_INTERVAL_MS in prj.conf */
#define ADV_INTERVAL_MS  (CONFIG_APP_ADV_INTERVAL_MS * 8 / 5)  /* 2400ms in units of 0.625ms */

static const struct bt_le_adv_param *param = BT_LE_ADV_PARAM(
	BT_LE_ADV_OPT_NONE,
	ADV_INTERVAL_MS,
	ADV_INTERVAL_MS,
	NULL);

/* TX power level used for both advertising and connections */
/* TX power set via CONFIG_APP_TX_POWER_DBM in prj.conf */
#define APP_TX_POWER_DBM  (CONFIG_APP_TX_POWER_DBM)

static void set_tx_power(uint8_t handle_type, uint16_t handle, int8_t tx_pwr_lvl)
{
	struct bt_hci_cp_vs_write_tx_power_level *cp;
	struct bt_hci_rp_vs_write_tx_power_level *rp;
	struct net_buf *buf, *rsp = NULL;
	int err;

	buf = bt_hci_cmd_alloc(K_FOREVER);
	if (!buf) {
		printk("Unable to allocate command buffer\n");
		return;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(handle);
	cp->handle_type = handle_type;
	cp->tx_power_level = tx_pwr_lvl;

	err = bt_hci_cmd_send_sync(BT_HCI_OP_VS_WRITE_TX_POWER_LEVEL, buf, &rsp);
	if (err) {
		printk("Set Tx power err: %d\n", err);
		return;
	}

	rp = (void *)rsp->data;
	printk("Actual Tx Power: %d dBm\n", rp->selected_tx_power);

	net_buf_unref(rsp);
}

static void get_tx_power(uint8_t handle_type, uint16_t handle, int8_t *tx_pwr_lvl)
{
	struct bt_hci_cp_vs_read_tx_power_level *cp;
	struct bt_hci_rp_vs_read_tx_power_level *rp;
	struct net_buf *buf, *rsp = NULL;
	int err;

	*tx_pwr_lvl = 0xFF;
	buf = bt_hci_cmd_alloc(K_FOREVER);
	if (!buf) {
		printk("Unable to allocate command buffer\n");
		return;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(handle);
	cp->handle_type = handle_type;

	err = bt_hci_cmd_send_sync(BT_HCI_OP_VS_READ_TX_POWER_LEVEL, buf, &rsp);
	if (err) {
		printk("Read Tx power err: %d\n", err);
		return;
	}

	rp = (void *)rsp->data;
	*tx_pwr_lvl = rp->tx_power_level;

	net_buf_unref(rsp);
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];
	int8_t txp;
	int ret;

	if (err) {
		printk("Connection failed, err 0x%02x %s\n", err,
		       bt_hci_err_to_str(err));
	} else {
		default_conn = bt_conn_ref(conn);
		ret = bt_hci_get_conn_handle(default_conn, &default_conn_handle);
		if (ret) {
			printk("No connection handle (err %d)\n", ret);
		} else {
			bt_addr_le_to_str(bt_conn_get_dst(conn), addr,
					  sizeof(addr));
			printk("Connected via connection (%d) at %s\n",
			       default_conn_handle, addr);

			/* Set connection TX power to same level as advertising
			 * — avoids NO_PREF defaulting to max power */
			set_tx_power(BT_HCI_VS_LL_HANDLE_TYPE_CONN,
				     default_conn_handle, APP_TX_POWER_DBM);
			get_tx_power(BT_HCI_VS_LL_HANDLE_TYPE_CONN,
				     default_conn_handle, &txp);
			printk("Connection (%d) - Tx Power = %d dBm\n",
			       default_conn_handle, txp);
		}
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected, reason 0x%02x %s\n", reason,
	       bt_hci_err_to_str(reason));

	if (default_conn) {
		bt_conn_unref(default_conn);
		default_conn = NULL;
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static void bt_ready(int err)
{
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}
	printk("Bluetooth initialized\n");
}

void BLE_Init(void)
{
	int err;
	int8_t tx_pwr_lvl_av = 0;

	default_conn = NULL;
	printk("Starting Dynamic Tx Power Beacon Demo (Low Power)\n");

	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	/* Set advertising TX power */
	set_tx_power(BT_HCI_VS_LL_HANDLE_TYPE_ADV, 0, APP_TX_POWER_DBM);
	get_tx_power(BT_HCI_VS_LL_HANDLE_TYPE_ADV, 0, &tx_pwr_lvl_av);
	printk("Advertising Tx Power set to %d dBm\n", tx_pwr_lvl_av);
}

void BLE_AdvertisingStart(void)
{
	int err;

	err = bt_le_adv_start(param, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}
	printk("Advertising started\n");
}

void BLE_AdvertisingStop(void)
{
	bt_le_adv_stop();
	printk("Advertising stopped\n");
}
