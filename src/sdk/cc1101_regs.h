#pragma once
#include <stdint.h>

enum {
	FLAG_BURST = 0x40,
	FLAG_READ = 0x80,
};

enum {
	// Regular, read/write configuration registers
	REG_IOCFG2 = 0x00,
	REG_IOCFG1 = 0x01,
	REG_IOCFG0 = 0x02,
	REG_FIFOTHR = 0x03,
	REG_SYNC1 = 0x04,
	REG_SYNC2 = 0x05,
	REG_PKTLEN = 0x06,
	REG_PKTCTRL1 = 0x07,
	REG_PKTCTRL0 = 0x08,
	REG_ADDR = 0x09,
	REG_CHANNR = 0x0a,
	REG_FSCTRL1 = 0x0b,
	REG_FSCTRL0 = 0x0c,
	REG_FREQ2 = 0x0d,
	REG_FREQ1 = 0x0e,
	REG_FREQ0 = 0x0f,
	REG_MDMCFG4 = 0x10,
	REG_MDMCFG3 = 0x11,
	REG_MDMCFG2 = 0x12,
	REG_MDMCFG1 = 0x13,
	REG_MDMCFG0 = 0x14,
	REG_DEVIATN = 0x15,
	REG_MCSM2 = 0x16,
	REG_MCSM1 = 0x17,
	REG_MCSM0 = 0x18,
	REG_FOCCFG = 0x19,
	REG_BSCFG = 0x1a,
	REG_AGCCTRL2 = 0x1b,
	REG_AGCCTRL1 = 0x1c,
	REG_AGCCTRL0 = 0x1d,
	REG_WOREVT1 = 0x1e,
	REG_WOREVT0 = 0x1f,
	REG_WORCTRL = 0x20,
	REG_FREND1 = 0x21,
	REG_FREND0 = 0x22,
	REG_FSCAL3 = 0x23,
	REG_FSCAL2 = 0x24,
	REG_FSCAL1 = 0x25,
	REG_FSCAL0 = 0x26,
	REG_RCCTRL1 = 0x27,
	REG_RCCTRL0 = 0x28,
	REG_FSTEST = 0x29,
	REG_PTEST = 0x2a,
	REG_AGCTEST = 0x2b,
	REG_TEST2 = 0x2c,
	REG_TEST1 = 0x2d,
	REG_TEST0 = 0x2e,
	REG_UNUSED_2F = 0x2f,

	// Read-only registers.
	REG_PARTNUM = 0xf0,
	REG_VERSION = 0xf1,
	REG_FREQEST = 0xf2,
	REG_LQI = 0xf3,
	REG_RSSI = 0xf4,
	REG_MARCSTATE = 0xf5,
	REG_WORTIME1 = 0xf6,
	REG_WORTIME0 = 0xf7,
	REG_PKTSTATUS = 0xf8,
	REG_VCO_VC_DAC = 0xf9,
	REG_TXBYTES = 0xfa,
	REG_RXBYTES = 0xfb,
	REG_RCCTRL1_STATUS = 0xfc,
	REG_RCCTRL0_STATUS = 0xfd,

	// Special register with power amplifier ramp values.
	// Actually 8 data bytes, best use burst mode.
	// Pointer resets to data0 upon CS going high.
	REG_PATABLE = 0x3e,

	// RX and TX FIFO access.
	// They are split, you cannot read TX nor write RX.
	REG_FIFO = 0x3f,
};

enum {
	SRES = 0x30,
	SFSTXON = 0x31,
	SXOFF = 0x32,
	SCAL = 0x33,
	SRX = 0x34,
	STX = 0x35,
	SIDLE = 0x36,
	SWOR = 0x38,
	SPWD = 0x39,
	SFRX = 0x3a,
	SFTX = 0x3b,
	SWORRST = 0x3c,
	SNOP = 0x3d,
};

enum {
	STATE_SLEEP = 0x00,
	STATE_IDLE = 0x01,
	STATE_XOFF = 0x02,
	STATE_VCCON_MC = 0x03,
	STATE_REGON_MC = 0x04,
	STATE_MANCAL = 0x05,
	STATE_VCOON = 0x06,
	STATE_REGON = 0x07,
	STATE_STARTCAL = 0x08,
	STATE_BWBOOST = 0x09,
	STATE_FS_LOCK = 0x0a,
	STATE_IFADCON = 0x0b,
	STATE_ENDCAL = 0x0c,
	STATE_RX = 0x0d,
	STATE_RX_END = 0x0e,
	STATE_RX_RST = 0x0f,
	STATE_TXRX_SWITCH = 0x10,
	STATE_RXFIFO_OVERFLOW = 0x11,
	STATE_FSTXON = 0x12,
	STATE_TX = 0x13,
	STATE_TX_END = 0x14,
	STATE_RXTX_SWITCH = 0x15,
	STATE_TXFIFO_UNDERFLOW = 0x16,
};

// GDO2 Output Pin Configuration
struct IOCFG2 {
	uint8_t GDO2_CFG : 6; // Function (0x29 default)
	uint8_t GDO2_INV : 1; // Invert GDO2
	uint8_t : 1;
};

// GDO1 Output Pin Configuration
struct IOCFG1 {
	uint8_t GDO1_CFG : 6; // Function (0x2e default)
	uint8_t GDO1_INV : 1; // Invert GDO1
	uint8_t GDO_DS : 1;   // Drive strength
};

// GDO0 Output Pin Configuration
struct IOCFG0 {
	uint8_t GDO0_CFG : 6; // Function (0x3f default)
	uint8_t GDO0_INV : 1; // Invert GDO0
	// Enable analog temperature sensor.
	// Write 0 in all other register bits when using temperature sensor.
	uint8_t TEMP_SENSOR_ENABLE : 1;
};

// RX FIFO and TX FIFO Thresholds
struct FIFOTHR {
	// (4 * FIFOTHR) for RX FIFO and (61 - 4 * FIFO_THR) for TX FIFO.
	// The threshold is exceeded when the number of bytes in the FIFO
	// is equal to or higher than the threshold value.
	uint8_t FIFO_THR : 4;

	// Extra RX attenuation of (CLOSE_IN_RX * 6) dB for operation in
	// close proximity of the transmitter. Prevents saturation.
	uint8_t CLOSE_IN_RX : 2;

	// Pertains to TEST1 and TEST2 registers during SLEEP.
	uint8_t ADC_RETENTION : 1;
	uint8_t : 1;
};

// Sync Word, High Byte
struct SYNC1 {
	uint8_t SYNC; // Sync Word MSB, 0xd3 default
};

// Sync Word, Low Byte
struct SYNC2 {
	uint8_t SYNC2; // Sync Word LSB, 0x91 default
};

// Packet Length
struct PKTLEN {
	// Packet length in fixed-length mode or maximum length in
	// variable-length packet mode. Defaults to 0xff.
	uint8_t PACKET_LENGTH;
};

// Packet Automation Control
struct PKTCTRL1 {
	// 00 = No address check
	// 01 = Address check, no broadcast
	// 10 = Address check and 0x00 is broadcast
	// 11 = Address check and 0x00, 0xff are broadcast
	uint8_t ADR_CHK : 2;

	// Append 2 status bytes with RSSI, LQI and CRC OK data.
	uint8_t APPEND_STATUS : 1;

	// Flush of RX FIFO when CRC is not OK.
	uint8_t CRC_AUTOFLUSH : 1;

	uint8_t : 1;

	// Preamble quality estimator threshold.
	uint8_t PQT : 3;
};

// Packet Automation Control
struct PKTCTRL0 {
	// 00 = Fixed packet length mode
	// 01 = Variable packet length mode (first byte is length)
	// 10 = Infinite packet length mode
	uint8_t LENGTH_CONFIG : 2;

	// Enable TX/RX CRC calculation/validation
	uint8_t CRC_EN : 1;

	// 00 = Normal mode, use FIFOs for RX and TX
	// 01 = Synchronous serial mode, data over GDOx pins
	// 10 = Random TX mode; sends random data
	// 11 = Asynchronous serial mode, data over GDOx pins
	uint8_t PKT_FORMAT : 2;

	// Enable data whitening
	uint8_t WHITE_DATA : 1;

	uint8_t : 1;
};

// Device Address
struct ADDR {
	// Address used for packet filtration
	uint8_t DEVICE_ADDR;
};

// Channel Number
struct CHANNR {
	// Channel number which is multiplied by the channel
	// spacing setting and added to the base frequency.
	uint8_t CHAN;
};

// Frequency Synthesizer Control
struct FSCTRL1 {
	// Controls RX IF. Defaults to 0x0f.
	// f_IF = (f_XOSC * FREQ_IF) >> 10
	uint8_t FREQ_IF : 5;

	uint8_t : 3;
};

// Frequency Synthesizer Control
struct FSCTRL0 {
	// Frequency offset added to the base frequency before
	// being used by the frequency synthesizer.
	// offset = (f_XOSC * FREQOFF) >> 14
	int8_t FREQOFF;
};

// Frequency Control Word, High Byte
struct FREQ2 {
	// f_CARRIER = (f_XOSC * FREQ) >> 16
	// Default FREQ = 0x1ec4ec (2016492), 800+ MHz
	uint8_t FREQ;
};

// Frequency Control Word, Middle Byte
struct FREQ1 {
	uint8_t FREQ;
};

// Frequency Control Word, Low Byte
struct FREQ0 {
	uint8_t FREQ;
};

// Modem Configuration
struct MDMCFG4 {
	// The exponent of the user specified symbol rate
	uint8_t DRATE_E : 4;

	// ADC decimation ratio => channel bandwidth.
	// BW_channel = f_XOSC / ((4 + CHANBW_M) << (3 + CHANBW_E))
	uint8_t CHANBW_M : 2;

	// See CHANBW_M.
	uint8_t CHANBW_E : 2;
};

// Modem Configuration
struct MDMCFG3 {
	// The mantissa of the user specified symbol rate.
	// R_data = (f_XOSC * (256 + DRATE_M)) >> (28 - DRATE_E)
	uint8_t DRATE_M;
};

// Modem Configuration
struct MDMCFG2 {
	// 000 = No preamble/sync
	// 001 = 15/16 sync word bits detected
	// 010 = 16/16 sync word bits detected
	// 011 = 30/32 sync word bits detected
	uint8_t SYNC_MODE : 2;

	// Require carrier sense above threshold
	uint8_t CARRIER_SENSE : 1;

	// Enable machester coding
	uint8_t MANCHESTER_EN : 1;

	// 000 = 2-FSK
	// 001 = GFSK
	// 011 = ASK/OOK
	// 100 = 4-FSK
	// 111 = MSK (26+ kBaud only)
	uint8_t MOD_FORMAT : 3;

	// Disable digital DC blocking filter.
	uint8_t DEM_DCFILT_OFF : 1;
};

// Modem Configuration
struct MDMCFG1 {
	// 2 bit exponent of channel spacing
	uint8_t CHANSPC_E : 2;

	uint8_t : 2;

	// Minimum number of preamble bytes to be transmitted
	// 000 = 2
	// 001 = 3
	// 010 = 4
	// 011 = 6
	// 100 = 8
	// 101 = 12
	// 110 = 16
	// 111 = 24
	uint8_t NUM_PREAMBLE : 3;

	// Forward Error Correction with interleaving.
	// Only supported for fixed packet length mode.
	uint8_t FEC_EN : 1;
};

// Modem Configuration
struct MDMCFG0 {
	// 8-bit mantissa of channel spacing.
	// f_CHANNEL = (f_XOSC * (256 + CHANSPC_M)) >> (18 - CHANSPC_E)
	uint8_t CHANSPC_M;
};

// Modem Deviation Setting
//
// f_DEV = (f_XOSC * (8 + DEVIATION_M)) >> (17 - DEVIATION_E)
struct DEVIATN {
	uint8_t DEVIATION_M : 3;
	uint8_t : 1;
	uint8_t DEVIATION_E : 3;
	uint8_t : 1;
};

// Main Radio Control State Machine Configuration
struct MCSM2 {
	// Timeout for sync word search in RX for both WOR mode and normal RX
	// operation. The timeout is relative to the programmed EVENT0 timeout.
	//
	// 111 = Until end of packet
	uint8_t RX_TIME : 3;

	// When the RX_TIME timer expires, the chip checks if sync word is found
	// when RX_TIME_QUAL=0, or either sync word is found or PQI is set when
	// RX_TIME_QUAL=1.
	uint8_t RX_TIME_QUAL : 1;

	// Direct RX termination based on RSSI measurement (carrier sense).
	// For ASK/OOK modulation, RX times out if there is no carrier sense in
	// the first 8 symbol periods.
	uint8_t RX_TIME_RSSI : 1;

	uint8_t : 3;
};

// Main Radio Control State Machine Configuration
struct MCSC1 {
	// Select where to go next after packet has been sent in TX
	// 00 = IDLE        (default)
	// 01 = FSTXON
	// 10 = Stay in TX  (starts sending preamble)
	// 11 = RX
	uint8_t TXOFF_MODE : 2;

	// Select where to go after packet has been received in RX
	// 00 = IDLE        (default)
	// 01 = FSTXON      (not with CCA)
	// 10 = TX          (not with CCA)
	// 11 = Stay in RX
	uint8_t RXOFF_MODE : 2;

	// Clear Channel Assessment Mode
	// 00 = Always
	// 01 = If RSSI below threshold
	// 10 = Unless receiving
	// 11 = If RSSI below threshold, unless receiving (default)
	uint8_t CCA_MODE : 2;

	uint8_t : 2;
};

struct MCSC0 {
	// Force XOSC to stay on in SLEEP
	uint8_t XOSC_FORCE_ON : 1;

	// Enables the pin radio control option
	uint8_t PIN_CTRL_EN : 1;

	// Power On XOSC timeout, defaults to 1 (~38us)
	// Probably can set this to 0 when not using crystal.
	uint8_t PO_TIMEOUT : 2;

	// Automatically calibrate when going to RX or TX, or back to IDLE
	// 00 = Never (default, need to call SCAL manually)
	// 01 = When going from IDLE to RX or TX (or FSTXON)
	// 10 = When going from RX or TX back to IDLE automatically
	// 11 = Every 4th time when going from RX or TX to IDLE automatically
	uint8_t FS_AUTOCAL : 2;

	uint8_t : 2;
};

// Frequency Offset Compensation Configuration
struct FOCCFG {
	// The saturation point for the frequency offset compensation algorithm.
	// Do not enable for ASK/OOK.
	//
	// 00 = no compensation
	// 01 = +/- BW_channel / 8
	// 10 = +/- BW_channel / 4 (default)
	// 11 = +/- BW_channel / 2
	uint8_t FOC_LIMIT : 2;

	// Loop gain after sync word is detected.
	// 0 = FOC_PRE_K
	// 1 = K/2 (default)
	uint8_t FOC_POST_K : 1;

	// Loop gain before sync word is detected.
	// 00 = 1 * K
	// 01 = 2 * K
	// 10 = 3 * K (default)
	// 11 = 4 * K
	uint8_t FOC_PRE_K : 2;

	// Freeze FOC while CS is low.
	// Default enabled.
	uint8_t FOC_BS_CS_GATE : 1;

	uint8_t : 2;
};

// Bit Synchronization Configuration
struct BSCFG {
	// The saturation point for the data rate offset compensation algorithm:
	// 00 = no compensation
	// 01 = R / 32
	// 10 = R / 16
	// 11 = R / 8
	uint8_t BS_LIMIT : 2;

	// Clock recovery feedback loop gain to be used after sync word.
	// 0 = BS_PRE_KP
	// 1 = K_P (default)
	uint8_t BS_POST_KP : 1;

	// Clock recovery feedback loop integral gain after sync word.
	// 0 = BS_PRE_KI
	// 1 = K_I / 2 (default)
	uint8_t BS_POST_KI : 1;

	// Clock recovery feedback loop integral gain before sync word.
	// 00 = 1 * K_P
	// 01 = 2 * K_P
	// 10 = 3 * K_P (default)
	// 11 = 4 * K_P
	uint8_t BS_PRE_KP : 2;

	// Clock recovery feedback loop integral gain before sync word,
	// used to correct offsets in data rate.
	// 00 = 1 * K_I
	// 01 = 2 * K_I (default)
	// 10 = 3 * K_I
	// 11 = 4 * K_I
	uint8_t BS_PRE_KI : 2;
};

// AGC Control
struct AGCCTRL2 {
	// Target average amplitude on the digital channel.
	// 000 = 24 dB
	// 001 = 27 dB
	// 010 = 30 dB
	// 011 = 33 dB (default)
	// 100 = 36 dB
	// 101 = 38 dB
	// 110 = 40 dB
	// 111 = 42 dB
	uint8_t MAGN_TARGET : 3;

	// Maximum LNA + LNA2 gain relative to maximum possible gain.
	// 000 =  -0.0 dB
	// 001 =  -2.6 dB
	// 010 =  -6.1 dB
	// 011 =  -7.4 dB
	// 100 =  -9.2 dB
	// 101 = -11.5 dB
	// 110 = -14.6 dB
	// 111 = -17.1 dB
	uint8_t MAX_LNA_GAIN : 3;

	// Reduces the maximum allowable DVGA gain.
	// 00 = no maximum
	// 01 = 1 highest disabled
	// 10 = 2 highest disabled
	// 11 = 3 highest disabled
	uint8_t MAX_DVGA_GAIN : 2;
};

// AGC Control
struct AGCCTRL1 {
	// Absolute RSSI threshold for asserting carrier sense.
	// In 1 dB steps, relative to MAGN_TARGET.
	int8_t CARRIER_SENSE_ABS_THR : 4;

	// Relative threshold for asserting carrier sense.
	// 00 = disabled
	// 01 =  +6 dB
	// 10 = +10 dB
	// 11 = +14 dB
	uint8_t CARRIER_SENSE_REL_THR : 2;

	// 0 = First attenuate LNA, then LNA2
	// 1 = First attenuate LNA2, then LNA (default)
	uint8_t AGC_LNA_PRIORITY : 1;

	uint8_t : 1;
};

// AGC Control
struct AGCCTRL0 {
	// In FSK mode: averaging length for the amplitude from the channel filter.
	// 00 = 8 samples
	// 01 = 16 samples (default)
	// 10 = 32 samples
	// 11 = 64 samples
	//
	// In ASK/OOK mode: decision boundary for reception.
	// 00 =  4 dB
	// 01 =  8 dB (default)
	// 10 = 12 dB
	// 11 = 16 dB
	uint8_t FILTER_LENGTH : 2;

	// 00 = normal operation
	// 01 = freeze gain after sync word
	// 10 = manual analog freeze, use digital gain
	// 11 = manual freeze of both gains
	uint8_t AGC_FREEZE : 2;

	// How many samples to wait after gain adjustment before starting
	// to accumulate new samples and potentially changing gain again.
	// 00 =  8 samples
	// 01 = 16 samples (default)
	// 10 = 24 samples
	// 11 = 32 samples
	uint8_t WAIT_TIME : 2;

	// Hysteresis on magnitude deviation.
	// 00 = No hysteresis, small symmetric dead zone, high gain
	// 01 = Low hysteresis, small asymmetric dead zone, medium gain
	// 10 = Medium hysteresis, medium asymmetric dead zone, medium gain (default)
	// 11 = Large hysteresis, large asymmetric dead zone, low gain
	uint8_t HYST_LEVEL : 2;
};

// High Byte Event0 Timeout
struct WOREVT1 {
	// t_Event0 = (750 / f_XOSC * EVENT0) << (5 * WOR_RES)
	uint8_t EVENT0;
};

// Low Byte Event0 Timeout
struct WOREVT0 {
	uint8_t EVENT0;
};

// Wake On Radio Control
struct WORCTRL {
	// WOR Resolution. See WOREVT1.EVENT0.
	uint8_t WOR_RES : 2;

	uint8_t : 1;

	// Calibrate RC oscillator.
	uint8_t RC_CAL : 1;

	// RC clock periods after Event0 for Event1 to time out.
	uint8_t WOR_EVENT1 : 3;

	// Power down RC oscillator.
	uint8_t RC_PD : 1;
};

// Front End RX Configuration
struct FREND1 {
	uint8_t MIX_CURRENT : 2;
	uint8_t LODIV_BUF_CURRENT_RX : 2;
	uint8_t LNA2MIX_CURRENT : 2;
	uint8_t LNA_CURRENT : 2;
};

// Front End RX Configuration
struct FREND0 {
	// Index to the PATABLE.
	//
	// In OOK/ASK mode, strength to use when transmitting '1'.
	// When transmitting '0', index 0 is always used.
	//
	// The indexes between 0 and this index are used to shape
	// the PA ramp in both FSK and ASK/OOK modes.
	uint8_t PA_POWER : 3;

	uint8_t : 1;

	// Adjust current to TX LO buffer (input to PA).
	// Default is 1.
	uint8_t LODIV_BUF_CURRENT_TX : 2;

	uint8_t : 2;
};

// Frequency Synthesizer Calibration
struct FSCAL3 {
	uint8_t FSCAL3 : 4;
	uint8_t CHIP_CURR_CAL_EN : 2;
	uint8_t FSCAL3_CFG : 2;
};

// Frequency Synthesizer Calibration
struct FSCAL2 {
	uint8_t FSCAL2 : 5;
	uint8_t VCO_CORE_H_EN : 1;
	uint8_t : 2;
};

// Frequency Synthesizer Calibration
struct FSCAL1 {
	uint8_t FSCAL1 : 5;
	uint8_t : 3;
};

// Frequency Synthesizer Calibration
struct FSCAL0 {
	uint8_t FSCAL0 : 7;
	uint8_t : 1;
};

// RC Oscillator Configuration
struct RCCTRL1 {
	uint8_t RCCTRL : 7;
	uint8_t : 1;
};

// RC Oscillator Configuration
struct RCCTRL0 {
	uint8_t RCCTRL : 7;
	uint8_t : 1;
};

struct FSTEST {
	uint8_t FSTEST;
};

struct PTEST {
	uint8_t PTEST;
};

struct AGCTEST {
	uint8_t AGCTEST;
};

struct TEST2 {
	uint8_t TEST2;
};

struct TEST1 {
	uint8_t TEST1;
};

struct TEST0 {
	uint8_t TEST0;
};

struct PARTNUM {
	uint8_t PARTNUM;
};

struct VERSION {
	uint8_t VERSION;
};

struct FREQEST {
	uint8_t FREQOFF_EST;
};

struct LQI {
	uint8_t LQI_EST : 7;
	uint8_t CRC_OK : 1;
};

struct RSSI {
	int8_t RSSI;
};

// Main Radio Control State Machine State
struct MARCSTATE {
	uint8_t MARC_STATE : 5;
	uint8_t : 3;
};

struct WORTIME1 {
	uint8_t TIME;
};

struct WORTIME0 {
	uint8_t TIME;
};

// Current GDOx Status and Packet Status
struct PKTSTATUS {
	uint8_t GDO0 : 1;
	uint8_t : 1;
	uint8_t GDO2 : 1;
	uint8_t SFD : 1;
	uint8_t CCA : 1;
	uint8_t PQT_REACHED : 1;
	uint8_t CS : 1;
	uint8_t CRC_OK : 1;
};

struct VCO_VC_DAC {
	uint8_t VCO_VC_DAC;
};

struct TXBYTES {
	uint8_t NUM_TXBYTES : 7;
	uint8_t TXFIFO_UNDERFLOW : 1;
};

struct RXBYTES {
	uint8_t NUM_RXBYTES : 7;
	uint8_t RXFIFO_OVERFLOW : 1;
};

struct RCCTRL1_STATUS {
	uint8_t RCCTRL1_STATUS : 7;
	uint8_t : 1;
};

struct RCCTRL0_STATUS {
	uint8_t RCCTRL0_STATUS : 7;
	uint8_t : 1;
};
