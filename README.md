# BLE Dynamic TX Power Beacon — Low Power Multi-Board
## Targets: nRF54L15DK | Ezurio BL54L15 DVK | u-blox NORA-B206 EVK
## NCS v3.3.0

## Overview

This is a low power optimized BLE TX power beacon demo for the Nordic nRF54L15.
It advertises for a configurable duration, then enters low power sleep, and repeats.
All timing and power settings are configurable in `prj.conf` — no source code changes needed.

---

## Customer Configurable Settings (prj.conf)

| Parameter | Config Symbol | Default | Description |
|---|---|---|---|
| Advertising Interval | `CONFIG_APP_ADV_INTERVAL_MS` | 2400 ms | Time between advertising events (20-10240ms) |
| Advertising Duration | `CONFIG_APP_ADV_DURATION_MS` | 15000 ms | How long device advertises per cycle |
| Sleep Duration | `CONFIG_APP_SLEEP_DURATION_MS` | 5000 ms | Low power sleep between cycles |
| TX Power | `CONFIG_APP_TX_POWER_DBM` | 0 dBm | BLE TX power (3, 0, -4, -8, -12, -16, -20 dBm) |

---

## Power Optimizations vs Standard Demo

- **Event-driven main loop** — CPU wakes only to start/stop advertising
- **Non-connectable advertising** — no scan window listening between ad events
- **RTT console** instead of UART — removes continuous peripheral clock drain
- **Consistent TX power** — connection TX power matches advertising power
- **Reduced BT stack** — SMP, GATT client, dynamic DB disabled
- **Reduced heap/stack** — 4KB heap, 1KB workqueue stack

---

## Project Structure

    ble_demo_lp/
    ├── CMakeLists.txt
    ├── Kconfig                                      ← app-level Kconfig definitions
    ├── prj.conf                                     ← customer timing & power settings
    ├── sysbuild.conf                                ← disables deprecated partition manager
    ├── west.yml
    ├── README.md
    ├── boards/
    │   ├── nrf54l15dk_nrf54l15_cpuapp.overlay      ← Nordic nRF54L15DK
    │   ├── bl54l15_dvk_nrf54l15_cpuapp.overlay     ← Ezurio BL54L15 DVK
    │   └── ubx_evknorab2_nrf54l15_cpuapp.overlay   ← u-blox NORA-B206 EVK
    └── src/
        ├── main.c                                   ← event-driven cycle loop
        ├── ble.c                                    ← BLE init, advertise, TX power
        └── ble.h

---

## Build Commands (NCS v3.3.0)

### Nordic nRF54L15DK
    cd /path/to/ble_demo_lp
    /opt/nordic/ncs/toolchains/0c0f19d91c/bin/cmake -B build -DBOARD=nrf54l15dk/nrf54l15/cpuapp -DCONF_FILE=prj.conf -DCONFIG_DEBUG_THREAD_INFO=y .
    /opt/nordic/ncs/toolchains/0c0f19d91c/bin/cmake --build build

### Ezurio BL54L15 DVK
    /opt/nordic/ncs/toolchains/0c0f19d91c/bin/cmake -B build_ezurio -DBOARD=bl54l15_dvk/nrf54l15/cpuapp -DCONF_FILE=prj.conf -DCONFIG_DEBUG_THREAD_INFO=y .
    /opt/nordic/ncs/toolchains/0c0f19d91c/bin/cmake --build build_ezurio

Note: Ezurio board files must be copied from Zephyr mainline into NCS 3.3.0:

    cp -r ~/Downloads/zephyr-main/boards/ezurio/bl54l15_dvk /opt/nordic/ncs/v3.3.0/zephyr/boards/ezurio/

### u-blox NORA-B206
    /opt/nordic/ncs/toolchains/0c0f19d91c/bin/cmake -B build_ublox -DBOARD=ubx_evknorab2/nrf54l15/cpuapp -DCONF_FILE=prj.conf -DCONFIG_DEBUG_THREAD_INFO=y .
    /opt/nordic/ncs/toolchains/0c0f19d91c/bin/cmake --build build_ublox

Note: u-blox board files require a one-time patch for NCS 3.3.0:

    sudo cp -r ~/Downloads/u-blox-sho-OpenCPU/zephyr/boards/u-blox/ubx_evknorab2 /opt/nordic/ncs/v3.3.0/zephyr/boards/u-blox/
    sudo sed -i '' 's/nrf54l15_partition.dtsi/nrf54l15_cpuapp_partition.dtsi/' /opt/nordic/ncs/v3.3.0/zephyr/boards/u-blox/ubx_evknorab2/ubx_evknorab2_nrf54l15_cpuapp.dts

---

## Flash Commands

    west flash --build-dir build          # Nordic DK
    west flash --build-dir build_ezurio   # Ezurio
    west flash --build-dir build_ublox    # u-blox

If readback protection is enabled on first flash:

    nrfjprog --recover --family NRF54L
    west flash --build-dir build

---

## Serial Console

Uses RTT instead of UART for near-zero power cost. View output using:
- nRF Connect Serial Terminal (RTT mode)
- J-Link RTT Viewer
- JLinkRTTClient from command line

---

## TX Power Reference

| dBm | Use Case |
|---|---|
| +3  | Maximum range |
|  0  | Default, good balance |
| -4  | Short range, lower power |
| -8  | Very short range |
| -12 | Proximity only |
| -16 | Minimal range |
| -20 | Lowest power |

---

## LFXO Note (u-blox NORA-B206)

NORA-B206 modules do NOT include an external 32.768kHz crystal on the module.
The EVK board does. If deploying to a bare module, configure LFRC in the device tree.
