#!/bin/bash

CMAKE=/opt/nordic/ncs/toolchains/0c0f19d91c/bin/cmake
APP=~/Documents/nrf/ble_demo_lp

echo "Building Nordic DK..."
rm -rf $APP/build
$CMAKE -B $APP/build -DBOARD=nrf54l15dk/nrf54l15/cpuapp -DCONF_FILE=prj.conf -DCONFIG_DEBUG_THREAD_INFO=y $APP
$CMAKE --build $APP/build

echo "Building Ezurio BL54L15..."
rm -rf $APP/build_ezurio
$CMAKE -B $APP/build_ezurio -DBOARD=bl54l15_dvk/nrf54l15/cpuapp -DCONF_FILE=prj.conf -DCONFIG_DEBUG_THREAD_INFO=y $APP
$CMAKE --build $APP/build_ezurio

echo "Building u-blox NORA-B206..."
rm -rf $APP/build_ublox
$CMAKE -B $APP/build_ublox -DBOARD=ubx_evknorab2/nrf54l15/cpuapp -DCONF_FILE=prj.conf -DCONFIG_DEBUG_THREAD_INFO=y $APP
$CMAKE --build $APP/build_ublox

echo "All builds complete!"
