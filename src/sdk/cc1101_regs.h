#pragma once
#include <stdint.h>

enum {
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
	uint8_t SYNC : 8; // Sync Word MSB, 0xd3 default
};

// Sync Word, Low Byte
struct SYNC2 {
	uint8_t SYNC2 : 8; // Sync Word LSB, 0x91 default
};

// Packet Length
struct PKTLEN {
	// Packet length in fixed-length mode or maximum length in
	// variable-length packet mode. Defaults to 0xff.
	uint8_t PACKET_LENGTH : 8;
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
	uint8_t DEVICE_ADDR : 8;
};

// Channel Number
struct CHANNR {
	// Channel number which is multiplied by the channel
	// spacing setting and added to the base frequency.
	uint8_t CHAN : 8;
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
	int8_t FREQOFF : 8;
};

// Frequency Control Word, High Byte
struct FREQ2 {
	// f_CARRIER = (f_XOSC * FREQ) >> 16
	// Default FREQ = 0x1ec4ec (2016492), 800+ MHz
	uint8_t FREQ : 8;
};

// Frequency Control Word, Middle Byte
struct FREQ1 {
	uint8_t FREQ : 8;
};

// Frequency Control Word, Low Byte
struct FREQ0 {
	uint8_t FREQ : 8;
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
	uint8_t DRATE_M : 8;
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
	uint8_t CHANSPC_M : 8;
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
