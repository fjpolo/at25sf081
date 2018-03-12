#include "AT25SF081.h"

#include <functional>
#include <iostream>
#include <cassert>
#include <cstring>

#define OP_BYTE_OR_PAGE_PROGRAM (0x02)
#define OP_READ_ARRAY_03 (0x03)
#define OP_READ_ARRAY_0B (0x0b)
#define OP_WRITE_ENABLE (0x06)
#define OP_WRITE_DISABLE (0x04)
#define OP_BLOCK_ERASE_4KB (0x20)
#define OP_BLOCK_ERASE_32KB (0x52)
#define OP_BLOCK_ERASE_64KB (0xd8)
#define OP_CHIP_ERASE_60 (0x60)
#define OP_CHIP_ERASE_C7 (0xc7)
#define OP_READ_STATUS_REGISTER_BYTE_1 (0x05)
#define OP_READ_STATUS_REGISTER_BYTE_2 (0x35)
#define OP_READ_MANUFACTURER_AND_DEVICE_ID (0x9f)
#define OP_READ_DEVICE_ID (0x90)


#define MAX_FREQUENCY (104000000)
#define RDHF_FREQUENCY (70000000)

AT25SF081::AT25SF081(int peripheral_id) :
        should_stop_(false),
        status_register1_(0),
        status_register2_(0) {
    int cs_pin_number = GetPinNumber("cs");
    int sck_pin_number = GetPinNumber("sck");
    int si_pin_number = GetPinNumber("si");
    int so_pin_number = GetPinNumber("so");
    wp_pin_number_ = GetPinNumber("wp");
    hold_pin_number_ = GetPinNumber("hold");

    SpiSlaveConfig spi_config = {
            .mosi_pin_number = si_pin_number,
            .miso_pin_number = so_pin_number,
            .ss_pin_number = cs_pin_number,
            .sclk_pin_number = sck_pin_number,
            .supported_spi_modes = SPI_MODE_0 | SPI_MODE_3,
            .max_frequency = MAX_FREQUENCY,
            .bit_order = MSB_FIRST
    };

    spi_slave_ = CreateSpiSlave(spi_config);
//    memory_ = CreateMemory(kMemorySize_, 0xff);
    MemReset();
}

void AT25SF081::Main() {
    while (!should_stop_) {
        uint8_t opcode = 0;
//    spi_slave_.SetMaxFrequency(MAX_FREQUENCY);
        if (spi_slave_->Read(&opcode, 1) == 0) {
//        std::cout << "[SLAVE] ExecSingleOp READ was false => return false" << std::endl;
            continue;
        }

        switch (opcode) {
            case (OP_READ_ARRAY_0B): {
//            std::cout << "[SLAVE] ExecSingleOp OP_READ_ARRAY_0B" << std::endl;
                ReadArray(true);
                break;
            }

            case (OP_READ_ARRAY_03): {
                ReadArray(false);
                break;
            }

            case (OP_BYTE_OR_PAGE_PROGRAM): {
//            std::cout << "[SLAVE] ExecSingleOp OP_BYTE_OR_PAGE_PROGRAM" << std::endl;
                ByteOrPageProgram();
                break;
            }

            case (OP_WRITE_ENABLE): {
                WriteEnable();
                break;
            }

            case (OP_WRITE_DISABLE): {
                WriteDisable();
                break;
            }

            case (OP_BLOCK_ERASE_4KB): {
                BlockErase(1U << 12);
                break;
            }

            case (OP_BLOCK_ERASE_32KB): {
                BlockErase(1U << 15);
                break;
            }

            case (OP_BLOCK_ERASE_64KB): {
                BlockErase(1U << 16);
                break;
            }

            case (OP_CHIP_ERASE_60): {
                ChipErase();
                break;
            }

            case (OP_CHIP_ERASE_C7): {
                ChipErase();
                break;
            }

            case (OP_READ_STATUS_REGISTER_BYTE_1): {
                ReadStatusRegister(STATUS_REGISTER_BYTE_1);
                break;
            }

            case (OP_READ_STATUS_REGISTER_BYTE_2): {
                ReadStatusRegister(STATUS_REGISTER_BYTE_2);
                break;
            }

            case (OP_READ_MANUFACTURER_AND_DEVICE_ID): {
                ReadManufacturerAndDeviceId();
                break;
            }

            case (OP_READ_DEVICE_ID): {
                ReadDeviceId();
                break;
            }

            default: {
                throw std::logic_error("AT25SF081: Unsupported opcode " + std::to_string(opcode));
            }
        }
    }
}

void AT25SF081::ReadArray(bool with_dummy_byte) {
//    spi_slave_.SetMaxFrequency(RDHF_FREQUENCY);
    int address = ReadAddress();
    if (address < 0) return;

    if (with_dummy_byte) {
        if (ReadDummyBytes(1) != 1 || !spi_slave_->IsSsActive()) return;
    }

    bool done = false;
    while (!done) {
        uint8_t value = memory_[address];
        size_t length = spi_slave_->Write(&value, 1);
        if (length == 0) {
            return;
        }
        address = (address + 1) % (int) kMemorySize_;
        done = !spi_slave_->IsSsActive();
    }
}

void AT25SF081::ByteOrPageProgram() {
    if (!WelStatus()) {
        throw std::logic_error("AT25SF081: Trying to program the device while WEL bit in status register is 0");
    }
    int address = ReadAddress();
    if (address < 0) return;

    bool done = false;
    while (!done) {
        uint8_t byte = 0;
        size_t length = spi_slave_->Read(&byte, 1);
        if (length == 0) {
            return;
        }
        memory_[address] = byte;
        address = (address & ~0xff) | ((address + 1) & 0xff);
        done = !spi_slave_->IsSsActive();
    }

    SetWel(0);
}

int AT25SF081::ReadAddress() {
//    std::cout << "[SLAVE] ReadAddress" << std::endl;
    uint8_t data[3];
    if (spi_slave_->Read(data, 3) != 3 || !spi_slave_->IsSsActive()) {
//        std::cout << "[SLAVE] ReadAddress return -1" << std::endl;
        return -1;
    }

//    std::cout << "[SLAVE] ReadAddress return address" << std::endl;
    return ((data[0] << 16) | (data[1] << 8) | data[2]) % (int) kMemorySize_;
}

size_t AT25SF081::ReadDummyBytes(size_t num) {
    uint8_t data[num];
    return spi_slave_->Read(data, num);
}

void AT25SF081::SetWel(bool enable) {
    if (enable) {
        status_register1_ |= kStatusRegisterBitWelMask_;
    } else {
        status_register1_ &= ~kStatusRegisterBitWelMask_;
    }
}

bool AT25SF081::WelStatus() {
    return (status_register1_ & kStatusRegisterBitWelMask_) != 0;
}

void AT25SF081::BlockErase(size_t block_size) {
    if (!WelStatus()) {
        throw std::logic_error("AT25SF081: Trying to erase block while WEL bit in status register is 0");
    }

    int address = ReadAddress();
    if (address == -1)
        return;

    address = ((unsigned int) block_size) * (address / (unsigned int) block_size);

    IgnoreUntilSsInactive();

    memset(memory_ + address, 0xff, block_size);
    SetWel(true);
}

void AT25SF081::ChipErase() {
    if (!WelStatus()) {
        throw std::logic_error("AT25SF081: Trying to erase chip while WEL bit in status register is 0");
    }
    IgnoreUntilSsInactive();
    MemReset();
}

void AT25SF081::MemReset() {
    memset(memory_, 0xff, kMemorySize_);
}

void AT25SF081::WriteEnable() {
    while (spi_slave_->IsSsActive()) {
        ReadDummyBytes(1);
    };
    SetWel(true);
}

void AT25SF081::WriteDisable() {
    while (spi_slave_->IsSsActive()) {
        ReadDummyBytes(1);
    };
    SetWel(false);
}

void AT25SF081::IgnoreUntilSsInactive() {
    while (spi_slave_->IsSsActive()) {
        ReadDummyBytes(1);
    };
}

void AT25SF081::ReadStatusRegister(AT25SF081::StatusRegisterByte byte) {
    uint8_t data = byte == STATUS_REGISTER_BYTE_1 ? status_register1_ : status_register2_;

    while (spi_slave_->IsSsActive()) {
        spi_slave_->Write(&data, 1);
    }
}

void AT25SF081::ReadManufacturerAndDeviceId() {
    static uint8_t response[3] = {kManufacturerId_, kDeviceIdPart1_, kDeviceIdPart2_};
    spi_slave_->Write(response, sizeof(response));
    IgnoreUntilSsInactive();
}

void AT25SF081::ReadDeviceId() {
    static uint8_t response[2] = {kManufacturerId_, kDeviceCode_};
    int i = 0;
    ReadDummyBytes(3);
    while (spi_slave_->IsSsActive()) {
        spi_slave_->Write(&response[i], 1);
        i = (i + 1) % 2;
    }
}
