/*  BubbleMon dockapp 1.2 - Linux specific code
 *  Copyright 2000, 2001 timecop@japan.co.jp
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
/* Huw Giddens 2003-10-27: Changed system_memory() so that it reports memory
 *     correctly on Linux 2.6 systems. Also works on 2.4 systems without
 *     recompile. Based on a patch by Marcelo E Magallon <mmagallo@debian.org>,
 *     see http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=175140
 */

#include <stdio.h>
#include <string.h>
#include "include/bubblemon.h"
#include "include/sys_include.h"

extern BubbleMonData bm;

/* returns current CPU load in percent, 0 to 100 */
int system_cpu(void)
{
    unsigned int cpuload;
    u_int64_t load, total, oload, ototal;
    u_int64_t ab, ac, ad, ae;
    int i;
    FILE *stat;

    stat = fopen("/proc/stat", "r");
    fscanf(stat, "%*s %Ld %Ld %Ld %Ld", &ab, &ac, &ad, &ae);
    fclose(stat);

    /* Find out the CPU load */
    /* user + sys = load
     * total = total */
    load = ab + ac + ad;	/* cpu.user + cpu.sys; */
    total = ab + ac + ad + ae;	/* cpu.total; */

    /* "i" is an index into a load history */
    i = bm.loadIndex;
    oload = bm.load[i];
    ototal = bm.total[i];

    bm.load[i] = load;
    bm.total[i] = total;
    bm.loadIndex = (i + 1) % bm.samples;

    /*
       Because the load returned from libgtop is a value accumulated
       over time, and not the current load, the current load percentage
       is calculated as the extra amount of work that has been performed
       since the last sample. yah, right, what the fuck does that mean?
     */
    if (ototal == 0)		/* ototal == 0 means that this is the first time
				   we get here */
	cpuload = 0;
    else if ((total - ototal) <= 0)
	cpuload = 100;
    else
	cpuload = (100 * (load - oload)) / (total - ototal);

    return cpuload;
}

int system_memory(void)
{
	u_int64_t mem_used, mem_max, swap_used, swap_max;
	u_int64_t value, mem_cache, mem_buffers, swap_cache;
	static int delay = 0;
	FILE *mem;
	char name[256];
	char line[256];

	if (delay-- <= 0) {
		mem = fopen("/proc/meminfo", "r"); 
		if (mem == NULL) return 0;
		while (!feof(mem)) {
			if (fgets(line, 256, mem) == NULL) break;
			if (sscanf(line, "%s %Ld", name, &value) != 2) continue;

			/* Before I calculate mem_used, I store MemFree in the mem_used
			 * variable. The same with swap_used/SwapFree */
			if (strcmp(name, "MemTotal:") == 0) mem_max = value;
		    else if (strcmp(name, "Cached:") == 0) mem_cache = value;
		    else if (strcmp(name, "Buffers:") == 0) mem_buffers = value;
		    else if (strcmp(name, "MemFree:") == 0) mem_used = value;
		    else if (strcmp(name, "SwapTotal:") == 0) swap_max = value;
		    else if (strcmp(name, "SwapFree:") == 0) swap_used = value;
		    else if (strcmp(name, "SwapCached:") == 0) swap_cache = value;
		}
		fclose(mem);

		mem_used = (mem_max - mem_used) + (swap_max - swap_used) -
			(mem_cache + mem_buffers + swap_cache);
		if (mem_used > mem_max) {
			swap_used = mem_used - mem_max;
			mem_used = mem_max;
		} else {
			swap_used = 0;
		}

		/* proc reports usage in kb, bm wants it in bytes. */
		bm.mem_used  = 1024 * mem_used;
		bm.mem_max   = 1024 * mem_max;
		bm.swap_used = 1024 * swap_used;
		bm.swap_max  = 1024 * swap_max;

		return 1; /* update */
	}
	return 0; /* nothing changed, don't update */
}

#ifdef ENABLE_MEMSCREEN
void system_loadavg(void)
{
    FILE *avg;
    static int avg_delay;
    if (avg_delay-- <= 0) {
	avg = fopen("/proc/loadavg", "r");
	fscanf(avg, "%d.%d %d.%d %d.%d", &bm.loadavg[0].i, &bm.loadavg[0].f,
		&bm.loadavg[1].i, &bm.loadavg[1].f,
		&bm.loadavg[2].i, &bm.loadavg[2].f);
	fclose(avg);
	avg_delay = ROLLVALUE;
    }
}
#endif				/* ENABLE_MEMSCREEN */


#ifdef ENABLE_FISH

#define NET_DEVICE		"eth0"
#define FISH_MAX_SPEED	8
#define DIFF_MIN 10

// The actual speed for the fish...
int tx_speed;
int rx_speed;

u_int64_t tx_amount;
u_int64_t rx_amount;

// Store the last one to compare
u_int64_t last_tx_amount;
u_int64_t last_rx_amount;

// Store the max for scaling, too.
u_int64_t max_tx_diff = DIFF_MIN;
u_int64_t max_rx_diff = DIFF_MIN;


// The cnt for scaling
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
	FILE *dev;
	char buffer[256];
	u_int64_t diff;

	// Have some delay in updating/sampling traffic...
	if(delay++ < 5)
	{
		return;
	}
	else
	{
		delay = 0;
	}

	if((dev = fopen("/proc/net/dev", "r")) == NULL)
	{
		// Sanity check
		fish_traffic = 0;
	}
	else
	{
		// Don't care about the first 2 lines
		fgets(buffer, 256, dev);
		fgets(buffer, 256, dev);

		while(fgets(buffer, 256, dev))
		{
			char name[256];
			
			// I love sscanf! :)
			sscanf(buffer, "%*[ ]%[^:]:%*d %Ld %*d %*d %*d %*d %*d %*d %*d %Ld %*d %*d %*d %*d %*d %*d", name, &rx_amount, &tx_amount);

			if(!strcmp(name, NET_DEVICE))
			{
				// Incoming traffic
				if(rx_amount != last_rx_amount)
				{
					if(last_rx_amount == 0)
					{
						last_rx_amount = rx_amount;
					}

					diff = rx_amount - last_rx_amount;
					last_rx_amount = rx_amount;
					rx_speed = FISH_MAX_SPEED * diff / max_rx_diff;
					if(rx_speed == 0)
					{
						// At least, make it move a bit, cos we know there's
						// traffic
						rx_speed = 1;
					}

					// Do something to max rate, to do proper (hopefully)
					// scaling
					if(max_rx_diff < diff)
					{
						max_rx_diff = diff;
					}
					else
					{
						// Slowly lower the scale
						if(++rx_cnt > 5)
						{
							max_rx_diff = diff;
							if(max_rx_diff < DIFF_MIN)
							{
								// And don't scale it too low
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


				// Outgoing traffic
				if(tx_amount != last_tx_amount)
				{
					if(last_tx_amount == 0)
					{
						last_tx_amount = tx_amount;
					}
					diff = tx_amount - last_tx_amount;
					last_tx_amount = tx_amount;
					tx_speed= FISH_MAX_SPEED * diff / max_tx_diff;
					if(tx_speed == 0)
					{
						// At least, make it move a bit, cos we know there's
						// traffic
						tx_speed = 1;
					}
					
					// Do something to max rate, to do proper (hopefully)
					// scaling
					if(max_tx_diff < diff)
					{
						max_tx_diff = diff;
					}
					else
					{
						// Slowly lower the scale
						if(++tx_cnt > 5)
						{
							max_tx_diff = diff;
							if(max_tx_diff < DIFF_MIN)
							{
								// And don't scale it too low
								max_tx_diff = DIFF_MIN;
							}
							tx_cnt = 0;
						}
					}
				}
				else
				{
					tx_speed = 0;
				}

			}

		}

	}
	fclose(dev);

}





#endif


