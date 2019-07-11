/* offsets des champs des paquets pour chaque protocole
	- tous les paquets sont DIX, dedans on trouve :
		- ARP
		- IP, dans lequel on trouve :
			- ICMP (ping)
			- UDP
			- TCP
   overheads :
	protocole	W/CRC	no CRC
	DIX		18	14
	IP		38	34
	icmp		46	42
	UDP		46	42
	TCP		58	54
 */

// DIX layout (Digital - Intel - Xerox) aka ethernet 2 (overhead 14 head + 4 CRC = 18)

#define	DIX_dest	0x00	// destination mac address
#define	DIX_src		0x06	// source mac address
#define	DIX_type	0x0C	// type-or-length field
#define	DIX_data	0x0E	// IP data area begins here

// ARP Layout

#define	ARP_hwtype	0x0E
#define	ARP_prtype	0x10
#define	ARP_hwlen	0x12
#define	ARP_prlen	0x13
#define	ARP_op		0x14
#define	ARP_shaddr	0x16	// arp source mac address
#define	ARP_sipaddr	0x1C	// arp source ip address
#define	ARP_thaddr	0x20	// arp target mac address
#define	ARP_tipaddr	0x26	// arp target ip address

// IP Header Layout  (overhead 20 + 18 DIX = 38) 

#define	IP_vers_len	0x0E	// IP version and header length
#define	IP_tos		0x0F	// IP type of service
#define	IP_pktlen	0x10	// packet length
#define	IP_id		0x12	// datagram id
#define	IP_frag_offset	0x14	// fragment offset
#define	IP_ttl		0x16	// time to live
#define	IP_proto	0x17	// protocol (ICMP=1, TCP=6, UDP=11)
#define	IP_hdr_cksum	0x18	// header checksum
#define	IP_srcaddr	0x1A	// IP address of source
#define	IP_destaddr	0x1E	// IP addess of destination
#define	IP_data		0x22	// IP data area

// IP Protocol Types 

#define	PROT_ICMP	0x01
#define	PROT_TCP	0x06
#define	PROT_UDP	0x11

// ICMP Header  (overhead 8 + IP 38 = 46)

#define	ICMP_type	IP_data
#define	ICMP_code	ICMP_type+1
#define	ICMP_cksum	ICMP_code+1
#define	ICMP_id		ICMP_cksum+2
#define	ICMP_seqnum	ICMP_id+2
#define ICMP_data	ICMP_seqnum+2

// UDP Header  (overhead 8 + IP 38 = 46)

#define	UDP_srcport	IP_data
#define	UDP_destport	UDP_srcport+2
#define	UDP_len		UDP_destport+2
#define	UDP_cksum	UDP_len+2
#define	UDP_data	UDP_cksum+2

// TCP Header Layout  (overhead 20 min + IP 38 = 58)

#define	TCP_srcport	0x22	// TCP source port
#define	TCP_destport	0x24	// TCP destination port
#define	TCP_seqnum	0x26	// sequence number
#define	TCP_acknum	0x2A	// acknowledgement number
#define	TCP_hdrflags	0x2E	// 4-bit header len and flags
#define	TCP_window	0x30	// window size
#define	TCP_cksum	0x32	// TCP checksum
#define	TCP_urgentptr	0x34	// urgent pointer
#define TCP_data	0x36	// option/data

// TCP Flags
//	IN flags represent incoming bits
//	OUT flags represent outgoing bits

#define  FIN_IN		0
#define  SYN_IN		1
#define  RST_IN		2
#define  PSH_IN		3
#define  ACK_IN		4
#define  URG_IN		5
#define  FIN_OUT	0
#define  SYN_OUT	1
#define  RST_OUT	2
#define  PSH_OUT	3
#define  ACK_OUT	4
#define  URG_OUT	5



