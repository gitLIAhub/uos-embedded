#include <runtime/lib.h>
#include <kernel/uos.h>
#include <mem/mem.h>
#include <buf/buf.h>
#include <net/netif.h>
#include <net/route.h>
#include <net/arp.h>
#include <net/ip.h>

//#undef ARP_TRACE
#ifndef ARP_PRINTF
#define ARP_PRINTF(...)
#endif

#ifdef DEBUG_NET_ARPTABLE
#define ARPTABLE_printf(...) ARP_PRINTF(__VA_ARGS__)
#else
#define ARPTABLE_printf(...)
#endif

#ifdef ARP_TRACE
#define ARPTRACE_printf(...) ARP_PRINTF(__VA_ARGS__)
#else
#define ARPTRACE_printf(...)
#endif



static const unsigned char BROADCAST[6] = {'\xff','\xff','\xff','\xff','\xff','\xff'};

/*
 * Initialize the ARP data strucure.
 * The arp_t structure must be zeroed before calling arp_init().
 * The size of ARP table depends on the size of area, allocated for `arp'.
 * The `route' argument is a reference to routing table, needed for
 * processing incoming ARP requests.
 */
arp_t *
arp_init (array_t *buf, unsigned bytes, struct _ip_t *ip)
{
	arp_t *arp;

	/* MUST be compiled with "pack structs" or equivalent! */
	assert (sizeof (struct eth_hdr) == 14);

	arp = (arp_t*) buf;
	arp->ip = ip;
	arp->timer = 0;
	arp->stamp = 0;
	arp->size = 1 + (bytes - sizeof(arp_t)) / sizeof(arp_entry_t);
	/*debug_printf ("arp_init: %d entries\n", arp->size);*/
	return arp;
}

/*
 * Find an Ethernet address by IP address in ARP table.
 * Mark it as age = 0.
 */
unsigned char *
arp_lookup (netif_t *netif, ip_addr_const ipaddr)
{
	arp_t *arp = netif->arp;
	arp_entry_t *e;

	for (e = arp->table; e < arp->table + arp->size; ++e)
		if (e->netif == netif && ipadr_is_same(e->ipaddr.var, ipaddr) ) {
			/*debug_printf ("arp_lookup: %d.%d.%d.%d -> %02x-%02x-%02x-%02x-%02x-%02x\n",
				ipaddr[0], ipaddr[1], ipaddr[2], ipaddr[3],
				e->ethaddr[0], e->ethaddr[1], e->ethaddr[2],
				e->ethaddr[3], e->ethaddr[4], e->ethaddr[5]);*/
			/* LY-TODO: не нужно сбрасывать age. */
			e->age = 0;
			return e->ethaddr.ucs;
		}
	/*debug_printf ("arp_lookup failed: ipaddr = %d.%d.%d.%d\n",
		ipaddr[0], ipaddr[1], ipaddr[2], ipaddr[3]);*/
	return 0;
}

/*
 * Add a new entry to ARP table.
 * Mark new entries as age = 0.
 */
static void
arp_add_entry (netif_t *netif, ip_addr_const ipaddr, const unsigned char *_ethaddr)
{
	arp_t *arp = netif->arp;
	arp_entry_t *e, *q;
	mac_addr    ethaddr;

	if (ipref_as_ucs(ipaddr)[0] == 0)
		return;

	ethaddr = macadr_4ucs(_ethaddr);
	
	
	/* Walk through the ARP mapping table and try to find an entry to
	 * update. If none is found, the IP -> MAC address mapping is
	 * inserted in the ARP table. */
	for (e = arp->table; e < arp->table + arp->size; ++e) {
		/* Only check those entries that are actually in use. */
		if (! e->netif)
			continue;

		/* IP address match? */
		if (ipadr_is_same(e->ipaddr.var, ipaddr)) {
			/* An old entry found, update this and return. */
			if (!macadr_is_same(&e->ethaddr, &ethaddr) ||
			    (e->netif != netif)) 
			{
			    ARPTRACE_printf ("arp: entry %@.4D changed from %#6D netif %s\n",
					ipaddr, e->ethaddr, e->netif->name);
			    ARPTRACE_printf("     to %#6D netif %s\n",
					ethaddr, netif->name);
				macadr_assign(&e->ethaddr, &ethaddr);
				e->netif = netif;
				++(arp->stamp);
			}
			e->age = 0;
			return;
		}
	}

	/* If we get here, no existing ARP table entry was found, so we
	 * create one. */

	/* First, we try to find an unused entry in the ARP table. */
	for (e = arp->table; e < arp->table + arp->size; ++e)
		if (! e->netif)
			break;

	/* If no unused entry is found, we try to find the oldest entry and
	 * throw it away. */
	if (e >= arp->table + arp->size) {
		e = arp->table;
		for (q = arp->table; q < arp->table + arp->size; ++q) {
			if (q->age > e->age)
				e = q;
		}
		ARPTABLE_printf ("arp: delete entry %@.4D %#6D netif %s age %d\n",
			e->ipaddr.ucs, e->ethaddr.ucs,
			e->netif->name, e->age);
	}

	/* Now, fill this table entry with the new information. */
	ipadr_assignref(&e->ipaddr, ipaddr);
	macadr_assign(&e->ethaddr, &ethaddr);
	e->netif = netif;
	e->age = 0;
    ++(arp->stamp);
	ARPTABLE_printf ("arp: create entry %@.4D %#6D netif %s\n",
        e->ipaddr.ucs, e->ethaddr.ucs, e->netif->name);
}

/*
 * Process the packet, received by adapter.
 * Process ARP requests. For IP packets, strip off the ethernet header.
 */
buf_t *
arp_input (netif_t *netif, buf_t *p)
{
	struct ethip_hdr *h;
	struct arp_hdr *ah;
	const unsigned char *ipaddr;

	/*debug_printf ("arp_input: %d bytes\n", p->tot_len);*/
	h = (struct ethip_hdr*) p->payload;
	switch (h->eth.proto) {
	default:
		/* Unknown protocol. */
	    ARPTRACE_printf ("arp_input: unknown protocol 0x%x\n", h->eth.proto);
		buf_free (p);
		return 0;

	case PROTO_IP:
	    //ARPTRACE_printf ("arp: ip packet from %@.4D %#6D\n", h->ip_src, h->eth.src);

		/* For unicasts, update an ARP entry, independent of
		 * the source IP address. */
		if (h->eth.dest[0] != 255) {
		    //ARPTRACE_printf ("arp: check entry %@.4D %#6D\n", h->ip_src.ucs, h->eth.src);
			arp_add_entry (netif, h->ip_src.var, h->eth.src);
		}

		/* Strip off the Ethernet header. */
		buf_add_header (p, -MAC_HLEN);
		return p;

	case PROTO_ARP:
		if (p->tot_len < sizeof (struct arp_hdr)) {
			/* debug_printf ("arp_input: too short packet\n"); */
			buf_free (p);
			return 0;
		}
		buf_truncate (p, sizeof (struct arp_hdr));
		p = buf_make_continuous (p);

		ah = (struct arp_hdr*) p->payload;
		switch (ah->opcode) {
		default:
			/* Unknown ARP operation code. */
			/* debug_printf ("arp_input: unknown opcode 0x%x\n", ah->opcode); */
			buf_free (p);
			return 0;

		case ARP_REQUEST:
			ARPTRACE_printf ("arp: got request for %@.4D\n", ah->dst_ipaddr);
			ARPTRACE_printf ("     from %@.4D %#6D\n", ah->src_ipaddr, ah->src_hwaddr);

			//arp_add_entry (netif, ah->src_ipaddr, ah->src_hwaddr);

			/* ARP request. If it asked for our address,
			 * we send out a reply. */
			ipaddr = route_lookup_ipaddr (netif->arp->ip,
			        ipref_4ucs(ah->dst_ipaddr), netif);

			if (!ipadr_or0_is_same_ucs(ipaddr, ah->dst_ipaddr)) {
				buf_free (p);
				return 0;
			}
			ah->opcode = ARP_REPLY;

			ipadr_assign_ucs(ah->dst_ipaddr, ah->src_ipaddr);
			ipadr_assign_ucs(ah->src_ipaddr, ipaddr);

			macadr_assign_ucs(ah->dst_hwaddr, ah->src_hwaddr);
			macadr_assign_ucs(ah->src_hwaddr, netif->ethaddr.ucs);
			macadr_assign_ucs(ah->eth.dest, ah->dst_hwaddr);
			macadr_assign_ucs(ah->eth.src, netif->ethaddr.ucs);

			ah->eth.proto = PROTO_ARP;

			ARPTRACE_printf ("arp: send reply %@.4D %s %#6D\n"
			        , ipaddr, netif->name, netif->ethaddr.ucs);
			ARPTRACE_printf ("     to %@.4D %#6D\n", ah->dst_ipaddr, ah->dst_hwaddr);

			netif->interface->output (netif, p, 0);
			return 0;

		case ARP_REPLY:
			/* ARP reply. We insert or update the ARP table.
			 * No need to check the destination IP address. */
		    ARPTRACE_printf ("arp: got reply from %@.4D %#6D\n"
		                     , ah->src_ipaddr,ah->src_hwaddr);
			arp_add_entry (netif, ipref_4ucs(ah->src_ipaddr), ah->src_hwaddr);
			buf_free (p);
			return 0;
		}
	}
}

/*
 * Create ARP request packet.
 * Argument `ipsrc' is the IP address of the network interface,
 * from which we send the ARP request. The network interface could have
 * several IP adresses (aliases), so ipsrc is needed to select
 * concrete alias.
 */
bool_t
arp_request (netif_t *netif, buf_t *p
        , ip_addr_const ipdest
        , ip_addr_const ipsrc)
{
	struct arp_hdr *ah;

	if (p != 0) {
	/* ARP packet place at the begin of buffer (offset 2) */
	buf_add_header (p, p->payload - (unsigned char*) p - sizeof (buf_t) - 2);

	if (p->tot_len < sizeof (struct arp_hdr)) {
		/* Not enough space for ARP packet. */
		netif_free_buf (netif, p);
		return 0;
	}
	buf_truncate (p, sizeof (struct arp_hdr));
	p = buf_make_continuous (p);
	}
	else{
	    p = buf_alloc(netif->arp->ip->pool
	                , IP_ALIGNED( (sizeof (struct arp_hdr)) )
	                , IP_ALIGNED( sizeof (struct eth_hdr)   )
	                );
	    if (p == 0)
	        return 0;
	    buf_add_header(p, sizeof (struct eth_hdr));
	}//if (p != 0)

	ah = (struct arp_hdr*) p->payload;
	ah->eth.proto = PROTO_ARP;
	macadr_assign_ucs(ah->eth.dest, BROADCAST);
	macadr_assign_ucs(ah->eth.src, netif->ethaddr.ucs);

	ah->opcode = ARP_REQUEST;
	ah->hwtype = HWTYPE_ETHERNET;
	ah->hwlen = 6;
	ah->proto = PROTO_IP;
	ah->protolen = 4;

	/* Most implementations set dst_hwaddr to zero. */
	memset (ah->dst_hwaddr, 0, 6);
	macadr_assign_ucs (ah->src_hwaddr, netif->ethaddr.ucs);
	ipadr_assignref_ucs(ah->dst_ipaddr, ipdest);
	ipadr_assignref_ucs(ah->src_ipaddr, ipsrc);

	ARPTRACE_printf ("arp: send request for %@.4D\n", ipref_as_ucs(ipdest));
	ARPTRACE_printf ("     from %@.4D %#6D\n"
	        , ipref_as_ucs(ipsrc), netif->ethaddr.ucs
	        );
	return netif->interface->output (netif, p, 0);
}

/*
 * Add Ethernet header for transmitted packet.
 * For broadcasts, ipdest must be NULL.
 */
bool_t
arp_add_header (netif_t *netif, buf_t *p
        , ip_addr_const ipdest
        , const unsigned char *ethdest)
{
	struct eth_hdr *h;

	/* Make room for Ethernet header. */
	if (! buf_add_header (p, sizeof (struct eth_hdr) )) {
		/* No space for header, deallocate packet. */
		ARPTRACE_printf ("arp_output: no space for header %d\n",
		        ( (p->payload - (unsigned char*) p) - sizeof (buf_t) )
		        );
		return 0;
	}
	h = (struct eth_hdr*) p->payload;

	/* Construct Ethernet header. Start with looking up deciding which
	 * MAC address to use as a destination address. Broadcasts and
	 * multicasts are special, all other addresses are looked up in the
	 * ARP table. */
	if (! ipadr_not0(ipdest)) {
		/* Broadcast. */
	    macadr_assign_ucs(h->dest, BROADCAST);

	} else if ((ipref_as_ucs(ipdest)[0] & 0xf0) == 0xe0) {
		/* Hash IP multicast address to MAC address. */
		h->dest[0] = 0x01;
		h->dest[1] = 0;
		h->dest[2] = 0x5e;
		h->dest[3] = ipref_as_ucs(ipdest)[1] & 0x7f;
		h->dest[4] = ipref_as_ucs(ipdest)[2];
		h->dest[5] = ipref_as_ucs(ipdest)[3];

	} else {
		macadr_assign_ucs(h->dest, ethdest);
	}

	h->proto = PROTO_IP;
	macadr_assign_ucs (h->src, netif->ethaddr.ucs);
	return 1;
}

/*
 * Aging all arp entry. Deleting old entries. By Serg Lvov.
 * Called 10 times per second.
 */
void
arp_timer (arp_t *arp)
{
	arp_entry_t *e;

	++arp->timer;
	if (arp->timer < 50)
		return;
	arp->timer = 0;

	/*
	 * Every 5 seconds walk through the ARP mapping table
	 * and try to find an entry to aging.
	 */
	 for (e = arp->table; e < arp->table + arp->size; ++e) {
		/* Only check those entries that are actually in use. */
		if (! e->netif)
			continue;

		/* Standard aging time is 300 seconds. */
		if (++e->age > 300/5) {
			/* debug_printf ("arp: delete entry %d.%d.%d.%d %02x-%02x-%02x-%02x-%02x-%02x netif %s age %d\n",
				e->ipaddr[0], e->ipaddr[1], e->ipaddr[2], e->ipaddr[3],
				e->ethaddr[0], e->ethaddr[1], e->ethaddr[2],
				e->ethaddr[3], e->ethaddr[4], e->ethaddr[5],
				e->netif->name, e->age); */
			e->netif = 0;
		}
	}
}
