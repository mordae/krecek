#pragma once
#include <stdint.h>

#if !defined(__packed)
#define __packed __attribute__((__packed__))
#endif

#define RESET_ADDR 0x00
struct Reset {
	uint16_t RESET : 9;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct Reset));

// Power Management

#define POWER_MANAGEMENT_1_ADDR 0x01
struct PowerManagement1 {
	uint16_t REFIMP : 2;
	uint16_t IOBUFEN : 1;
	uint16_t ABIASEN : 1;
	uint16_t MICBIASEN : 1;
	uint16_t PLLEN : 1;
	uint16_t AUX2MXEN : 1;
	uint16_t AUX1MXEN : 1;
	uint16_t DCBUFEN : 1;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct PowerManagement1));

#define POWER_MANAGEMENT_2_ADDR 0x02
struct PowerManagement2 {
	uint16_t LADCEN : 1;
	uint16_t RADCEN : 1;
	uint16_t LPGAEN : 1;
	uint16_t RPGAEN : 1;
	uint16_t LBSTEN : 1;
	uint16_t RBSTEN : 1;
	uint16_t SLEEP : 1;
	uint16_t LHPEN : 1;
	uint16_t RHPEN : 1;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct PowerManagement2));

#define POWER_MANAGEMENT_3_ADDR 0x03
struct PowerManagement3 {
	uint16_t LDACEN : 1;
	uint16_t RDACEN : 1;
	uint16_t LMIXEN : 1;
	uint16_t RMIXEN : 1;
	uint16_t reserved1 : 1;
	uint16_t RSPKEN : 1;
	uint16_t LSPKEN : 1;
	uint16_t AUXOUT2EN : 1;
	uint16_t AUXOUT1EN : 1;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct PowerManagement3));

// General Audio Controls

#define AUDIO_INTERFACE_ADDR 0x04
struct AudioInterface {
	uint16_t MONO : 1;
	uint16_t ADCPHS : 1;
	uint16_t DACPHS : 1;
	uint16_t AIFMT : 2;
	uint16_t WLEN : 2;
	uint16_t LRP : 1;
	uint16_t BCLKP : 1;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct AudioInterface));

#define COMPANDING_ADDR 0x05
struct Companding {
	uint16_t ADDAP : 1;
	uint16_t ADCCM : 2;
	uint16_t DACCM : 2;
	uint16_t CMB8 : 1;
	uint16_t : 3;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct Companding));

#define CLOCK_CONTROL_1_ADDR 0x06
struct ClockControl1 {
	uint16_t CLKIOEN : 1;
	uint16_t : 1;
	uint16_t BCLKSEL : 3;
	uint16_t MCLKSEL : 3;
	uint16_t CLKM : 1;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct ClockControl1));

#define CLOCK_CONTROL_2_ADDR 0x07
struct ClockControl2 {
	uint16_t SCLKEN : 1;
	uint16_t SMPLR : 3;
	uint16_t : 4;
	uint16_t _4WSPIEN : 1;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct ClockControl2));

#define GPIO_ADDR 0x08
struct GPIO {
	uint16_t GPIO1SEL : 3;
	uint16_t GPIO1PL : 1;
	uint16_t GPIO1PLL : 2;
	uint16_t : 3;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct GPIO));

#define JACK_DETECT_1_ADDR 0x09
struct JackDetect1 {
	uint16_t : 4;
	uint16_t JCKDIO : 2;
	uint16_t JCKDEN : 1;
	uint16_t JCKMIDEN : 2;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct JackDetect1));

#define DAC_CONTROL_ADDR 0x0a
struct DACControl {
	uint16_t LDACPL : 1;
	uint16_t RDACPL : 1;
	uint16_t AUTOMT : 1;
	uint16_t DACOS : 1;
	uint16_t : 2;
	uint16_t SOFTMT : 1;
	uint16_t : 2;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct DACControl));

#define LEFT_DAC_VOLUME_ADDR 0x0b
struct LeftDACVolume {
	uint16_t LDACGAIN : 8;
	uint16_t LDACVU : 1;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct LeftDACVolume));

#define RIGHT_DAC_VOLUME_ADDR 0x0c
struct RightDACVolume {
	uint16_t RDACGAIN : 8;
	uint16_t RDACVU : 1;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct RightDACVolume));

#define JACK_DETECT_2_ADDR 0x0d
struct JackDetect2 {
	uint16_t JCKDOEN0 : 4;
	uint16_t JCKDOEN1 : 4;
	uint16_t : 1;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct JackDetect2));

#define ADC_CONTROL_ADDR 0x0e
struct ADCControl {
	uint16_t LADCPL : 1;
	uint16_t RADCPL : 1;
	uint16_t : 1;
	uint16_t ADCOS : 1;
	uint16_t HPF : 3;
	uint16_t HPFAM : 1;
	uint16_t HPFEN : 1;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct ADCControl));

#define LEFT_ADC_VOLUME_ADDR 0x0f
struct LeftADCVolume {
	uint16_t LADCGAIN : 8;
	uint16_t LADCVU : 1;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct LeftADCVolume));

#define RIGHT_ADC_VOLUME_ADDR 0x10
struct RightADCVolume {
	uint16_t RADCGAIN : 8;
	uint16_t RADCVU : 1;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct RightADCVolume));

// Equalizer

#define EQ1_LOW_CUTOFF_ADDR 0x12
struct EQ1LowCutoff {
	uint16_t EQ1GC : 5;
	uint16_t EQ1CF : 2;
	uint16_t : 1;
	uint16_t EQM : 1;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct EQ1LowCutoff));

#define EQ2_PEAK_1_ADDR 0x13
struct EQ2Peak1 {
	uint16_t EQ2GC : 5;
	uint16_t EQ2CF : 2;
	uint16_t : 1;
	uint16_t EQ2BW : 1;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct EQ2Peak1));

#define EQ3_PEAK_2_ADDR 0x14
struct EQ3Peak2 {
	uint16_t EQ3GC : 5;
	uint16_t EQ3CF : 2;
	uint16_t : 1;
	uint16_t EQ3BW : 1;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct EQ3Peak2));

#define EQ4_PEAK_3_ADDR 0x15
struct EQ4Peak3 {
	uint16_t EQ4GC : 5;
	uint16_t EQ4CF : 2;
	uint16_t : 1;
	uint16_t EQ4BW : 1;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct EQ4Peak3));

#define EQ5_HIGH_CUTOFF_ADDR 0x16
struct EQ5HighCutoff {
	uint16_t EQ5GC : 5;
	uint16_t EQ5CF : 2;
	uint16_t : 2;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct EQ5HighCutoff));

// DAC Limiter

#define DAC_LIMITER_1_ADDR 0x18
struct DACLimiter1 {
	uint16_t DACLIMATK : 4;
	uint16_t DACLIMDCY : 4;
	uint16_t DACLIMEN : 1;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct DACLimiter1));

#define DAC_LIMITER_2_ADDR 0x19
struct DACLimiter2 {
	uint16_t DACLIMBST : 4;
	uint16_t DACLIMTHL : 3;
	uint16_t : 2;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct DACLimiter2));

// Notch Filter

#define NOTCH_FILTER_1_ADDR 0x1b
struct NotchFilter1 {
	uint16_t NFCA0 : 7;
	uint16_t NFCEN : 1;
	uint16_t NFCU1 : 1;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct NotchFilter1));

#define NOTCH_FILTER_2_ADDR 0x1c
struct NotchFilter2 {
	uint16_t NFCA0 : 7;
	uint16_t : 1;
	uint16_t NFCU2 : 1;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct NotchFilter2));

#define NOTCH_FILTER_3_ADDR 0x1d
struct NotchFilter3 {
	uint16_t NFCA1 : 7;
	uint16_t : 1;
	uint16_t NFCU3 : 1;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct NotchFilter3));

#define NOTCH_FILTER_4_ADDR 0x1e
struct NotchFilter4 {
	uint16_t NFCA1 : 7;
	uint16_t : 1;
	uint16_t NFCU4 : 1;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct NotchFilter4));

// ALC and Noise Gate Control

#define ALC_CONTROL_1_ADDR 0x20
struct ALCControl1 {
	uint16_t ALCMNGAIN : 3;
	uint16_t ALCMXGAIN : 3;
	uint16_t : 1;
	uint16_t ALCEN : 2;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct ALCControl1));

#define ALC_CONTROL_2_ADDR 0x21
struct ALCControl2 {
	uint16_t ALCSL : 4;
	uint16_t ALCHT : 4;
	uint16_t : 1;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct ALCControl2));

#define ALC_CONTROL_3_ADDR 0x22
struct ALCControl3 {
	uint16_t ALCATK : 4;
	uint16_t ALCDCY : 4;
	uint16_t ALCM : 1;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct ALCControl3));

#define NOISE_GATE_ADDR 0x23
struct NoiseGate {
	uint16_t ALCNTH : 3;
	uint16_t ALCNEN : 1;
	uint16_t : 5;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct NoiseGate));

// Phase Locked Loop

#define PLL_N_ADDR 0x24
struct PLLN {
	uint16_t PLLN : 4;
	uint16_t PLLMCLK : 1;
	uint16_t : 4;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct PLLN));

#define PLL_K1_ADDR 0x25
struct PLLK1 {
	uint16_t PLLK : 6;
	uint16_t : 3;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct PLLK1));

#define PLL_K2_ADDR 0x26
struct PLLK2 {
	uint16_t PLLK : 9;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct PLLK2));

#define PLL_K3_ADDR 0x27
struct PLLK3 {
	uint16_t PLLK : 9;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct PLLK3));

// Miscellaneous

#define _3D_CONTROL_ADDR 0x29
struct _3DControl {
	uint16_t _3DDEPTH : 4;
	uint16_t : 5;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct _3DControl));

#define RIGHT_SPEAKER_SUBMIX_ADDR 0x2b
struct RightSpeakerSubmix {
	uint16_t RAUXSMUT : 1;
	uint16_t RAUXRSUBG : 3;
	uint16_t RSUBBYP : 1;
	uint16_t RMIXMUT : 1;
	uint16_t : 3;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct RightSpeakerSubmix));

#define INPUT_CONTROL_ADDR 0x2c
struct InputControl {
	uint16_t LMICPLPGA : 1;
	uint16_t LMICNLPGA : 1;
	uint16_t LLINLPGA : 1;
	uint16_t : 1;
	uint16_t RMICPRPGA : 1;
	uint16_t RMICNRPGA : 1;
	uint16_t RLINRPGA : 1;
	uint16_t MICBIASV : 2;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct InputControl));

#define LEFT_INPUT_GPA_GAIN_ADDR 0x2d
struct LeftInputPGAGain {
	uint16_t LPGAGAIN : 6;
	uint16_t LPGAMT : 1;
	uint16_t LPGAZC : 1;
	uint16_t LPGAU : 1;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct LeftInputPGAGain));

#define RIGHT_INPUT_PGA_GAIN_ADDR 0x2e
struct RightInputPGAGain {
	uint16_t RPGAGAIN : 6;
	uint16_t RPGAMT : 1;
	uint16_t RPGAZC : 1;
	uint16_t RPGAU : 1;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct RightInputPGAGain));

#define LEFT_ADC_BOOST_ADDR 0x2f
struct LeftADCBoost {
	uint16_t LAUXBSTGAIN : 3;
	uint16_t : 2;
	uint16_t LPGABSTGAIN : 3;
	uint16_t LPGABST : 1;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct LeftADCBoost));

#define RIGHT_ADC_BOOST_ADDR 0x30
struct RightADCBoost {
	uint16_t RAUXBSTGAIN : 3;
	uint16_t : 2;
	uint16_t RPGABSTGAIN : 3;
	uint16_t RPGABST : 1;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct RightADCBoost));

#define OUTPUT_CONTROL_ADDR 0x31
struct OutputControl {
	uint16_t AOUTIMP : 1;
	uint16_t TSEN : 1;
	uint16_t SPKBST : 1;
	uint16_t AUX2BST : 1;
	uint16_t AUX1BST : 1;
	uint16_t RDACLMX : 1;
	uint16_t LDACRMX : 1;
	uint16_t : 2;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct OutputControl));

#define LEFT_MIXER_ADDR 0x32
struct LeftMixer {
	uint16_t LDACLMX : 1;
	uint16_t LBYPLMX : 1;
	uint16_t LBYPMXGAIN : 3;
	uint16_t LAUXLMX : 1;
	uint16_t LAUXMXGAIN : 3;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct LeftMixer));

#define RIGHT_MIXER_ADDR 0x33
struct RightMixer {
	uint16_t RDACRMX : 1;
	uint16_t RBYPRMX : 1;
	uint16_t RBYPMXGAIN : 3;
	uint16_t RAUXRMX : 1;
	uint16_t RAUXMXGAIN : 3;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct RightMixer));

#define LHP_VOLUME_ADDR 0x34
struct LHPVolume {
	uint16_t LHPGAIN : 6;
	uint16_t LHPMUTE : 1;
	uint16_t LHPZC : 1;
	uint16_t LHPVU : 1;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct LHPVolume));

#define RHP_VOLUME_ADDR 0x35
struct RHPVolume {
	uint16_t RHPGAIN : 6;
	uint16_t RHPMUTE : 1;
	uint16_t RHPZC : 1;
	uint16_t RHPVU : 1;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct RHPVolume));

#define LSPKOUT_VOLUME_ADDR 0x36
struct LSPKOutVolume {
	uint16_t LSPKGAIN : 6;
	uint16_t LSPKMUTE : 1;
	uint16_t LSPKZC : 1;
	uint16_t LSPKVU : 1;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct LSPKOutVolume));

#define RSPKOUT_VOLUME_ADDR 0x37
struct RSPKOutVolume {
	uint16_t RSPKGAIN : 6;
	uint16_t RSPKMUTE : 1;
	uint16_t RSPKZC : 1;
	uint16_t RSPKVU : 1;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct RSPKOutVolume));

#define AUX2_MIXER_ADDR 0x38
struct AUX2Mixer {
	uint16_t LDACAUX2 : 1;
	uint16_t LMIXAUX2 : 1;
	uint16_t LADCAUX2 : 1;
	uint16_t AUX1MIX2 : 1;
	uint16_t : 2;
	uint16_t AUXOUT2MT : 1;
	uint16_t : 2;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct AUX2Mixer));

#define AUX1_MIXER_ADDR 0x39
struct AUX1Mixer {
	uint16_t RDACAUX1 : 1;
	uint16_t RMIXAUX1 : 1;
	uint16_t RADCAUX1 : 1;
	uint16_t LDACAUX1 : 1;
	uint16_t LMIXAUX1 : 1;
	uint16_t AUX1HALF : 1;
	uint16_t AUXOUT1MT : 1;
	uint16_t : 2;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct AUX1Mixer));

// NAU88C22 Proprietary Register Space

#define POWER_MANAGEMENT_4_ADDR 0x3a
struct PowerManagement4 {
	uint16_t IBADJ : 2;
	uint16_t REGVOLT : 2;
	uint16_t MICBIASM : 1;
	uint16_t LPSPKD : 1;
	uint16_t LPADC : 1;
	uint16_t LPIPBST : 1;
	uint16_t LPDAC : 1;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct PowerManagement4));

// PCM Time Slot and ADCOUT Impedance Option Control

#define LEFT_TIME_SLOT_ADDR 0x3b
struct LeftTimeSlot {
	uint16_t LTSLOT : 9;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct LeftTimeSlot));

#define MISC_ADDR 0x3c
struct Misc {
	uint16_t LTSLOT : 1;
	uint16_t RTSLOT : 1;
	uint16_t : 1;
	uint16_t PUDPS : 1;
	uint16_t PUDPE : 1;
	uint16_t PUDEN : 1;
	uint16_t PCM8BIT : 1;
	uint16_t TRI : 1;
	uint16_t PCMTSEN : 1;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct Misc));

#define RIGHT_TIME_SLOT_ADDR 0x3d
struct RightTimeSlot {
	uint16_t RTSLOT : 9;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct RightTimeSlot));

// Silicon Revision and Device ID

#define DEVICE_REVISION_ADDR 0x3e
struct DeviceRevision {
	uint16_t REV : 8;
	uint16_t : 1;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct DeviceRevision));

#define DEVICE_ID_ADDR 0x3f
struct DeviceId {
	uint16_t ID : 9;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct DeviceId));

#define _5V_BIASING_ADDR 0x45
struct _5VBiasing {
	uint16_t HVOP : 1;
	uint16_t : 1;
	uint16_t HVOPU : 1;
	uint16_t : 6;
	uint16_t addr : 7;
};

#define ALC_ENHANCEMENTS_1_ADDR 0x46
struct ALCEnhancements1 {
	uint16_t ALCGAINL : 6;
	uint16_t ALCNGSEL : 1;
	uint16_t ALCPKSEL : 1;
	uint16_t ALCTBLSEL : 1;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct ALCEnhancements1));

#define ALC_ENHANCEMENTS_2_ADDR 0x47
struct ALCEnhancements2 {
	uint16_t ALCGAINR : 6;
	uint16_t : 2;
	uint16_t PKLIMENA : 1;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct ALCEnhancements2));

#define _192KHZ_SAMPLING_ADDR 0x48
struct _192kHzSampling {
	uint16_t ADC_OSR32x : 1;
	uint16_t DAC_OSR32x : 1;
	uint16_t PLL49MOUT : 1;
	uint16_t : 2;
	uint16_t ADCB_OVER : 1;
	uint16_t : 3;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct _192kHzSampling));

#define MISC_CONTROLS_ADDR 0x49
struct MiscControls {
	uint16_t DACOS256 : 1;
	uint16_t PLLLOKBP : 1;
	uint16_t : 2;
	uint16_t FSERRENA : 1;
	uint16_t FSERFLSH : 1;
	uint16_t FSERRVAL : 2;
	uint16_t _4WSPIENA : 1;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct MiscControls));

#define TIE_OFF_OVERRIDES_ADDR 0x4a
struct TieOffOverrides {
	uint16_t MANLMICP : 1;
	uint16_t MANLMICN : 1;
	uint16_t MANLLIN : 1;
	uint16_t MANLAUX : 1;
	uint16_t MANRMICP : 1;
	uint16_t MANRMICN : 1;
	uint16_t MANRLIN : 1;
	uint16_t MANRAUX : 1;
	uint16_t MANINENA : 1;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct TieOffOverrides));

#define POWER_TIE_OFF_CTRL_ADDR 0x51
struct PowerTieOffCtrl {
	uint16_t MANVREFL : 1;
	uint16_t MANVREFM : 1;
	uint16_t MANVREFH : 1;
	uint16_t MANINPAD : 1;
	uint16_t MANINBBP : 1;
	uint16_t IBT250DN : 1;
	uint16_t IBT500UP : 1;
	uint16_t : 1;
	uint16_t IBTHALFI : 1;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct PowerTieOffCtrl));

#define P2P_DETECTOR_READ_ADDR 0x4c
struct P2PDetectorRead {
	uint16_t P2PVAL : 9;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct P2PDetectorRead));

#define PEAK_DETECTOR_READ_ADDR 0x4d
struct PeakDetectorRead {
	uint16_t PEAKVAL : 9;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct PeakDetectorRead));

#define CONTROL_AND_STATUS_ADDR 0xe4
struct ControlAndStatus {
	uint16_t DIGMUTER : 1;
	uint16_t DIGMUTEL : 1;
	uint16_t ANAMUTE : 1;
	uint16_t NSGATE : 1;
	uint16_t HVDET : 1;
	uint16_t AMUTCTRL : 1;
	uint16_t : 3;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct ControlAndStatus));

#define OUTPUT_TIE_OFF_CONTROL_ADDR 0xef
struct OutputTieOffControl {
	uint16_t SHRTRHP : 1;
	uint16_t SHRTLHP : 1;
	uint16_t SHRTAUX2 : 1;
	uint16_t SHRTAUX1 : 1;
	uint16_t SHRTRSPK : 1;
	uint16_t SHRTLSPK : 1;
	uint16_t SHRTBUFL : 1;
	uint16_t SHRTBUFH : 1;
	uint16_t MANOUTEN : 1;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct OutputTieOffControl));

#define SPI1_ADDR 0x57
struct SPI1 {
	uint16_t SPI1 : 9;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct SPI1));

#define SPI2_ADDR 0x6c
struct SPI2 {
	uint16_t SPI2 : 9;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct SPI2));

#define SPI3_ADDR 0x73
struct SPI3 {
	uint16_t SPI3 : 9;
	uint16_t addr : 7;
};
static_assert(2 == sizeof(struct SPI3));
