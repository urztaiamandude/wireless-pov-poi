/*
 * Battery Monitor Module for POV Poi
 *
 * Monitors battery voltage and current via INA219 module
 * Provides low-battery alerts and runtime estimation
 *
 * Hardware: INA219 I2C voltage/current monitor
 * - Connect INA219 SCL to Teensy pin 19 (I2C0_SCL)
 * - Connect INA219 SDA to Teensy pin 18 (I2C0_SDA)
 * - Connect INA219 GND to system ground
 * - Connect INA219 VCC to system 5V
 *
 * Connection:
 *   - Positive lead: measures voltage from battery to buck regulator
 *   - Load: between INA219 and buck regulator input
 */

#ifndef BATTERY_MONITOR_H
#define BATTERY_MONITOR_H

#include <Wire.h>

// INA219 I2C Address
#define INA219_ADDRESS 0x40  // Default address (A0/A1 pins not connected)

// INA219 Register addresses
#define INA219_REG_CONFIG      0x00
#define INA219_REG_SHUNT_VOLT  0x01
#define INA219_REG_BUS_VOLT    0x02
#define INA219_REG_POWER       0x03
#define INA219_REG_CURRENT     0x04
#define INA219_REG_CALIB       0x05

// Configuration for INA219
// Bits 15-13: Bus voltage range (001 = 32V)
// Bits 12-10: Shunt voltage range (001 = 40mV)
// Bits 9-7: Bus ADC resolution (1111 = 12-bit)
// Bits 6-4: Shunt ADC resolution (1111 = 12-bit)
// Bits 3-0: Mode (0111 = Continuous shunt+bus voltage)
#define INA219_CONFIG 0x399F  // 16-bit value: Bus 32V, Shunt 40mV, 12-bit ADC, Continuous

class BatteryMonitor {
public:
  // Initialization
  bool begin();

  // Voltage readings (in volts)
  float getVoltage();      // Battery voltage
  float getShuntVoltage(); // Voltage across current sense resistor (for diagnostics)

  // Current readings (in amps)
  float getCurrent();      // Current draw
  float getPower();        // Power consumption (watts)

  // Battery status
  float getPercentage();   // Estimated battery remaining (%)
  int getRuntimeMinutes(); // Estimated runtime at current draw
  bool isLowBattery();     // True if voltage < 10.5V (battery exhausted)
  bool isCritical();       // True if voltage < 9.6V (stop immediately)

  // Calibration
  void setCalibration(uint16_t calValue);

private:
  uint16_t readRegister(uint8_t reg);
  void writeRegister(uint8_t reg, uint16_t value);

  // Calibration value (0.04096 / (shunt resistance * max current))
  // Default: 10A max with 0.004 ohm shunt = 4096 (see calibration section)
  uint16_t calibrationValue = 4096;

  // Battery characteristics
  const float batteryCapacity = 1500.0;  // mAh (change based on which module)
  const float nominalVoltage = 11.1;    // 3S LiPo nominal voltage
  const float minVoltage = 9.0;         // 3S LiPo minimum safe voltage
  const float maxVoltage = 12.6;        // 3S LiPo fully charged
};

// Implementation
inline bool BatteryMonitor::begin() {
  Wire.begin();
  Wire.setClock(100000); // 100kHz I2C speed

  // Check if INA219 is present
  Wire.beginTransmission(INA219_ADDRESS);
  uint8_t error = Wire.endTransmission();
  if (error != 0) {
    Serial.println("INA219 not found at 0x40");
    return false;
  }

  // Configure INA219
  writeRegister(INA219_REG_CONFIG, INA219_CONFIG);

  // Set calibration register
  // For 10A max with 0.004 ohm shunt: Cal = 4096
  // Formula: Cal = 0.04096 / (Rshunt * I_max)
  //   = 0.04096 / (0.004 * 10) = 1024
  // We use 4096 as a practical value for most hobby LiPo setups
  writeRegister(INA219_REG_CALIB, calibrationValue);

  delay(10);
  Serial.println("INA219 initialized successfully");
  return true;
}

inline uint16_t BatteryMonitor::readRegister(uint8_t reg) {
  Wire.beginTransmission(INA219_ADDRESS);
  Wire.write(reg);
  Wire.endTransmission();

  Wire.requestFrom(INA219_ADDRESS, 2);
  uint16_t value = 0;
  if (Wire.available() >= 2) {
    value = (uint16_t)Wire.read() << 8;
    value |= Wire.read();
  }
  return value;
}

inline void BatteryMonitor::writeRegister(uint8_t reg, uint16_t value) {
  Wire.beginTransmission(INA219_ADDRESS);
  Wire.write(reg);
  Wire.write((value >> 8) & 0xFF);
  Wire.write(value & 0xFF);
  Wire.endTransmission();
}

inline float BatteryMonitor::getVoltage() {
  uint16_t raw = readRegister(INA219_REG_BUS_VOLT);
  // Voltage is in bits 15-3, LSB = 4mV
  float voltage = (raw >> 3) * 0.004;
  return voltage;
}

inline float BatteryMonitor::getShuntVoltage() {
  int16_t raw = (int16_t)readRegister(INA219_REG_SHUNT_VOLT);
  // Shunt voltage is 16-bit, LSB = 0.01mV
  float voltage = raw * 0.00001;
  return voltage;
}

inline float BatteryMonitor::getCurrent() {
  // Current is register value * LSB
  // With calibration register set, LSB = 2.56mA per bit typical
  // Actual value depends on calibration
  int16_t raw = (int16_t)readRegister(INA219_REG_CURRENT);
  // LSB = 0.1A per bit (with 4096 calibration value)
  float current = raw * 0.001; // Rough approximation
  return current;
}

inline float BatteryMonitor::getPower() {
  uint16_t raw = readRegister(INA219_REG_POWER);
  // Power = register value * 20mW per bit (with 4096 calibration)
  float power = raw * 0.020;
  return power;
}

inline float BatteryMonitor::getPercentage() {
  float voltage = getVoltage();

  // Simple linear approximation for 3S LiPo
  // 12.6V = 100%, 9.0V = 0%
  if (voltage >= maxVoltage) return 100.0;
  if (voltage <= minVoltage) return 0.0;

  float percentage = ((voltage - minVoltage) / (maxVoltage - minVoltage)) * 100.0;
  return constrain(percentage, 0.0, 100.0);
}

inline int BatteryMonitor::getRuntimeMinutes() {
  float current = getCurrent();
  if (current < 0.1) return -1; // Not measuring properly

  float percentage = getPercentage();
  float remainingCapacity = (batteryCapacity / 100.0) * percentage;

  // Runtime in minutes = (remaining mAh) / (current in mA)
  int runtime = (remainingCapacity / (current * 1000.0)) * 60;
  return max(0, runtime);
}

inline bool BatteryMonitor::isLowBattery() {
  // 3S LiPo should not go below 10.5V (3.5V per cell)
  // Below this, capacity is effectively depleted
  return getVoltage() < 10.5;
}

inline bool BatteryMonitor::isCritical() {
  // 3S LiPo should not go below 9.6V (3.2V per cell)
  // Below this, battery may be damaged
  return getVoltage() < 9.6;
}

inline void BatteryMonitor::setCalibration(uint16_t calValue) {
  calibrationValue = calValue;
  writeRegister(INA219_REG_CALIB, calibrationValue);
}

#endif // BATTERY_MONITOR_H
