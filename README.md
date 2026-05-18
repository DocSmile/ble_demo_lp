# BLE Dynamic TX Power Beacon — Multi-Board
# Targets: nRF54L15DK | Ezurio BL54L15 DVK | u-blox NORA-B206 EVK
# NCS v3.3.0

## Project structure

```
ble_txpower_multiboard/
├── CMakeLists.txt                                   ← shared, unchanged for all boards
├── prj.conf                                         ← shared Kconfig for all boards
├── west.yml                                         ← NCS v3.3.0 + u-blox OpenCPU BSP
├── README.md
├── boards/
│   ├── nrf54l15dk_nrf54l15_cpuapp.overlay          ← Nordic nRF54L15DK
│   ├── bl54l15_dvk_nrf54l15_cpuapp.overlay         ← Ezurio BL54L15 DVK
│   └── ubx_evknorab2_nrf54l15_cpuapp.overlay       ← u-blox NORA-B206 EVK
└── src/
    ├── main.c                                       ← unchanged
    ├── ble.c                                        ← unchanged
    └── ble.h                                        ← unchanged
```

The application source files are IDENTICAL across all three boards.
Only the board overlay and the `-b` build flag change between targets.

---

## One-time workspace setup (NCS v3.3.0)

```bash
west init -m https://github.com/nrfconnect/sdk-nrf --mr v3.3.0 ~/ncs/v3.3.0
cd ~/ncs/v3.3.0
west update
```

For the NORA-B206 target only, the u-blox OpenCPU BSP is fetched
automatically by `west update` via the west.yml entry above.

---

## Build commands

### Nordic nRF54L15DK
BSP is built into NCS — no extra steps needed.

```bash
cd /path/to/ble_txpower_multiboard

west build -b nrf54l15dk/nrf54l15/cpuapp --sysbuild
west flash
```

---

### Ezurio BL54L15 DVK
BSP (`bl54l15_dvk`) is included in NCS v3.3.0 and Zephyr mainline.
No extra steps needed.

```bash
cd /path/to/ble_txpower_multiboard

west build -b bl54l15_dvk/nrf54l15/cpuapp --sysbuild
west flash
```

> If the board shows readback protection on first flash:
> ```bash
> nrfjprog --recover --family NRF54L
> west flash
> ```

---

### u-blox NORA-B206 (EVK-NORA-B2)
The `ubx_evknorab2` BSP is NOT in NCS mainline. It lives in the
u-blox OpenCPU GitHub repo. west.yml fetches it automatically,
but you must tell the build system where to find it:

```bash
cd /path/to/ble_txpower_multiboard

west build -b ubx_evknorab2/nrf54l15/cpuapp --sysbuild \
  -DBOARD_ROOT=~/ncs/v3.3.0/u-blox-sho-OpenCPU

west flash
```

> On first use with a fresh NORA-B206 module (readback protection):
> ```bash
> nrfjprog --recover --family NRF54L
> west flash
> ```

> LFXO note: NORA-B206 modules do NOT include an external 32.768kHz
> crystal. The EVK board does. If deploying to a bare module, follow
> u-blox App Note "RC oscillator configuration for nRF5 open CPU modules"
> to switch from LFXO to LFRC in the device tree.

---

## Serial console

All three boards expose a USB CDC virtual COM port via their onboard
J-Link debugger. Connect at **115200 baud, 8N1**.

- Windows: COMxx (appears in Device Manager as "JLink CDC UART Port")
- Linux:   /dev/ttyACM0 (or ttyACM1)
- macOS:   /dev/tty.usbmodemXXXXX

Use nRF Connect Serial Terminal, PuTTY, or:
```bash
screen /dev/ttyACM0 115200
```

---

## What the application does

- Initialises the BT stack and sets advertiser TX power to **-8 dBm**
- Starts connectable advertising (2400ms interval) at ticker == 0
- Stops advertising at ticker == 16
- Ticker wraps every 24 steps (48 seconds total cycle)
- Prints TX power for any incoming BLE connection
- On disconnect, restarts the advertising cycle

---

## Why only the overlay changes between boards

All three modules use the same Nordic nRF54L15 SoC. The `prj.conf`
Kconfig and `src/` files are completely hardware-agnostic. The board
overlay is the only thing that differs because it tells Zephyr:
  - Which UART peripheral to use for the console
  - Which physical pins that UART is connected to on that specific PCB

NCS automatically selects the matching overlay by filename convention:
  `boards/<board_name>.overlay` is picked up when building for that target.
