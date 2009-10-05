/*  BubbleMon dockapp 1.2 - OpenBSD specific code
 *  Copyright (C) 2001, Peter Stromberg <wilfried@openbsd.org>
 *  
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 *
 */

#include <stdlib.h>
#include <unistd.h>
#include <sys/dkstat.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/resource.h>

#include <uvm/uvm_object.h>
#include <uvm/uvm_extern.h>
#include <sys/swap.h>

#include "include/bubblemon.h"
#include "include/sys_include.h"

extern BubbleMonData bm;

/* Returns the current CPU load in percent */
int system_cpu(void)
{
	int loadPercentage;
	int previous_total, previous_load;
	int total, load;
	long cpu_time[CPUSTATES];
	int i;

	int mib[2];
	size_t size;

	mib[0] = CTL_KERN;
	mib[1] = KERN_CPTIME;
	size = sizeof (cpu_time);

	if (sysctl(mib, 2, &cpu_time, &size, NULL, 0) < 0)
	return 0;

	load = cpu_time[CP_USER] + cpu_time[CP_SYS] + cpu_time[CP_NICE];
	total = load + cpu_time[CP_IDLE];

	i = bm.loadIndex;
	previous_load = bm.load[i];
	previous_total = bm.total[i];

	bm.load[i] = load;
	bm.total[i] = total;
	bm.loadIndex = (i + 1) % bm.samples;

	if (previous_total == 0)
		loadPercentage = 0;	/* first time here */
	else if (total == previous_total)
		loadPercentage = 100;
	else
		loadPercentage = (100 * (load - previous_load)) / (total - previous_total);

	return loadPercentage;
}

int system_memory(void)
{
#define pagetob(size) ((size) << (uvmexp.pageshift))
	struct uvmexp uvmexp;
	int nswap, rnswap, i;
	int mib[] = { CTL_VM, VM_UVMEXP };
	size_t size = sizeof (uvmexp);

	if (sysctl(mib, 2, &uvmexp, &size, NULL, 0) < 0)
		return 0;

	bm.mem_used = pagetob(uvmexp.active);
	bm.mem_max = pagetob(uvmexp.npages);
	bm.swap_used = 0;
	bm.swap_max = 0;
	if ((nswap = swapctl(SWAP_NSWAP, 0, 0)) != 0) {
		struct swapent *swdev = malloc(nswap * sizeof(*swdev));
		if((rnswap = swapctl(SWAP_STATS, swdev, nswap)) != nswap) {
			for (i = 0; i < nswap; i++) {
				if (swdev[i].se_flags & SWF_ENABLE) {
					bm.swap_used += (swdev[i].se_inuse / (1024 / DEV_BSIZE));
					bm.swap_max += (swdev[i].se_nblks / (1024 / DEV_BSIZE));
				}
			}
		}
		free(swdev);
	}

	return 1;
}

#ifdef ENABLE_MEMSCREEN
void system_loadavg(void)
{
	static int avg_delay;

	if (avg_delay-- <= 0) {
		struct loadavg loadinfo;
		int i;
		int mib[] = { CTL_VM, VM_LOADAVG };
		size_t size = sizeof (loadinfo);

		if (sysctl(mib, 2, &loadinfo, &size, NULL, 0) >= 0)
			for (i = 0; i < 3; i++) {
				bm.loadavg[i].i = loadinfo.ldavg[i] / loadinfo.fscale;
				bm.loadavg[i].f = ((loadinfo.ldavg[i] * 100 + 
				loadinfo.fscale / 2) / loadinfo.fscale) % 100;
			}

		avg_delay = ROLLVALUE;
	}
}
#endif				/* ENABLE_MEMSCREEN */

#ifdef ENABLE_FISH

#include <string.h>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <net/if.h>

#define FISH_MAX_SPEED	8
#define DIFF_MIN 10

/* The actual speed for the fish... */
int tx_speed;
int rx_speed;

u_int64_t tx_amount;
u_int64_t rx_amount;

/* Store the last one to compare */
u_int64_t last_tx_amount;
u_int64_t last_rx_amount;

/* Store the max for scaling, too. */
u_int64_t max_tx_diff = DIFF_MIN;
u_int64_t max_rx_diff = DIFF_MIN;


/* The cnt for scaling */
int tx_cnt;
int rx_cnt;
int delay;

extern int fish_traffic;
void get_traffic(void);

int net_tx_speed(void)
{
	get_traffic();
	return tx_speed;
}


int net_rx_speed(void)
{
	get_traffic();
	return rx_speed;
}


void get_traffic(void)
{
	struct ifaddrs *ifap, *ifa;
	struct if_data *ifd;
	u_int64_t diff;

	/* Have some delay in updating/sampling traffic... */
	if(delay++ < 5) {
		return;
	}
	else {
		delay = 0;
	}

	if (getifaddrs(&ifap) < 0)
		return;

	rx_amount = tx_amount = 0;
	for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
		if (ifa->ifa_flags & IFF_UP) {
			if (ifa->ifa_addr->sa_family != AF_LINK)
				continue;
			ifd = (struct if_data *)ifa->ifa_data;
			/* ignore case */
			if ((strncmp(ifa->ifa_name, "lo", 2) != 0) &&
			    (strncmp(ifa->ifa_name, "pflog", 5) != 0)) {
				rx_amount += ifd->ifi_ibytes;
				tx_amount += ifd->ifi_obytes;
			}
		}
	}
	freeifaddrs(ifap);

	/* Incoming traffic */
	if (rx_amount != last_rx_amount) {
		if (last_rx_amount == 0) {
			last_rx_amount = rx_amount;
		}
		diff = rx_amount - last_rx_amount;
		last_rx_amount = rx_amount;
		rx_speed = FISH_MAX_SPEED * diff / max_rx_diff;
		if(rx_speed == 0) {
			/*
			 * At least, make it move a bit, cos we know there's
			 * traffic
			 */
			rx_speed = 1;
		}
		/*
		  Do something to max rate, to do proper (hopefully)
		  scaling
		*/
		if (max_rx_diff < diff) {
			max_rx_diff = diff;
		}
		else {
			/* Slowly lower the scale */
			if(++rx_cnt > 5) {
				max_rx_diff = diff;
				if (max_rx_diff < DIFF_MIN) {
					/* And don't scale it too low */
					max_rx_diff = DIFF_MIN;
				}
				rx_cnt = 0;
			}
		}
	}
	else
	{
		rx_speed = 0;
	}
	/* Outgoing traffic */
	if (tx_amount != last_tx_amount) {
		if (last_tx_amount == 0) {
			last_tx_amount = tx_amount;
		}
		diff = tx_amount - last_tx_amount;
		last_tx_amount = tx_amount;
		tx_speed= FISH_MAX_SPEED * diff / max_tx_diff;
	if (tx_speed == 0) {
		/*
		 * At least, make it move a bit, cos we know there's
		 * traffic
		 */
		tx_speed = 1;
	}

	/*
	 * Do something to max rate, to do proper (hopefully)
	 * scaling
	 */
	if (max_tx_diff < diff) {
		max_tx_diff = diff;
	}
	else {
		/* Slowly lower the scale */
		if (++tx_cnt > 5) {
			max_tx_diff = diff;
			if (max_tx_diff < DIFF_MIN) {
				/* And don't scale it too low */
				max_tx_diff = DIFF_MIN;
			}
			tx_cnt = 0;
		}
	}
	}
	else {
		tx_speed = 0;
	}
}
#endif /* ENABLE_FISH */

/* ex:set sw=4 ts=4: */
