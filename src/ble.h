
#ifndef __BLE_H__
#define __BLE_H__

#include <zephyr/types.h>
#include <stddef.h>

void BLE_Init(void);
void BLE_AdvertisingStart(void);
void BLE_AdvertisingStop(void);

#endif
