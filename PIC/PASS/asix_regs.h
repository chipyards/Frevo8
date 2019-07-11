/* registres et bits ASIX */

// core registers
//			RW	R	W
#define CR		0x00
#define PSTART		0x01
#define PSTOP		0x02
#define BNRY		0x03
#define TSR			0x04
#define TPSR				0x04
#define NCR			0x05
#define CPRRD			0x06
#define TBCR0				0x05
#define TBCR1				0x06
#define ISR		0x07
#define CPR		0x07   // Page 1
#define RSAR0		0x08
#define RSAR1		0x09
#define RBCR0				0x0A
#define RBCR1				0x0B
#define RSR			0x0C
#define RCR				0x0C
#define TCR				0x0D
#define DCR				0x0E
#define IMR				0x0F
#define DMAPORT		0x10
#define MEMR		0x14
#define TR		0x15
#define GPOC      			0x17
#define GPI			0x17
#define RSTPORT			0x1F
#define MACA0		0x01	// page 1

// CR bits "Command Register"
#define PAGE1	0x40
#define RDMA	0x08	// remote read
#define WDMA	0x10	// remote write
#define ADMA	0x20	// abort DMA
#define	TXP	0x04	// transmit current packet
#define START	0x02
#define STOP	0x01

// ISR bits "Interrupt Status" (write 1 to bit to clear it !)
#define RST	0x80	// NIC is reset, waiting for START, self clear
#define RDC	0x40	// remote DMA complete
#define OVW	0x10	// RX ring overwrite
#define PTX	0x02	// packet transmitted OK
#define PRX	0x01	// packet received OK

// IMR bits "Interrupt Mask" (write 1 to enable)
#define OVWE	0x10	// OVW enable
#define PTXE	0x02	// PTX enable
#define PRXE	0x01	// PRX enable

// DCR bits "Data Configuration"
#define WTS	0x01	// 0 for byte-wide bus

// TCR bits "Transmit Config"
#define FDU	0x80	// full duplex
#define PD	0x40	// pad disable (default is pad short packet)
#define LB1	0x04	// PHY loopback
#define LB0	0x02	// MAC loopback

// RCR bits "Receive Config"
#define INTT	0x40	// interrupt pin active level
#define MON	0x20	// Monitor mode (rx packet not copied in ring)
#define PRO	0x10	// Promiscuous mode
#define AB	0x04	// Accept Broadcast

// GPI bits
#define I_SPD	0x04	// PHY speed status (1 = 100M)
#define I_DPX	0x02	// PHY duplex status (1 = full)
#define I_LINK  0x01	// PHY link status

// GPOC bits
#define MPSET	0x20	// 0 for internal PHY 
#define MPSEL	0x10	// 1 for MPSET enable

// MII internal PHY registers
#define	MR0 	0x00	// Control register
#define	MR1 	0x01	// Status register
#define	MR28 	0x1C	// Device Specific Status register

// MR0 bits 
#define SW_RST	0x8000	// PHY software reset (self clear)
#define LOOP	0x4000	// PHY loopback
#define SPEED	0x2000	// 1 ==> 100 Mbits/s
#define NWAY_E	0x1000	// Auto Negotiation Enable
#define PWRDN	0x0800	// PHY power down
#define RE_NWAY	0x0400	// Restart auto nego (self clear)

// MR1 bits
#define NWAY_OK	0x0020	// Autonegotiation complete
#define LINK_OK	0x0004	// Link valid (failure ==> 0 latched until read)

// MR28 bits
#define UP100	0x0002	// 100M transceiver up
#define UP10	0x0001	// 10M transceiver up
