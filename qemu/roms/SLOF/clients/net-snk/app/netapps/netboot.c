/******************************************************************************
 * Copyright (c) 2004, 2008 IBM Corporation
 * All rights reserved.
 * This program and the accompanying materials
 * are made available under the terms of the BSD License
 * which accompanies this distribution, and is available at
 * http://www.opensource.org/licenses/bsd-license.php
 *
 * Contributors:
 *     IBM Corporation - initial implementation
 *****************************************************************************/

#include <netlib/tftp.h>
#include <netlib/ethernet.h>
#include <netlib/dhcp.h>
//#include <netlib/dhcpv6.h>
#include <netlib/ipv4.h>
//#include <netlib/ipv6.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netapps/args.h>
#include <libbootmsg/libbootmsg.h>
#include <of.h>

#define IP_INIT_DEFAULT 2
#define IP_INIT_NONE    0
#define IP_INIT_BOOTP   1
#define IP_INIT_DHCP    2
#define IP_INIT_DHCPV6_STATELESS    3
#define IP_INIT_IPV6_MANUAL         4

#define DEFAULT_BOOT_RETRIES 600
#define DEFAULT_TFTP_RETRIES 20
static int ip_version = 4;

typedef struct {
	char filename[100];
	int  ip_init;
	char siaddr[4];
	//ip6_addr_t si6addr;
	char ciaddr[4];
	//ip6_addr_t ci6addr;
	char giaddr[4];
	//ip6_addr_t gi6addr;
	int  bootp_retries;
	int  tftp_retries;
} obp_tftp_args_t;


/**
 * Parses a argument string for IPv6 booting, extracts all
 * parameters and fills a structure accordingly
 *
 * @param  arg_str        string with arguments, seperated with ','
 * @param  argc           number of arguments
 * @param  obp_tftp_args  structure which contains the result
 * @return                updated arg_str
 */
/*
static const char * 
parse_ipv6args (const char *arg_str, unsigned int argc,
		obp_tftp_args_t *obp_tftp_args)
{
	char *ptr = NULL;
	char arg_buf[100];

	// find out siaddr
	if (argc == 0)
		memset(&obp_tftp_args->si6addr.addr, 0, 16);
	else {
		argncpy(arg_str, 0, arg_buf, 100);
		if(parseip6(arg_buf, (uint8_t *) &(obp_tftp_args->si6addr.addr[0]))) {
			arg_str = get_arg_ptr(arg_str, 1);
			--argc;
		}
		else if(arg_buf[0] == 0) {
			memset(&obp_tftp_args->si6addr.addr, 0, 16);
			arg_str = get_arg_ptr(arg_str, 1);
			--argc;
		}
		else
			memset(&obp_tftp_args->si6addr.addr, 0, 16);
	}

	// find out filename
	if (argc == 0)
		obp_tftp_args->filename[0] = 0;
	else {
		argncpy(arg_str, 0, obp_tftp_args->filename, 100);
		for(ptr = obp_tftp_args->filename; *ptr != 0; ++ptr)
			if(*ptr == '\\') {
				*ptr = '/';
			}
		arg_str = get_arg_ptr(arg_str, 1);
		--argc;
	}

	// find out ciaddr
	if (argc == 0)
		memset(&obp_tftp_args->ci6addr, 0, 16);
	else {
		argncpy(arg_str, 0, arg_buf, 100);
		if (parseip6(arg_buf, (uint8_t *) &(obp_tftp_args->ci6addr.addr)) ) {
			arg_str = get_arg_ptr(arg_str, 1);
			--argc;
		}
		else if(arg_buf[0] == 0) {
			memset(&obp_tftp_args->ci6addr.addr, 0, 16);
			arg_str = get_arg_ptr(arg_str, 1);
			--argc;
		}
		else
			memset(&obp_tftp_args->ci6addr.addr, 0, 16);
	}

	// find out giaddr
	if (argc == 0)
		memset(&obp_tftp_args->gi6addr, 0, 16);
	else {
		argncpy(arg_str, 0, arg_buf, 100);
		if (parseip6(arg_buf, (uint8_t *) &(obp_tftp_args->gi6addr.addr)) ) {
			arg_str = get_arg_ptr(arg_str, 1);
			--argc;
		}
		else if(arg_buf[0] == 0) {
			memset(&obp_tftp_args->gi6addr, 0, 16);
			arg_str = get_arg_ptr(arg_str, 1);
			--argc;
		}
		else
			memset(&obp_tftp_args->gi6addr.addr, 0, 16);
	}

	return arg_str;
}
*/


/**
 * Parses a argument string for IPv4 booting, extracts all
 * parameters and fills a structure accordingly
 *
 * @param  arg_str        string with arguments, seperated with ','
 * @param  argc           number of arguments
 * @param  obp_tftp_args  structure which contains the result
 * @return                updated arg_str
 */
static const char * 
parse_ipv4args (const char *arg_str, unsigned int argc,
		obp_tftp_args_t *obp_tftp_args)
{
	char *ptr = NULL;
	char arg_buf[100];

	// find out siaddr
	if(argc==0) {
		memset(obp_tftp_args->siaddr, 0, 4);
	} else {
		argncpy(arg_str, 0, arg_buf, 100);
		if(strtoip(arg_buf, obp_tftp_args->siaddr)) {
			arg_str = get_arg_ptr(arg_str, 1);
			--argc;
		}
		else if(arg_buf[0] == 0) {
			memset(obp_tftp_args->siaddr, 0, 4);
			arg_str = get_arg_ptr(arg_str, 1);
			--argc;
		}
		else
			memset(obp_tftp_args->siaddr, 0, 4);
	}

	// find out filename
	if(argc==0)
		obp_tftp_args->filename[0] = 0;
	else {
		argncpy(arg_str, 0, obp_tftp_args->filename, 100);
		for(ptr = obp_tftp_args->filename; *ptr != 0; ++ptr)
			if(*ptr == '\\')
				*ptr = '/';
		arg_str = get_arg_ptr(arg_str, 1);
		--argc;
	}

	// find out ciaddr
	if(argc==0)
		memset(obp_tftp_args->ciaddr, 0, 4);
	else {
		argncpy(arg_str, 0, arg_buf, 100);
		if(strtoip(arg_buf, obp_tftp_args->ciaddr)) {
			arg_str = get_arg_ptr(arg_str, 1);
			--argc;
		}
		else if(arg_buf[0] == 0) {
			memset(obp_tftp_args->ciaddr, 0, 4);
			arg_str = get_arg_ptr(arg_str, 1);
			--argc;
		}
		else
			memset(obp_tftp_args->ciaddr, 0, 4);
	}

	// find out giaddr
	if(argc==0)
		memset(obp_tftp_args->giaddr, 0, 4);
	else {
		argncpy(arg_str, 0, arg_buf, 100);
		if(strtoip(arg_buf, obp_tftp_args->giaddr)) {
			arg_str = get_arg_ptr(arg_str, 1);
			--argc;
		}
		else if(arg_buf[0] == 0) {
			memset(obp_tftp_args->giaddr, 0, 4);
			arg_str = get_arg_ptr(arg_str, 1);
			--argc;
		}
		else
			memset(obp_tftp_args->giaddr, 0, 4);
	}

	return arg_str;
}

/**
 * Parses a argument string which is given by netload, extracts all
 * parameters and fills a structure according to this
 *
 * Netload-Parameters:
 *    [bootp,]siaddr,filename,ciaddr,giaddr,bootp-retries,tftp-retries
 *
 * @param  arg_str        string with arguments, seperated with ','
 * @param  obp_tftp_args  structure which contains the result
 * @return                none
 */
static void
parse_args(const char *arg_str, obp_tftp_args_t *obp_tftp_args)
{
	unsigned int argc;
	char arg_buf[100];

	argc = get_args_count(arg_str);

	// find out if we should use BOOTP or DHCP
	if(argc==0)
		obp_tftp_args->ip_init = IP_INIT_DEFAULT;
	else {
		argncpy(arg_str, 0, arg_buf, 100);
		if (strcasecmp(arg_buf, "bootp") == 0) {
			obp_tftp_args->ip_init = IP_INIT_BOOTP;
			arg_str = get_arg_ptr(arg_str, 1);
			--argc;
		}
		else if(strcasecmp(arg_buf, "dhcp") == 0) {
			obp_tftp_args->ip_init = IP_INIT_DHCP;
			arg_str = get_arg_ptr(arg_str, 1);
			--argc;
		}
		else if(strcasecmp(arg_buf, "ipv6") == 0) {
			obp_tftp_args->ip_init = IP_INIT_DHCPV6_STATELESS;
			arg_str = get_arg_ptr(arg_str, 1);
			--argc;
			ip_version = 6;
		}
		else
			obp_tftp_args->ip_init = IP_INIT_DEFAULT;
	}

	if (ip_version == 4) {
		arg_str = parse_ipv4args (arg_str, argc, obp_tftp_args);
	}
/*
	else if (ip_version == 6) {
		arg_str = parse_ipv6args (arg_str, argc, obp_tftp_args);
	}
*/

	// find out bootp-retries
	if (argc == 0)
		obp_tftp_args->bootp_retries = DEFAULT_BOOT_RETRIES;
	else {
		argncpy(arg_str, 0, arg_buf, 100);
		if(arg_buf[0] == 0)
			obp_tftp_args->bootp_retries = DEFAULT_BOOT_RETRIES;
		else {
			obp_tftp_args->bootp_retries = strtol(arg_buf, 0, 10);
			if(obp_tftp_args->bootp_retries < 0)
				obp_tftp_args->bootp_retries = DEFAULT_BOOT_RETRIES;
		}
		arg_str = get_arg_ptr(arg_str, 1);
		--argc;
	}

	// find out tftp-retries
	if (argc == 0)
		obp_tftp_args->tftp_retries = DEFAULT_TFTP_RETRIES;
	else {
		argncpy(arg_str, 0, arg_buf, 100);
		if(arg_buf[0] == 0)
			obp_tftp_args->tftp_retries = DEFAULT_TFTP_RETRIES;
		else {
			obp_tftp_args->tftp_retries = strtol(arg_buf, 0, 10);
			if(obp_tftp_args->tftp_retries < 0)
				obp_tftp_args->tftp_retries = DEFAULT_TFTP_RETRIES;
		}
		arg_str = get_arg_ptr(arg_str, 1);
		--argc;
	}
}

int
netboot(int argc, char *argv[])
{
	char buf[256];
	int rc;
	int len = strtol(argv[2], 0, 16);
	char *buffer = (char *) strtol(argv[1], 0, 16);
	char *ret_buffer = (char *) strtol(argv[3], 0, 16);
	filename_ip_t fn_ip;
	int fd_device;
	tftp_err_t tftp_err;
	obp_tftp_args_t obp_tftp_args;
	char null_ip[4] = { 0x00, 0x00, 0x00, 0x00 };
/*
	char null_ip6[16] = { 0x00, 0x00, 0x00, 0x00,
			     0x00, 0x00, 0x00, 0x00,
			     0x00, 0x00, 0x00, 0x00, 
			     0x00, 0x00, 0x00, 0x00 };
*/
	int huge_load = strtol(argv[4], 0, 10);
	int32_t block_size = strtol(argv[5], 0, 10);
	uint8_t own_mac[6];

	printf("\n");
	printf(" Bootloader 1.6 \n");
	memset(&fn_ip, 0, sizeof(filename_ip_t));

	/***********************************************************
	 *
	 * Initialize network stuff and retrieve boot informations
	 *
	 ***********************************************************/

	/* Wait for link up and get mac_addr from device */
	for(rc=0; rc<DEFAULT_BOOT_RETRIES; ++rc) {
		if(rc > 0) {
			set_timer(TICKS_SEC);
			while (get_timer() > 0);
		}
		fd_device = socket(0, 0, 0, (char*) own_mac);
		if(fd_device != -2)
			break;
		if(getchar() == 27) {
			fd_device = -2;
			break;
		}
	}

	if (fd_device == -1) {
		strcpy(buf,"E3000: (net) Could not read MAC address");
		bootmsg_error(0x3000, &buf[7]);

		write_mm_log(buf, strlen(buf), 0x91);
		return -100;
	}
	else if (fd_device == -2) {
		strcpy(buf,"E3006: (net) Could not initialize network device");
		bootmsg_error(0x3006, &buf[7]);

		write_mm_log(buf, strlen(buf), 0x91);
		return -101;
	}

	printf("  Reading MAC address from device: "
	       "%02x:%02x:%02x:%02x:%02x:%02x\n",
	       own_mac[0], own_mac[1], own_mac[2],
	       own_mac[3], own_mac[4], own_mac[5]);

	// init ethernet layer
	set_mac_address(own_mac);

	if (argc > 6) {
		parse_args(argv[6], &obp_tftp_args);
		if(obp_tftp_args.bootp_retries - rc < DEFAULT_BOOT_RETRIES)
			obp_tftp_args.bootp_retries = DEFAULT_BOOT_RETRIES;
		else
			obp_tftp_args.bootp_retries -= rc;
	}
	else {
		memset(&obp_tftp_args, 0, sizeof(obp_tftp_args_t));
		obp_tftp_args.ip_init = IP_INIT_DEFAULT;
		obp_tftp_args.bootp_retries = DEFAULT_BOOT_RETRIES;
		obp_tftp_args.tftp_retries = DEFAULT_TFTP_RETRIES;
	}
	memcpy(&fn_ip.own_ip, obp_tftp_args.ciaddr, 4);

	//  reset of error code
	rc = 0;

	/* if we still have got all necessary parameters, then we don't
	   need to perform an BOOTP/DHCP-Request */
	if (ip_version == 4) {
		if (memcmp(obp_tftp_args.ciaddr, null_ip, 4) != 0
		    && memcmp(obp_tftp_args.siaddr, null_ip, 4) != 0
		    && obp_tftp_args.filename[0] != 0) {

			memcpy(&fn_ip.server_ip, &obp_tftp_args.siaddr, 4);
			obp_tftp_args.ip_init = IP_INIT_NONE;
		}
	}
/*
	else if (ip_version == 6) {
		if (memcmp(&obp_tftp_args.ci6addr, null_ip6, 16) != 0
		    && memcmp(&obp_tftp_args.si6addr, null_ip6, 16) != 0
		    && obp_tftp_args.filename[0] != 0) {

			memcpy(&fn_ip.server_ip6.addr[0], 
			       &obp_tftp_args.si6addr.addr, 16);
			obp_tftp_args.ip_init = IP_INIT_IPV6_MANUAL;
		} 
		else {
			obp_tftp_args.ip_init = IP_INIT_DHCPV6_STATELESS;
		}
	}
*/
	// construction of fn_ip from parameter
	switch(obp_tftp_args.ip_init) {
	case IP_INIT_BOOTP:
		printf("  Requesting IP address via BOOTP: ");
		// if giaddr in not specified, then we have to identify
		// the BOOTP server via broadcasts
		if(memcmp(obp_tftp_args.giaddr, null_ip, 4) == 0) {
			// don't do this, when using DHCP !!!
			fn_ip.server_ip = 0xFFFFFFFF;
		}
		// if giaddr is specified, then we have to use this
		// IP address as proxy to identify the BOOTP server
		else {
			memcpy(&fn_ip.server_ip, obp_tftp_args.giaddr, 4);
		}
		rc = bootp(ret_buffer, &fn_ip, obp_tftp_args.bootp_retries);
		break;
	case IP_INIT_DHCP:
		printf("  Requesting IP address via DHCP: ");
		rc = dhcp(ret_buffer, &fn_ip, obp_tftp_args.bootp_retries);
		break;
/*
	case IP_INIT_DHCPV6_STATELESS:
		set_ipv6_address(0);
		rc = do_dhcpv6 (ret_buffer, &fn_ip, 10, DHCPV6_STATELESS);
		break;
	case IP_INIT_IPV6_MANUAL:
		set_ipv6_address(&obp_tftp_args.ci6addr);
		break;
*/
	case IP_INIT_NONE:
	default:
		break;
	}

	if(rc >= 0 && ip_version == 4) {
		if(memcmp(obp_tftp_args.ciaddr, null_ip, 4) != 0
		&& memcmp(obp_tftp_args.ciaddr, &fn_ip.own_ip, 4) != 0)
			memcpy(&fn_ip.own_ip, obp_tftp_args.ciaddr, 4);

		if(memcmp(obp_tftp_args.siaddr, null_ip, 4) != 0
		&& memcmp(obp_tftp_args.siaddr, &fn_ip.server_ip, 4) != 0)
			memcpy(&fn_ip.server_ip, obp_tftp_args.siaddr, 4);

		// init IPv4 layer
		set_ipv4_address(fn_ip.own_ip);
	}
/*
	else if (rc >= 0 && ip_version == 6) {
		if(memcmp(&obp_tftp_args.ci6addr.addr, null_ip6, 16) != 0
		&& memcmp(&obp_tftp_args.ci6addr.addr, &fn_ip.own_ip6, 16) != 0)
			memcpy(&fn_ip.own_ip6, &obp_tftp_args.ci6addr.addr, 16);

		if(memcmp(&obp_tftp_args.si6addr.addr, null_ip6, 16) != 0
		&& memcmp(&obp_tftp_args.si6addr.addr, &fn_ip.server_ip6.addr, 16) != 0)
			memcpy(&fn_ip.server_ip6.addr, &obp_tftp_args.si6addr.addr, 16);
	}
*/
	if (rc == -1) {
		strcpy(buf,"E3001: (net) Could not get IP address");
		bootmsg_error(0x3001, &buf[7]);

		write_mm_log(buf, strlen(buf), 0x91);
		return -101;
	}

	printf("%d.%d.%d.%d\n",
	       ((fn_ip.own_ip >> 24) & 0xFF), ((fn_ip.own_ip >> 16) & 0xFF),
	       ((fn_ip.own_ip >>  8) & 0xFF), ( fn_ip.own_ip        & 0xFF));

	if (rc == -2) {
		sprintf(buf,
			"E3002: (net) ARP request to TFTP server "
			"(%d.%d.%d.%d) failed",
			((fn_ip.server_ip >> 24) & 0xFF),
			((fn_ip.server_ip >> 16) & 0xFF),
			((fn_ip.server_ip >>  8) & 0xFF),
			( fn_ip.server_ip        & 0xFF));
		bootmsg_error(0x3002, &buf[7]);

		write_mm_log(buf, strlen(buf), 0x91);
		return -102;
	}
	if (rc == -4 || rc == -3) {
		strcpy(buf,"E3008: (net) Can't obtain TFTP server IP address");
		bootmsg_error(0x3008, &buf[7]);

		write_mm_log(buf, strlen(buf), 0x91);
		return -107;
	}


	/***********************************************************
	 *
	 * Load file via TFTP into buffer provided by OpenFirmware
	 *
	 ***********************************************************/

	if (obp_tftp_args.filename[0] != 0) {
		strncpy((char *) fn_ip.filename, obp_tftp_args.filename, sizeof(fn_ip.filename)-1);
		fn_ip.filename[sizeof(fn_ip.filename)-1] = 0;
	}

	printf("  Requesting file \"%s\" via TFTP from %d.%d.%d.%d\n",
		fn_ip.filename,
		((fn_ip.server_ip >> 24) & 0xFF),
		((fn_ip.server_ip >> 16) & 0xFF),
		((fn_ip.server_ip >>  8) & 0xFF),
		( fn_ip.server_ip        & 0xFF));

	// accept at most 20 bad packets
	// wait at most for 40 packets
	rc = tftp(&fn_ip, (unsigned char *) buffer,
	          len, obp_tftp_args.tftp_retries,
	          &tftp_err, huge_load, block_size, ip_version);

	if(obp_tftp_args.ip_init == IP_INIT_DHCP)
		dhcp_send_release();

	if (rc > 0) {
		printf("  TFTP: Received %s (%d KBytes)\n", fn_ip.filename,
		       rc / 1024);
	} else if (rc == -1) {
		bootmsg_error(0x3003, "(net) unknown TFTP error");
		return -103;
	} else if (rc == -2) {
		sprintf(buf,
			"E3004: (net) TFTP buffer of %d bytes "
			"is too small for %s",
			len, fn_ip.filename);
		bootmsg_error(0x3004, &buf[7]);

		write_mm_log(buf, strlen(buf), 0x91);
		return -104;
	} else if (rc == -3) {
		sprintf(buf,"E3009: (net) file not found: %s",
		       fn_ip.filename);
		bootmsg_error(0x3009, &buf[7]);

		write_mm_log(buf, strlen(buf), 0x91);
		return -108;
	} else if (rc == -4) {
		strcpy(buf,"E3010: (net) TFTP access violation");
		bootmsg_error(0x3010, &buf[7]);

		write_mm_log(buf, strlen(buf), 0x91);
		return -109;
	} else if (rc == -5) {
		strcpy(buf,"E3011: (net) illegal TFTP operation");
		bootmsg_error(0x3011, &buf[7]);

		write_mm_log(buf, strlen(buf), 0x91);
		return -110;
	} else if (rc == -6) {
		strcpy(buf, "E3012: (net) unknown TFTP transfer ID");
		bootmsg_error(0x3012, &buf[7]);

		write_mm_log(buf, strlen(buf), 0x91);
		return -111;
	} else if (rc == -7) {
		strcpy(buf, "E3013: (net) no such TFTP user");
		bootmsg_error(0x3013, &buf[7]);

		write_mm_log(buf, strlen(buf), 0x91);
		return -112;
	} else if (rc == -8) {
		strcpy(buf, "E3017: (net) TFTP blocksize negotiation failed");
		bootmsg_error(0x3017, &buf[7]);

		write_mm_log(buf, strlen(buf), 0x91);
		return -116;
	} else if (rc == -9) {
		strcpy(buf,"E3018: (net) file exceeds maximum TFTP transfer size");
		bootmsg_error(0x3018, &buf[7]);

		write_mm_log(buf, strlen(buf), 0x91);
		return -117;
	} else if (rc <= -10 && rc >= -15) {
		sprintf(buf,"E3005: (net) ICMP ERROR \"");
		switch (rc) {
		case -ICMP_NET_UNREACHABLE - 10:
			sprintf(buf+strlen(buf),"net unreachable");
			break;
		case -ICMP_HOST_UNREACHABLE - 10:
			sprintf(buf+strlen(buf),"host unreachable");
			break;
		case -ICMP_PROTOCOL_UNREACHABLE - 10:
			sprintf(buf+strlen(buf),"protocol unreachable");
			break;
		case -ICMP_PORT_UNREACHABLE - 10:
			sprintf(buf+strlen(buf),"port unreachable");
			break;
		case -ICMP_FRAGMENTATION_NEEDED - 10:
			sprintf(buf+strlen(buf),"fragmentation needed and DF set");
			break;
		case -ICMP_SOURCE_ROUTE_FAILED - 10:
			sprintf(buf+strlen(buf),"source route failed");
			break;
		default:
			sprintf(buf+strlen(buf)," UNKNOWN");
			break;
		}
		sprintf(buf+strlen(buf),"\"");
		bootmsg_error(0x3005, &buf[7]);

		write_mm_log(buf, strlen(buf), 0x91);
		return -105;
	} else if (rc == -40) {
		sprintf(buf,
			"E3014: (net) TFTP error occurred after "
			"%d bad packets received",
			tftp_err.bad_tftp_packets);
		bootmsg_error(0x3014, &buf[7]);
		write_mm_log(buf, strlen(buf), 0x91);
		return -113;
	} else if (rc == -41) {
		sprintf(buf,
			"E3015: (net) TFTP error occurred after "
			"missing %d responses",
			tftp_err.no_packets);
		bootmsg_error(0x3015, &buf[7]);
		write_mm_log(buf, strlen(buf), 0x91);
		return -114;
	} else if (rc == -42) {
		sprintf(buf,
			"E3016: (net) TFTP error missing block %d, "
			"expected block was %d",
			tftp_err.blocks_missed,
			tftp_err.blocks_received);
		bootmsg_error(0x3016, &buf[7]);
		write_mm_log(buf, strlen(buf), 0x91);
		return -115;
	}
	return rc;
}
