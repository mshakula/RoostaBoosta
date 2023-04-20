/// \file pinout.hpp
/// \date 2023-04-19
/// \author mshakula (matvey@gatech.edu)
///
/// \brief Defines the pinout for the peripheral devices for hw1.0.

#ifndef RB_PINOUT_HPP
#define RB_PINOUT_HPP

#ifndef __cplusplus
#error "pinout.hpp is a cxx-only header."
#endif // __cplusplus

#include <PinNames.h>

// ======================= Public Interface ==========================

namespace rb {
namespace pinout {

constexpr int kVersionMajor = 1;
constexpr int kVersionMinor = 0;

// SD Card Pins.
constexpr PinName kSD_mosi = p5;
constexpr PinName kSD_miso = p6;
constexpr PinName kSD_sck  = p7;
constexpr PinName kSD_cs   = p8;

// Bluetooth Pins.
constexpr PinName kBT_tx = p9;
constexpr PinName kBT_rx = p10;

// Sonar Pins.
constexpr PinName kSonar_trig = p11;
constexpr PinName kSonar_echo = p12;

// LCD Pins.
constexpr PinName kLCD_tx  = p13;
constexpr PinName kLCD_rx  = p14;
constexpr PinName kLCD_res = p15;

// Audio Pins.
constexpr PinName kAudio_out = p18;

// Button Pins.
constexpr PinName kBtn5 = p22;
constexpr PinName kBtn4 = p23;
constexpr PinName kBtn3 = p24;
constexpr PinName kBtn2 = p25;
constexpr PinName kBtn1 = p26;

// Wifi Pins.
constexpr PinName kWifi_rx = p27;
constexpr PinName kWifi_tx = p28;

// Ethernet Pins automatically defined and used in ethernet creation by mbed-os.
// constexpr PinName kEther_tdp = p33;
// constexpr PinName kEther_tdm = p34;
// constexpr PinName kEther_rdp = p35;
// constexpr PinName kEther_rdm = p36;

} // namespace pinout
} // namespace rb

// ===================== Detail Implementation =======================

#endif // RB_PINOUT_HPP
