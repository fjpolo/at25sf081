#include "ModelingFramework.h"
#include <cassert>
#include <utility>
#include "iJemuInterface.h"

iJemuInterface *jemu_interface_ = nullptr;

extern "C" void InitJemuInterface(iJemuInterface *jemu_interface) {
    jemu_interface_ = jemu_interface;
}

int GetPinNumber(const std::string &pin_name) {
    assert(jemu_interface_);
    return jemu_interface_->GetPinNumber(pin_name.c_str());
}

iSpiSlave* CreateSpiSlave(SpiSlaveConfig &spi_config) {
    assert(jemu_interface_);
    return jemu_interface_->CreateSpiSlave(spi_config);
}

bool GetPinLevel(int pin_number) {
    assert(jemu_interface_);
    return jemu_interface_->GetPinLevel(pin_number);
}

bool GetPinLevel(const std::string &pin_name) {
    assert(jemu_interface_);
    const auto pin_number = jemu_interface_->GetPinNumber(pin_name.c_str());
    return jemu_interface_->GetPinLevel(pin_number);    
}

void SetPinLevel(int pin_number, bool pin_level) {
    assert(jemu_interface_);
    jemu_interface_->SetPinLevel(pin_number, pin_level);
}

void SetPinLevel(const std::string &pin_name, bool pin_level) {
    assert(jemu_interface_);
    const auto pin_number = jemu_interface_->GetPinNumber(pin_name.c_str());
    jemu_interface_->SetPinLevel(pin_number, pin_level);
}

int32_t GetNextInt32FromDataGenerator(std::string name) {
    assert(jemu_interface_);
    return jemu_interface_->GetNextInt32FromDataGenerator(name.c_str());
}

double GetNextDoubleFromDataGenerator(std::string name) {
    assert(jemu_interface_);
    return jemu_interface_->GetNextDoubleFromDataGenerator(name.c_str());
}

int16_t GetNextInt16FromDataGenerator(std::string name) {
    assert(jemu_interface_);
    return jemu_interface_->GetNextInt16FromDataGenerator(name.c_str());
}

uint8_t GetNextUInt8FromDataGenerator(std::string name) {
    assert(jemu_interface_);
    return jemu_interface_->GetNextUInt8FromDataGenerator(name.c_str());
}

double GetCachedValueFromDataGenerator(std::string name) {
    assert(jemu_interface_);
    return jemu_interface_->GetCachedValueFromDataGenerator(name.c_str());
}

int AddTimedCallback(uint64_t ns, const callback_t &callback,  bool run_once) {
    assert(jemu_interface_);
    return jemu_interface_->AddTimedCallback(ns, callback, run_once);
}

void CancelTimedCallback(int id) {
    assert(jemu_interface_);
    jemu_interface_->CancelTimedCallback(id);
}

void UpdateTimedCallback(int id, uint64_t ns) {
    assert(jemu_interface_);
    jemu_interface_->UpdateTimedCallback(id,ns);
}
