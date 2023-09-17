/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#include "wm8750.hpp"

#include <cassert>
#include <cstdio>

namespace psp::wm8750 {

namespace CodecReg {
    enum : u8 {
        LINVolume = 0x00,
        RINVolume = 0x01,
        LOUT1Volume = 0x02,
        ROUT1Volume = 0x03,
        DACControl = 0x05,
        AudioInterface = 0x07,
        SampleRate = 0x08,
        LDACVolume = 0x0A,
        RDACVolume = 0x0B,
        BassControl = 0x0C,
        TrebleControl = 0x0D,
        Reset = 0x0F,
        _3DControl = 0x10,
        ALC1 = 0x11,
        ALC2 = 0x12,
        ALC3 = 0x13,
        NoiseGate = 0x14,
        LADCVolume = 0x15,
        RADCVolume = 0x16,
        AdditionalControl1 = 0x17,
        AdditionalControl2 = 0x18,
        PowerManagement1 = 0x19,
        PowerManagement2 = 0x1A,
        AdditionalControl3 = 0x1B,
        ADCInputMode = 0x1F,
        ADCLSignalPath = 0x20,
        ADCRSignalPath = 0x21,
        LOUTMix1 = 0x22,
        LOUTMix2 = 0x23,
        ROUTMix1 = 0x24,
        ROUTMix2 = 0x25,
        MONOOUTMix1 = 0x26,
        MONOOUTMix2 = 0x27,
        LOUT2Volume = 0x28,
        ROUT2Volume = 0x29,
        MONOOUTVolume = 0x2A,
    };
}

// Codec registers
u16 linVolume, rinVolume, lout1Volume, rout1Volume;
u16 dacControl, ldacVolume, rdacVolume;
u16 audioInterface;
u16 sampleRate;
u16 bassControl, trebleControl;
u16 _3DControl;
u16 alc[3];
u16 noiseGate;
u16 ladcVolume, radcVolume;
u16 additionalControl[3];
u16 powerManagement[2];
u16 adcInputMode, adclSignalPath, adcrSignalPath;
u16 loutMix[2], routMix[2], monooutMix[2];
u16 lout2Volume, rout2Volume, monooutVolume;

void reset() {
    std::puts("[WM8750  ] Reset");

    // Default values according to the WM8750 manual
    linVolume = rinVolume = 0x097;
    lout1Volume = rout1Volume = 0x079;
    dacControl = 0x008;
    audioInterface = 0x00A;
    sampleRate = 0x000;
    ldacVolume = rdacVolume = 0x0FF;
    bassControl = trebleControl = 0x00F;
    _3DControl = 0x000;
    alc[0] = 0x07B;
    alc[1] = 0x000;
    alc[2] = 0x032;
    noiseGate = 0x000;
    ladcVolume = radcVolume = 0x0C3;
    additionalControl[0] = 0x0C0;
    additionalControl[1] = 0x000;
    additionalControl[2] = 0x000;
    powerManagement[0] = powerManagement[1] = 0x000;
    adcInputMode = adclSignalPath = adcrSignalPath = 0x000;
    loutMix[0] = loutMix[1] = routMix[0] = routMix[1] = monooutMix[0] = monooutMix[1] = 0x050;
    lout2Volume = rout2Volume = monooutVolume = 0x79;
}

void transmit(u8 *txData) {
    std::puts("[WM8750  ] Transmit");

    // Note: the reg addresses are 7-bit, the reg data 9-bit

    const u16 data = ((u16)(txData[0] & 1) << 8) | (u16)txData[1];

    const auto addr = txData[0] >> 1;

    switch (addr) {
        case CodecReg::LINVolume:
            std::printf("[WM8750  ] Set LIN Volume = 0x%03X\n", data);

            linVolume = data;
            break;
        case CodecReg::RINVolume:
            std::printf("[WM8750  ] Set RIN Volume = 0x%03X\n", data);

            rinVolume = data;
            break;
        case CodecReg::LOUT1Volume:
            std::printf("[WM8750  ] Set LOUT1 Volume = 0x%03X\n", data);

            lout1Volume = data;
            break;
        case CodecReg::ROUT1Volume:
            std::printf("[WM8750  ] Set ROUT1 Volume = 0x%03X\n", data);

            rout1Volume = data;
            break;
        case CodecReg::DACControl:
            std::printf("[WM8750  ] Set DAC Control = 0x%03X\n", data);

            dacControl = data;
            break;
        case CodecReg::AudioInterface:
            std::printf("[WM8750  ] Set Audio Interface = 0x%03X\n", data);

            audioInterface = data;
            break;
        case CodecReg::SampleRate:
            std::printf("[WM8750  ] Set Sample Rate = 0x%03X\n", data);

            sampleRate = data;
            break;
        case CodecReg::LDACVolume:
            std::printf("[WM8750  ] Set LDAC Volume = 0x%03X\n", data);

            ldacVolume = data;
            break;
        case CodecReg::RDACVolume:
            std::printf("[WM8750  ] Set RDAC Volume = 0x%03X\n", data);

            rdacVolume = data;
            break;
        case CodecReg::BassControl:
            std::printf("[WM8750  ] Set Bass Control = 0x%03X\n", data);

            bassControl = data;
            break;
        case CodecReg::TrebleControl:
            std::printf("[WM8750  ] Set Treble Control = 0x%03X\n", data);

            trebleControl = data;
            break;
        case CodecReg::Reset:
            reset();
            break;
        case CodecReg::_3DControl:
            std::printf("[WM8750  ] Set 3D Control = 0x%03X\n", data);

            _3DControl = data;
            break;
        case CodecReg::ALC1:
            std::printf("[WM8750  ] Set ALC1 = 0x%03X\n", data);

            alc[0] = data;
            break;
        case CodecReg::ALC2:
            std::printf("[WM8750  ] Set ALC2 = 0x%03X\n", data);

            alc[1] = data;
            break;
        case CodecReg::ALC3:
            std::printf("[WM8750  ] Set ALC3 = 0x%03X\n", data);

            alc[2] = data;
            break;
        case CodecReg::NoiseGate:
            std::printf("[WM8750  ] Set Noise Gate = 0x%03X\n", data);

            noiseGate = data;
            break;
        case CodecReg::LADCVolume:
            std::printf("[WM8750  ] Set LADC Volume = 0x%03X\n", data);

            ladcVolume = data;
            break;
        case CodecReg::RADCVolume:
            std::printf("[WM8750  ] Set RADC Volume = 0x%03X\n", data);

            radcVolume = data;
            break;
        case CodecReg::AdditionalControl1:
            std::printf("[WM8750  ] Set Additional Control 1 = 0x%03X\n", data);

            additionalControl[0] = data;
            break;
        case CodecReg::AdditionalControl2:
            std::printf("[WM8750  ] Set Additional Control 2 = 0x%03X\n", data);

            additionalControl[1] = data;
            break;
        case CodecReg::PowerManagement1:
            std::printf("[WM8750  ] Set Power Management 1 = 0x%03X\n", data);

            powerManagement[0] = data;
            break;
        case CodecReg::PowerManagement2:
            std::printf("[WM8750  ] Set Power Management 2 = 0x%03X\n", data);

            powerManagement[1] = data;
            break;
        case CodecReg::AdditionalControl3:
            std::printf("[WM8750  ] Set Additional Control 3 = 0x%03X\n", data);

            additionalControl[2] = data;
            break;
        case CodecReg::ADCInputMode:
            std::printf("[WM8750  ] Set ADC Input Mode = 0x%03X\n", data);

            adcInputMode = data;
            break;
        case CodecReg::ADCLSignalPath:
            std::printf("[WM8750  ] Set ADCL Signal Path = 0x%03X\n", data);

            adclSignalPath = data;
            break;
        case CodecReg::ADCRSignalPath:
            std::printf("[WM8750  ] Set ADCR Signal Path = 0x%03X\n", data);

            adcrSignalPath = data;
            break;
        case CodecReg::LOUTMix1:
            std::printf("[WM8750  ] Set LOUT Mix 1 = 0x%03X\n", data);

            loutMix[0] = data;
            break;
        case CodecReg::LOUTMix2:
            std::printf("[WM8750  ] Set LOUT Mix 2 = 0x%03X\n", data);

            loutMix[1] = data;
            break;
        case CodecReg::ROUTMix1:
            std::printf("[WM8750  ] Set ROUT Mix 1 = 0x%03X\n", data);

            routMix[0] = data;
            break;
        case CodecReg::ROUTMix2:
            std::printf("[WM8750  ] Set ROUT Mix 2 = 0x%03X\n", data);

            routMix[1] = data;
            break;
        case CodecReg::MONOOUTMix1:
            std::printf("[WM8750  ] Set MONOOUT Mix 1 = 0x%03X\n", data);

            monooutMix[0] = data;
            break;
        case CodecReg::MONOOUTMix2:
            std::printf("[WM8750  ] Set MONOOUT Mix 2 = 0x%03X\n", data);

            monooutMix[1] = data;
            break;
        case CodecReg::LOUT2Volume:
            std::printf("[WM8750  ] Set LOUT2 Volume = 0x%03X\n", data);

            lout2Volume = data;
            break;
        case CodecReg::ROUT2Volume:
            std::printf("[WM8750  ] Set ROUT2 Volume = 0x%03X\n", data);

            rout2Volume = data;
            break;
        case CodecReg::MONOOUTVolume:
            std::printf("[WM8750  ] Set MONOOUT Volume = 0x%03X\n", data);

            monooutVolume = data;
            break;
        default:
            std::printf("Unhandled WM8750 address 0x%02X\n", addr);

            exit(0);
    }
}

}
