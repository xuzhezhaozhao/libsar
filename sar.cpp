

#include "sar.h"

#include <cstdio>
#include <ctime>
#include <cstring>

#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <net/if.h>
#include <poll.h>

#include <algorithm>

namespace Sar {

struct FileStats {
	/* --- LONG LONG --- */
	/* Machine uptime (multiplied by the # of proc) */
	unsigned long long uptime			__attribute__ ((aligned (16)));
	/* Uptime reduced to one processor. Set *only* on SMP machines */
	unsigned long long uptime0			__attribute__ ((aligned (16)));
	unsigned long long context_swtch		__attribute__ ((aligned (16)));
	unsigned long long cpu_user			__attribute__ ((aligned (16)));
	unsigned long long cpu_nice			__attribute__ ((aligned (16)));
	unsigned long long cpu_system		__attribute__ ((aligned (16)));
	unsigned long long cpu_idle			__attribute__ ((aligned (16)));
	unsigned long long cpu_iowait		__attribute__ ((aligned (16)));
	unsigned long long cpu_steal			__attribute__ ((aligned (16)));
	unsigned long long irq_sum			__attribute__ ((aligned (16)));
	/* --- LONG --- */
	/* Time stamp (number of seconds since the epoch) */
	unsigned long ust_time			__attribute__ ((aligned (16)));
	unsigned long processes			__attribute__ ((aligned (8)));
	unsigned long pgpgin				__attribute__ ((aligned (8)));
	unsigned long pgpgout			__attribute__ ((aligned (8)));
	unsigned long pswpin				__attribute__ ((aligned (8)));
	unsigned long pswpout			__attribute__ ((aligned (8)));
	/* Memory stats in kB */
	unsigned long frmkb				__attribute__ ((aligned (8)));
	unsigned long bufkb				__attribute__ ((aligned (8)));
	unsigned long camkb				__attribute__ ((aligned (8)));
	unsigned long tlmkb				__attribute__ ((aligned (8)));
	unsigned long frskb				__attribute__ ((aligned (8)));
	unsigned long tlskb				__attribute__ ((aligned (8)));
	unsigned long caskb				__attribute__ ((aligned (8)));
	unsigned long nr_running			__attribute__ ((aligned (8)));
	unsigned long pgfault			__attribute__ ((aligned (8)));
	unsigned long pgmajfault			__attribute__ ((aligned (8)));
	/* --- INT --- */
	unsigned int  dk_drive			__attribute__ ((aligned (8)));
	unsigned int  dk_drive_rio			__attribute__ ((packed));
	unsigned int  dk_drive_wio			__attribute__ ((packed));
	unsigned int  dk_drive_rblk			__attribute__ ((packed));
	unsigned int  dk_drive_wblk			__attribute__ ((packed));
	unsigned int  file_used			__attribute__ ((packed));
	unsigned int  inode_used			__attribute__ ((packed));
	unsigned int  super_used			__attribute__ ((packed));
	unsigned int  super_max			__attribute__ ((packed));
	unsigned int  dquot_used			__attribute__ ((packed));
	unsigned int  dquot_max			__attribute__ ((packed));
	unsigned int  rtsig_queued			__attribute__ ((packed));
	unsigned int  rtsig_max			__attribute__ ((packed));
	unsigned int  sock_inuse			__attribute__ ((packed));
	unsigned int  tcp_inuse			__attribute__ ((packed));
	unsigned int  udp_inuse			__attribute__ ((packed));
	unsigned int  raw_inuse			__attribute__ ((packed));
	unsigned int  frag_inuse			__attribute__ ((packed));
	unsigned int  dentry_stat			__attribute__ ((packed));
	unsigned int  load_avg_1			__attribute__ ((packed));
	unsigned int  load_avg_5			__attribute__ ((packed));
	unsigned int  load_avg_15			__attribute__ ((packed));
	unsigned int  nr_threads			__attribute__ ((packed));
	unsigned int  nfs_rpccnt			__attribute__ ((packed));
	unsigned int  nfs_rpcretrans			__attribute__ ((packed));
	unsigned int  nfs_readcnt			__attribute__ ((packed));
	unsigned int  nfs_writecnt			__attribute__ ((packed));
	unsigned int  nfs_accesscnt			__attribute__ ((packed));
	unsigned int  nfs_getattcnt			__attribute__ ((packed));
	unsigned int  nfsd_rpccnt			__attribute__ ((packed));
	unsigned int  nfsd_rpcbad			__attribute__ ((packed));
	unsigned int  nfsd_netcnt			__attribute__ ((packed));
	unsigned int  nfsd_netudpcnt			__attribute__ ((packed));
	unsigned int  nfsd_nettcpcnt			__attribute__ ((packed));
	unsigned int  nfsd_rchits			__attribute__ ((packed));
	unsigned int  nfsd_rcmisses			__attribute__ ((packed));
	unsigned int  nfsd_readcnt			__attribute__ ((packed));
	unsigned int  nfsd_writecnt			__attribute__ ((packed));
	unsigned int  nfsd_accesscnt			__attribute__ ((packed));
	unsigned int  nfsd_getattcnt			__attribute__ ((packed));
	/* --- CHAR --- */
	/* Record type: R_STATS or R_DUMMY */
	/* unsigned char record_type; */
	/*
	 * Time stamp: hour, minute and second.
	 * Used to determine TRUE time (immutable, non locale dependent time).
	 */
	unsigned char hour;		/* (0-23) */
	unsigned char minute;		/* (0-59) */
	unsigned char second;		/* (0-59) */
};

struct StatsOneCpu {
	unsigned long long per_cpu_idle		__attribute__ ((aligned (16)));
	unsigned long long per_cpu_iowait		__attribute__ ((aligned (16)));
	unsigned long long per_cpu_user		__attribute__ ((aligned (16)));
	unsigned long long per_cpu_nice		__attribute__ ((aligned (16)));
	unsigned long long per_cpu_system		__attribute__ ((aligned (16)));
	unsigned long long per_cpu_steal		__attribute__ ((aligned (16)));
	unsigned long long pad			__attribute__ ((aligned (16)));
};


struct StatsNetDev {
	unsigned long rx_packets			__attribute__ ((aligned (8)));
	unsigned long tx_packets			__attribute__ ((aligned (8)));
	unsigned long rx_bytes			__attribute__ ((aligned (8)));
	unsigned long tx_bytes			__attribute__ ((aligned (8)));
	unsigned long rx_compressed			__attribute__ ((aligned (8)));
	unsigned long tx_compressed			__attribute__ ((aligned (8)));
	unsigned long multicast			__attribute__ ((aligned (8)));
	unsigned long collisions			__attribute__ ((aligned (8)));
	unsigned long rx_errors			__attribute__ ((aligned (8)));
	unsigned long tx_errors			__attribute__ ((aligned (8)));
	unsigned long rx_dropped			__attribute__ ((aligned (8)));
	unsigned long tx_dropped			__attribute__ ((aligned (8)));
	unsigned long rx_fifo_errors			__attribute__ ((aligned (8)));
	unsigned long tx_fifo_errors			__attribute__ ((aligned (8)));
	unsigned long rx_frame_errors		__attribute__ ((aligned (8)));
	unsigned long tx_carrier_errors		__attribute__ ((aligned (8)));
	char		 interface[MAX_IFACE_LEN]	__attribute__ ((aligned (8)));
};


struct DiskStats {
	unsigned long long rd_sect			__attribute__ ((aligned (16)));
	unsigned long long wr_sect			__attribute__ ((aligned (16)));
	unsigned long rd_ticks			__attribute__ ((aligned (16)));
	unsigned long wr_ticks			__attribute__ ((aligned (8)));
	unsigned long tot_ticks			__attribute__ ((aligned (8)));
	unsigned long rq_ticks			__attribute__ ((aligned (8)));
	unsigned long nr_ios				__attribute__ ((aligned (8)));
	unsigned int  major				__attribute__ ((aligned (8)));
	unsigned int  minor				__attribute__ ((packed));
};


static StatsOneCpu stats_one_cpu[2][MAX_CPU_NR];
static StatsNetDev stats_net_dev[2][MAX_NET_DEV_NR];
static DiskStats disk_stats[2][MAX_DISK_NR];

static int g_cpu_nr;		/* number of processors on this machine */ 
static int g_disk_nr;	/* number of devices in /proc/stat */
static int g_iface_nr;	/* number of network devices (interfaces) */
static int g_hz;
static int g_shift;

/*
 * kB -> number of pages.
 * Page size depends on machine architecture (4 kB, 8 kB, 16 kB, 64 kB...)
 */
#define PG(k)	((k) >> (g_shift))

/* Get page shift in kB */
static int get_kb_shift()
{
	long size;
	/* One can also use getpagesize() to get the size of a page */
	if ((size = sysconf(_SC_PAGESIZE)) == -1) {
		/* perror("sysconf"); */
		return -1;
	}

	size >>= 10;	/* Assume that a page has a minimum size of 1 kB */

	int shift = 0;
	while (size > 1) {
		shift++;
		size >>= 1;
	}

	return shift;
}

/* Get number of clock ticks per second */
static long get_HZ() 
{
	long ticks;

	if ((ticks = sysconf(_SC_CLK_TCK)) == -1) {
		/* perror("sysconf"); */
		return -1;
	}

	return ticks;
}


/* Get date and time */
static time_t get_time(struct tm &rectime)
{
	time_t timer = 0;
	time(&timer);

	struct tm *ptm = gmtime(&timer);
	rectime = (*ptm);

	return timer;
}


/* Count number of processors in /sys */
static int get_sys_cpu_nr()
{
	/* Open relevant /sys directory */
	DIR *dir;
	if ((dir = opendir(SYSFS_DEVCPU)) == NULL) {
		return 0;
	}

	/* Get current file entry */
	struct dirent *drd;
	char line[MAX_PF_NAME];
	struct stat buf;
	int proc_nr = 0;
	while ((drd = readdir(dir)) != NULL) {
		if (!strncmp(drd->d_name, "cpu", 3)) {
			sprintf(line, "%s/%s", SYSFS_DEVCPU, drd->d_name);
			if (stat(line, &buf) < 0) {
				continue;
			}
			if (S_ISDIR(buf.st_mode)) {
				proc_nr++;
			}
		}
	}

	/* Close directory */
	closedir(dir);

	return proc_nr;
}

/* Count number of processors in /proc/stat */
static int get_proc_cpu_nr()
{
	FILE *fp;
	if ((fp = fopen(STAT, "r")) == NULL) {
		/* fprintf(stderr, _("Cannot open %s: %s\n"), STAT, strerror(errno)); */
		/* exit(1); */
		return 0;
	}

	char line[16];
	int proc_nr = -1;
	while (fgets(line, 16, fp) != NULL) {
		if (strncmp(line, "cpu ", 4) && !strncmp(line, "cpu", 3)) {
			int num_proc = 0;
			if (sscanf(line + 3, "%d", &num_proc) != 1) {
				return 0;
			}
			if (num_proc > proc_nr)
				proc_nr = num_proc;
		}
	}

	fclose(fp);

	return (proc_nr + 1);
}

/*
 * Return the number of processors used on the machine:
 * 0: one proc and non SMP kernel
 * 1: one proc and SMP kernel, or SMP machine with all but one offlined CPU
 * 2: two proc...
 * Try to use /sys for that, or /proc/stat if /sys doesn't exist.
 */
static int get_cpu_nr()
{
	int cpu_nr = 0;

	if ((cpu_nr = get_sys_cpu_nr()) == 0) {
		/* /sys may be not mounted. Use /proc/stat instead */
		cpu_nr = get_proc_cpu_nr();
	}

	return cpu_nr;
}

/* Look for partitions of a given block device in /sys filesystem */
static int get_dev_part_nr(char *dev_name)
{
	char dfile[MAX_PF_NAME], line[MAX_PF_NAME];
	sprintf(dfile, "%s/%s", SYSFS_BLOCK, dev_name);

	/* Open current device directory in /sys/block */
	DIR *dir;
	if ((dir = opendir(dfile)) == NULL) {
		return 0;
	}

	/* Get current file entry */
	struct dirent *drd;
	int part = 0;
	while ((drd = readdir(dir)) != NULL) {
		if (!strcmp(drd->d_name, ".") || !strcmp(drd->d_name, "..")) {
			continue;
		}

		sprintf(line, "%s/%s/%s", dfile, drd->d_name, S_STAT);

		/* Try to guess if current entry is a directory containing a stat file */
		if (!access(line, R_OK)) {
			/* Yep... */
			part++;
		}
	}

	/* Close directory */
	closedir(dir);

	return part;
}

/*
 * Look for block devices present in /sys/ filesystem:
 * Check first that sysfs is mounted (done by trying to open /sys/block
 * directory), then find number of devices registered.
 */
static int get_sysfs_dev_nr(int display_partitions)
{
	/* Open /sys/block directory */
	DIR *dir = NULL;
	if ((dir = opendir(SYSFS_BLOCK)) == NULL) {
		/* sysfs not mounted, or perhaps this is an old kernel */
		return 0;
	}

	/* Get current file entry in /sys/block directory */
	struct dirent *drd = NULL;
	int dev = 0;
	char line[MAX_PF_NAME];
	while ((drd = readdir(dir)) != NULL) {
		if (!strcmp(drd->d_name, ".") || !strcmp(drd->d_name, "..")) {
			continue;
		}
		sprintf(line, "%s/%s/%s", SYSFS_BLOCK, drd->d_name, S_STAT);

		/* Try to guess if current entry is a directory containing a stat file */
		if (!access(line, R_OK)) {
			/* Yep... */
			dev++;

			if (display_partitions) {
				/* We also want the number of partitions for this device */
				dev += get_dev_part_nr(drd->d_name);
			}
		}
	}

	/* Close /sys/block directory */
	closedir(dir);

	return dev;
}


/*
 * Find number of devices and partitions available in /proc/diskstats.
 * Args: count_part : set to TRUE if devices _and_ partitions are to be
 *		counted.
 *       only_used_dev : when counting devices, set to TRUE if only devices
 *		with non zero stats are to be counted.
 */
static int get_diskstats_dev_nr(int count_part, int only_used_dev)
{
	FILE *fp = NULL;
	if ((fp = fopen(DISKSTATS, "r")) == NULL) {
		/* File non-existent */
		return 0;
	}

	/*
	 * Counting devices and partitions is simply a matter of counting
	 * the number of lines...
	 */
	int dev = 0;
	char line[256];
	while (fgets(line, 256, fp) != NULL) {
		if (!count_part) {
			unsigned long rd_ios, wr_ios;
			int i = sscanf(line, "%*d %*d %*s %lu %*u %*u %*u %lu",
					&rd_ios, &wr_ios);
			if (i == 1) {
				/* It was a partition and not a device */
				continue;
			}
			if (only_used_dev && !rd_ios && !wr_ios) {
				/* Unused device */
				continue;
			}
		}
		dev++;
	}

	fclose(fp);

	return dev;
}

/* TODO */
/* int get_ppartitions_dev_nr(int count_part) */

/*
 * Find number of disk entries that are registered on the
 * "disk_io:" line in /proc/stat.
 */
static int get_disk_io_nr()
{
	FILE *fp = NULL;
	if ((fp = fopen(STAT, "r")) == NULL) {
		/* fprintf(stderr, _("Cannot open %s: %s\n"), STAT, strerror(errno)); */
		/* exit(2); */
		return 0;
	}

	char line[8192];
	int dsk = 0;
	while (fgets(line, 8192, fp) != NULL) {
		if (!strncmp(line, "disk_io: ", 9)) {
			for (int pos = 9; pos < (int)strlen(line) - 1; 
					pos +=strcspn(line + pos, " ") + 1)
				dsk++;
		}
	}

	fclose(fp);

	return dsk;
}


/*
 * Find number of interfaces (network devices) that are in /proc/net/dev
 * file
 */
int get_net_dev(void)
{
	FILE *fp;
	if ((fp = fopen(NET_DEV, "r")) == NULL) {
		return 0;        /* No network device file */
	}

	char line[128];
	int dev = 0;
	while (fgets(line, 128, fp) != NULL) {
		if (strchr(line, ':')) {
			dev++;
		}
	}

	fclose(fp);

	return (dev + NR_IFACE_PREALLOC);
}

/* Read stats from /proc/stat */
static int read_proc_stat(FileStats &file_stats, int curr)
{
	FILE *fp;
	if ((fp = fopen(STAT, "r")) == NULL) {
		/* exit(2); */
		return -1;
	}

	unsigned long long cc_user, cc_nice, cc_system, cc_hardirq, cc_softirq;
	unsigned long long cc_idle, cc_iowait, cc_steal;

	char line[8192];
	while (fgets(line, 8192, fp) != NULL) {
		if (!strncmp(line, "cpu ", 4)) {
			/*
			 * Read the number of jiffies spent in the different modes
			 * (user, nice, etc.) among all proc. CPU usage is not reduced
			 * to one processor to avoid rounding problems.
			 */
			file_stats.cpu_iowait = 0;        /* For pre 2.5 kernels */
			file_stats.cpu_steal = 0;
			cc_hardirq = cc_softirq = 0;
			sscanf(line + 5, "%llu %llu %llu %llu %llu %llu %llu %llu",
					&(file_stats.cpu_user),   &(file_stats.cpu_nice),
					&(file_stats.cpu_system), &(file_stats.cpu_idle),
					&(file_stats.cpu_iowait), &cc_hardirq, &cc_softirq,
					&(file_stats.cpu_steal));

			/*
			 * Time spent in system mode also includes time spent
			 * servicing hard interrrupts and softirqs.
			 */
			file_stats.cpu_system += cc_hardirq + cc_softirq;

			/*
			 * Compute the uptime of the system in jiffies (1/100ths of a second
			 * if HZ=100).
			 * Machine uptime is multiplied by the number of processors here.
			 * Note that overflow is not so far away: ULONG_MAX is 4294967295 on
			 * 32 bit systems. Overflow happens when machine uptime is:
			 * 497 days on a monoprocessor machine,
			 * 248 days on a bi processor,
			 * 124 days on a quad processor...
			 */
			file_stats.uptime = file_stats.cpu_user + file_stats.cpu_nice +
				file_stats.cpu_system + file_stats.cpu_idle +
				file_stats.cpu_iowait + file_stats.cpu_steal;
		}
		else if (!strncmp(line, "cpu", 3)) {
			if (g_cpu_nr) {
				/*
				 * Read the number of jiffies spent in the different modes
				 * (user, nice, etc) for current proc.
				 * This is done only on SMP machines.
				 */

				cc_iowait = cc_steal = 0;        /* For pre 2.5 kernels */
				cc_hardirq = cc_softirq = 0;
				int proc_nb = 0;
				sscanf(line + 3, "%d %llu %llu %llu %llu %llu %llu %llu %llu",
						&proc_nb,
						&cc_user, &cc_nice, &cc_system, &cc_idle, &cc_iowait,
						&cc_hardirq, &cc_softirq, &cc_steal);
				cc_system += cc_hardirq + cc_softirq;

				StatsOneCpu *st_cpu_i = NULL;
				if (proc_nb < g_cpu_nr) {
					st_cpu_i = stats_one_cpu[curr] + proc_nb;
					st_cpu_i->per_cpu_user   = cc_user;
					st_cpu_i->per_cpu_nice   = cc_nice;
					st_cpu_i->per_cpu_system = cc_system;
					st_cpu_i->per_cpu_idle   = cc_idle;
					st_cpu_i->per_cpu_iowait = cc_iowait;
					st_cpu_i->per_cpu_steal  = cc_steal;
				}
				/* else additional CPUs have been dynamically registered
				 * in /proc/stat */
				if (!proc_nb) {
					/*
					 * Compute uptime reduced to one proc using proc#0.
					 * Assume that proc#0 can never be offlined.
					 */
					file_stats.uptime0 = cc_user + cc_nice + cc_system +
						cc_idle + cc_iowait + cc_steal;
				}
			}
		}
		else if (!strncmp(line, "page ", 5)) {
			/* Read number of pages the system paged in and out */
			sscanf(line + 5, "%lu %lu",
					&(file_stats.pgpgin), &(file_stats.pgpgout));
		}
		else if (!strncmp(line, "swap ", 5)) {
			/* Read number of swap pages brought in and out */
			sscanf(line + 5, "%lu %lu",
					&(file_stats.pswpin), &(file_stats.pswpout));
		}
		else if (!strncmp(line, "intr ", 5)) {
			/* Read total number of interrupts received since system boot */
			sscanf(line + 5, "%llu", &(file_stats.irq_sum));
		}
		else if (!strncmp(line, "ctxt ", 5)) {
			/* Read number of context switches */
			sscanf(line + 5, "%llu", &(file_stats.context_swtch));
		}
		else if (!strncmp(line, "processes ", 10)) {
			/* Read number of processes created since system boot */
			sscanf(line + 10, "%lu", &(file_stats.processes));
		}
	}

	fclose(fp);

	return 0;
}


/* Read stats from /proc/meminfo */
static int read_proc_meminfo(FileStats &file_stats)
{
	FILE *fp;
	if ((fp = fopen(MEMINFO, "r")) == NULL) {
		return -1;
	}

	static char line[128];
	while (fgets(line, 128, fp) != NULL) {
		if (!strncmp(line, "MemTotal:", 9)) {
			/* Read the total amount of memory in kB */
			sscanf(line + 9, "%lu", &(file_stats.tlmkb));
		}
		else if (!strncmp(line, "MemFree:", 8)) {
			/* Read the amount of free memory in kB */
			sscanf(line + 8, "%lu", &(file_stats.frmkb));
		}
		else if (!strncmp(line, "Buffers:", 8)) {
			/* Read the amount of buffered memory in kB */
			sscanf(line + 8, "%lu", &(file_stats.bufkb));
		}
		else if (!strncmp(line, "Cached:", 7)) {
			/* Read the amount of cached memory in kB */
			sscanf(line + 7, "%lu", &(file_stats.camkb));
		}
		else if (!strncmp(line, "SwapCached:", 11)) {
			/* Read the amount of cached swap in kB */
			sscanf(line + 11, "%lu", &(file_stats.caskb));
		}
		else if (!strncmp(line, "SwapTotal:", 10)) {
			/* Read the total amount of swap memory in kB */
			sscanf(line + 10, "%lu", &(file_stats.tlskb));
		}
		else if (!strncmp(line, "SwapFree:", 9)) {
			/* Read the amount of free swap memory in kB */
			sscanf(line + 9, "%lu", &(file_stats.frskb));
		}
	}

	fclose(fp);

	return 0;
}


/* Read stats from /proc/loadavg */
static int read_proc_loadavg(FileStats &file_stats)
{
	FILE *fp;
	if ((fp = fopen(LOADAVG, "r")) == NULL) {
		return -1;
	}

	int load_tmp[3];
	/* Read load averages and queue length */
	fscanf(fp, "%d.%d %d.%d %d.%d %ld/%d %*d\n",
			&(load_tmp[0]), &(file_stats.load_avg_1),
			&(load_tmp[1]), &(file_stats.load_avg_5),
			&(load_tmp[2]), &(file_stats.load_avg_15),
			&(file_stats.nr_running),
			&(file_stats.nr_threads));
	fclose(fp);

	file_stats.load_avg_1  += load_tmp[0] * 100;
	file_stats.load_avg_5  += load_tmp[1] * 100;
	file_stats.load_avg_15 += load_tmp[2] * 100;
	if (file_stats.nr_running) {
		/* Do not take current process into account */
		file_stats.nr_running--;
	}

	return 0;
}

/* Read stats from /proc/vmstat (post 2.5 kernels) */
static int read_proc_vmstat(FileStats &file_stats)
{
	FILE *fp;
	if ((fp = fopen(VMSTAT, "r")) == NULL) {
		return -1;
	}

	static char line[128];
	while (fgets(line, 128, fp) != NULL) {
		/*
		 * Some of these stats may have already been read
		 * in /proc/stat file (pre 2.5 kernels).
		 */
		if (!strncmp(line, "pgpgin", 6)) {
			/* Read number of pages the system paged in */
			sscanf(line + 6, "%lu", &(file_stats.pgpgin));
		}
		else if (!strncmp(line, "pgpgout", 7)) {
			/* Read number of pages the system paged out */
			sscanf(line + 7, "%lu", &(file_stats.pgpgout));
		}
		else if (!strncmp(line, "pswpin", 6)) {
			/* Read number of swap pages brought in */
			sscanf(line + 6, "%lu", &(file_stats.pswpin));
		}
		else if (!strncmp(line, "pswpout", 7)) {
			/* Read number of swap pages brought out */
			sscanf(line + 7, "%lu", &(file_stats.pswpout));
		}
		else if (!strncmp(line, "pgfault", 7)) {
			/* Read number of faults (major+minor) made by the system */
			sscanf(line + 7, "%lu", &(file_stats.pgfault));
		}
		else if (!strncmp(line, "pgmajfault", 10)) {
			/* Read number of faults (major only) made by the system */
			sscanf(line + 10, "%lu", &(file_stats.pgmajfault));
		}
	}

	fclose(fp);

	return 0;
}

/*
 * Read stats from /proc/sys/fs/...
 * Some files may not exist, depending on the kernel configuration.
 */
static int read_ktables_stat(FileStats &file_stats)
{
	FILE *fp;
	/* Open /proc/sys/fs/dentry-state file */
	if ((fp = fopen(FDENTRY_STATE, "r")) != NULL) {
		fscanf(fp, "%*d %u", &(file_stats.dentry_stat));
		fclose(fp);
	}

	/* Open /proc/sys/fs/file-nr file */
	if ((fp = fopen(FFILE_NR, "r")) != NULL) {
		unsigned int parm;
		fscanf(fp, "%u %u", &(file_stats.file_used), &parm);
		fclose(fp);
		/*
		 * The number of used handles is the number of allocated ones
		 * minus the number of free ones.
		 */
		file_stats.file_used -= parm;
	}

	/* Open /proc/sys/fs/inode-state file */
	if ((fp = fopen(FINODE_STATE, "r")) != NULL) {
		unsigned int parm;
		fscanf(fp, "%u %u", &(file_stats.inode_used), &parm);
		fclose(fp);
		/*
		 * The number of inuse inodes is the number of allocated ones
		 * minus the number of free ones.
		 */
		file_stats.inode_used -= parm;
	}

	/* Open /proc/sys/fs/super-max file */
	if ((fp = fopen(FSUPER_MAX, "r")) != NULL) {
		fscanf(fp, "%u\n", &(file_stats.super_max));
		fclose(fp);

		/* Open /proc/sys/fs/super-nr file */
		if ((fp = fopen(FSUPER_NR, "r")) != NULL) {
			fscanf(fp, "%u\n", &(file_stats.super_used));
			fclose(fp);
		}
	}

	/* Open /proc/sys/fs/dquot-max file */
	if ((fp = fopen(FDQUOT_MAX, "r")) != NULL) {
		fscanf(fp, "%u\n", &(file_stats.dquot_max));
		fclose(fp);

		/* Open /proc/sys/fs/dquot-nr file */
		if ((fp = fopen(FDQUOT_NR, "r")) != NULL) {
			fscanf(fp, "%u", &(file_stats.dquot_used));
			fclose(fp);
		}
	}

	/* Open /proc/sys/kernel/rtsig-max file */
	if ((fp = fopen(FRTSIG_MAX, "r")) != NULL) {
		fscanf(fp, "%u\n", &(file_stats.rtsig_max));
		fclose(fp);

		/* Open /proc/sys/kernel/rtsig-nr file */
		if ((fp = fopen(FRTSIG_NR, "r")) != NULL) {
			fscanf(fp, "%u\n", &(file_stats.rtsig_queued));
			fclose(fp);
		}
	}

	return 0;
}

/* Read stats from /proc/net/dev */
static int read_net_dev_stat(FileStats &file_stats, int curr)
{
	FILE *fp;
	if ((fp = fopen(NET_DEV, "r")) == NULL) {
		return -1;
	}

	int dev = 0;
	static char line[256];
	char iface[MAX_IFACE_LEN];
	while ((fgets(line, 256, fp) != NULL) && (dev < g_iface_nr)) {
		int pos = strcspn(line, ":");
		StatsNetDev *stats_net_dev_i = NULL;
		if (pos < (int)strlen(line)) {
			stats_net_dev_i = stats_net_dev[curr] + dev;
			strncpy(iface, line, std::min(pos, MAX_IFACE_LEN - 1));
			iface[std::min(pos, MAX_IFACE_LEN - 1)] = '\0';
			/* Skip heading spaces */
			sscanf(iface, "%s", stats_net_dev_i->interface); 
			sscanf(line + pos + 1, "%lu %lu %lu %lu %lu %lu %lu %lu %lu %lu "
					"%lu %lu %lu %lu %lu %lu",
					&(stats_net_dev_i->rx_bytes),
					&(stats_net_dev_i->rx_packets),
					&(stats_net_dev_i->rx_errors),
					&(stats_net_dev_i->rx_dropped),
					&(stats_net_dev_i->rx_fifo_errors),
					&(stats_net_dev_i->rx_frame_errors),
					&(stats_net_dev_i->rx_compressed),
					&(stats_net_dev_i->multicast),
					&(stats_net_dev_i->tx_bytes),
					&(stats_net_dev_i->tx_packets),
					&(stats_net_dev_i->tx_errors),
					&(stats_net_dev_i->tx_dropped),
					&(stats_net_dev_i->tx_fifo_errors),
					&(stats_net_dev_i->collisions),
					&(stats_net_dev_i->tx_carrier_errors),
					&(stats_net_dev_i->tx_compressed));
			dev++;
		}
	}

	fclose(fp);

	if (dev < g_iface_nr) {
		/* Reset unused structures */
		memset(stats_net_dev[curr] + dev, 0, sizeof(StatsNetDev) * (g_iface_nr - dev));

		while (dev < g_iface_nr) {
			/*
			 * Nb of network interfaces has changed, or appending data to an
			 * old file with more interfaces than are actually available now.
			 */
			StatsNetDev *stats_net_dev_i = stats_net_dev[curr] + dev++;
			strcpy(stats_net_dev_i->interface, "?");
		}
	}
	return 0;
}


/* Read stats from /proc/net/sockstat */
static int read_net_sock_stat(FileStats &file_stats)
{
	FILE *fp;
	if ((fp = fopen(NET_SOCKSTAT, "r")) == NULL) {
		return -1;
	}

	static char line[96];
	while (fgets(line, 96, fp) != NULL) {
		if (!strncmp(line, "sockets:", 8)) {
			/* Sockets */
			sscanf(line + 14, "%u", &(file_stats.sock_inuse));
		}
		else if (!strncmp(line, "TCP:", 4)) {
			/* TCP sockets */
			sscanf(line + 11, "%u", &(file_stats.tcp_inuse));
		}
		else if (!strncmp(line, "UDP:", 4)) {
			/* UDP sockets */
			sscanf(line + 11, "%u", &(file_stats.udp_inuse));
		}
		else if (!strncmp(line, "RAW:", 4)) {
			/* RAW sockets */
			sscanf(line + 11, "%u", &(file_stats.raw_inuse));
		}
		else if (!strncmp(line, "FRAG:", 5)) {
			/* FRAGments */
			sscanf(line + 12, "%u", &(file_stats.frag_inuse));
		}
	}
	fclose(fp);

	return 0;
}

/* Read stats from /proc/net/rpc/nfs */
static int read_net_nfs_stat(FileStats &file_stats)
{
	FILE *fp;
	if ((fp = fopen(NET_RPC_NFS, "r")) == NULL) {
		return -1;
	}

	static char line[256];
	while (fgets(line, 256, fp) != NULL) {
		if (!strncmp(line, "rpc", 3))
			sscanf(line + 3, "%u %u",
					&(file_stats.nfs_rpccnt), &(file_stats.nfs_rpcretrans));

		else if (!strncmp(line, "proc3", 5))
			sscanf(line + 5, "%*u %*u %u %*u %*u %u %*u %u %u",
					&(file_stats.nfs_getattcnt), &(file_stats.nfs_accesscnt),
					&(file_stats.nfs_readcnt), &(file_stats.nfs_writecnt));
	}

	fclose(fp);

	return 0;
}

/* Read stats from /proc/net/rpc/nfsd */
static int read_net_nfsd_stat(FileStats &file_stats)
{
	FILE *fp;
	if ((fp = fopen(NET_RPC_NFSD, "r")) == NULL) {
		return -1;
	}

	static char line[256];
	while (fgets(line, 256, fp) != NULL) {
		if (!strncmp(line, "rc", 2))
			sscanf(line + 2, "%u %u",
					&(file_stats.nfsd_rchits), &(file_stats.nfsd_rcmisses));

		else if (!strncmp(line, "net", 3))
			sscanf(line + 3, "%u %u %u",
					&(file_stats.nfsd_netcnt), &(file_stats.nfsd_netudpcnt),
					&(file_stats.nfsd_nettcpcnt));

		else if (!strncmp(line, "rpc", 3))
			sscanf(line + 3, "%u %u",
					&(file_stats.nfsd_rpccnt), &(file_stats.nfsd_rpcbad));

		else if (!strncmp(line, "proc3", 5))
			sscanf(line + 5, "%*u %*u %u %*u %*u %u %*u %u %u",
					&(file_stats.nfsd_getattcnt), &(file_stats.nfsd_accesscnt),
					&(file_stats.nfsd_readcnt), &(file_stats.nfsd_writecnt));
	}

	fclose(fp);

	return 0;
}

/* Init dk_drive* counters (used for sar -b) */
static void init_dk_drive_stat(FileStats &file_stats)
{
	file_stats.dk_drive = 0;
	file_stats.dk_drive_rio  = file_stats.dk_drive_wio  = 0;
	file_stats.dk_drive_rblk = file_stats.dk_drive_wblk = 0;
}

/* Read stats from /proc/diskstats */
static int read_diskstats_stat(FileStats &file_stats, int curr)
{
	FILE *fp;
	if ((fp = fopen(DISKSTATS, "r")) == NULL) {
		return 0;
	}

	init_dk_drive_stat(file_stats);

	int dsk = 0;
	static char line[256];
	while ((fgets(line, 256, fp) != NULL) && (dsk < g_disk_nr)) {
		unsigned int major, minor;
		unsigned long rd_ios, wr_ios, rd_ticks, wr_ticks;
		unsigned long tot_ticks, rq_ticks;
		unsigned long long rd_sec, wr_sec;
		if (sscanf(line, "%u %u %*s %lu %*u %llu %lu %lu %*u %llu"
					" %lu %*u %lu %lu",
					&major, &minor,
					&rd_ios, &rd_sec, &rd_ticks, &wr_ios, &wr_sec, &wr_ticks,
					&tot_ticks, &rq_ticks) == 10) {
			/* It's a device and not a partition */
			if (!rd_ios && !wr_ios) {
				/* Unused device: ignore it */
				continue;
			}
			DiskStats *disk_stats_i = disk_stats[curr] + dsk++;
			disk_stats_i->major = major;
			disk_stats_i->minor = minor;
			disk_stats_i->nr_ios = rd_ios + wr_ios;
			disk_stats_i->rd_sect = rd_sec;
			disk_stats_i->wr_sect = wr_sec;
			disk_stats_i->rd_ticks = rd_ticks;
			disk_stats_i->wr_ticks = wr_ticks;
			disk_stats_i->tot_ticks = tot_ticks;
			disk_stats_i->rq_ticks = rq_ticks;

			file_stats.dk_drive += rd_ios + wr_ios;
			file_stats.dk_drive_rio += rd_ios;
			file_stats.dk_drive_rblk += (unsigned int) rd_sec;
			file_stats.dk_drive_wio += wr_ios;
			file_stats.dk_drive_wblk += (unsigned int) wr_sec;
		}
	}

	fclose(fp);

	while (dsk < g_disk_nr) {
		/*
		 * Nb of disks has changed, or appending data to an old file
		 * with more disks than are actually available now.
		 */
		DiskStats *disk_stats_i = disk_stats[curr] + dsk++;
		disk_stats_i->major = disk_stats_i->minor = 0;
	}
	return 0;
}

/* Read stats from /proc/partitions */
/* TODO */
/* static int read_ppartitions_stat(FileStats &file_stats) */

static int read_stats(FileStats &file_stats, int curr)
{
	int ret = 0;
	ret += read_proc_stat(file_stats, curr);
	ret += read_proc_meminfo(file_stats);
	ret += read_proc_loadavg(file_stats);
	ret += read_proc_vmstat(file_stats);
	ret += read_ktables_stat(file_stats);
	ret += read_net_sock_stat(file_stats);
	ret += read_net_nfs_stat(file_stats);
	ret += read_net_nfsd_stat(file_stats);
	ret += read_diskstats_stat(file_stats, curr);
	ret += read_net_dev_stat(file_stats, curr);

	return ret;
}



/*
 * Set interval value.
 * g_itv is the interval in jiffies multiplied by the # of proc.
 * itv is the interval in jiffies.
 */
static void get_itv_value(
		const FileStats &file_stats_curr,
		const FileStats &file_stats_prev,
		int nr_proc,
		unsigned long long *itv, unsigned long long *g_itv)
{
	/* Interval value in jiffies */
	if (!file_stats_prev.uptime) {
		/*
		 * Stats from boot time to be displayed: only in this case we admit
		 * that the interval (which is unsigned long long) may be greater
		 * than 0xffffffff, else it was an overflow.
		 */
		*g_itv = file_stats_curr.uptime;
	}
	else {
		*g_itv = (file_stats_curr.uptime - file_stats_prev.uptime) & 0xffffffff;
	}

	if (!(*g_itv)) {
		/* Paranoia checking */
		*g_itv = 1;
	}

	if (nr_proc > 1) {
		if (!file_stats_prev.uptime0) {
			*itv = file_stats_curr.uptime0;
		}
		else {
			*itv = (file_stats_curr.uptime0 - file_stats_prev.uptime0) 
				& 0xffffffff;
		}

		if (!(*itv)) {
			*itv = 1;
		}
	}
	else {
		*itv = *g_itv;
	}
}


static int check_iface_reg(StatsNetDev st_net_dev[][MAX_NET_DEV_NR],
							short curr, short ref, unsigned int pos)
{
	StatsNetDev *st_net_dev_i, *st_net_dev_j;
	st_net_dev_i = st_net_dev[curr] + pos;

	int index = 0;
	while (index < g_iface_nr) {
		st_net_dev_j = st_net_dev[ref] + index;
		if (!strcmp(st_net_dev_i->interface, st_net_dev_j->interface)) {
			/*
			 * Network interface found.
			 * If a counter has decreased, then we may assume that the
			 * corresponding interface was unregistered, then registered again.
			 */
			if ((st_net_dev_i->rx_packets < st_net_dev_j->rx_packets) ||
					(st_net_dev_i->tx_packets < st_net_dev_j->tx_packets) ||
					(st_net_dev_i->rx_bytes < st_net_dev_j->rx_bytes) ||
					(st_net_dev_i->tx_bytes < st_net_dev_j->tx_bytes) ||
					(st_net_dev_i->rx_compressed < st_net_dev_j->rx_compressed) ||
					(st_net_dev_i->tx_compressed < st_net_dev_j->tx_compressed) ||
					(st_net_dev_i->multicast < st_net_dev_j->multicast) ||
					(st_net_dev_i->rx_errors < st_net_dev_j->rx_errors) ||
					(st_net_dev_i->tx_errors < st_net_dev_j->tx_errors) ||
					(st_net_dev_i->collisions < st_net_dev_j->collisions) ||
					(st_net_dev_i->rx_dropped < st_net_dev_j->rx_dropped) ||
					(st_net_dev_i->tx_dropped < st_net_dev_j->tx_dropped) ||
					(st_net_dev_i->tx_carrier_errors < st_net_dev_j->tx_carrier_errors) ||
					(st_net_dev_i->rx_frame_errors < st_net_dev_j->rx_frame_errors) ||
					(st_net_dev_i->rx_fifo_errors < st_net_dev_j->rx_fifo_errors) ||
					(st_net_dev_i->tx_fifo_errors < st_net_dev_j->tx_fifo_errors)) {

				/*
				 * Special processing for rx_bytes (_packets) and tx_bytes (_packets)
				 * counters: If the number of bytes (packets) has decreased, whereas
				 * the number of packets (bytes) has increased, then assume that the
				 * relevant counter has met an overflow condition, and that the
				 * interface was not unregistered, which is all the more plausible
				 * that the previous value for the counter was > ULONG_MAX/2.
				 * NB: the average value displayed will be wrong in this case...
				 *
				 * If such an overflow is detected, just set the flag. There is no
				 * need to handle this in a special way: the difference is still
				 * properly calculated if the result is of the same type (i.e.
				 * unsigned long) as the two values.
				 */
				bool ovfw = false;

				if ((st_net_dev_i->rx_bytes < st_net_dev_j->rx_bytes) &&
						(st_net_dev_i->rx_packets > st_net_dev_j->rx_packets) &&
						(st_net_dev_j->rx_bytes > (~0UL >> 1))) {
					ovfw = true;
				}
				if ((st_net_dev_i->tx_bytes < st_net_dev_j->tx_bytes) &&
						(st_net_dev_i->tx_packets > st_net_dev_j->tx_packets) &&
						(st_net_dev_j->tx_bytes > (~0UL >> 1))) {
					ovfw = true;
				}
				if ((st_net_dev_i->rx_packets < st_net_dev_j->rx_packets) &&
						(st_net_dev_i->rx_bytes > st_net_dev_j->rx_bytes) &&
						(st_net_dev_j->rx_packets > (~0UL >> 1))) {
					ovfw = true;
				}
				if ((st_net_dev_i->tx_packets < st_net_dev_j->tx_packets) &&
						(st_net_dev_i->tx_bytes > st_net_dev_j->tx_bytes) &&
						(st_net_dev_j->tx_packets > (~0UL >> 1))) {
					ovfw = true;
				}

				if (!ovfw) {
					/* OK: assume here that the device was actually unregistered */
					memset(st_net_dev_j, 0, sizeof(StatsNetDev));
					strcpy(st_net_dev_j->interface, st_net_dev_i->interface);
				}
			}
			return index;
		}
		index++;
	}

	/* Network interface not found: Look for the first free structure */
	for (index = 0; index < g_iface_nr; index++) {
		st_net_dev_j = st_net_dev[ref] + index;
		if (!strcmp(st_net_dev_j->interface, "?")) {
			memset(st_net_dev_j, 0, sizeof(StatsNetDev));
			strcpy(st_net_dev_j->interface, st_net_dev_i->interface);
			break;
		}
	}
	if (index >= g_iface_nr) {
		/* No free structure: Default is structure of same rank */
		index = pos;
	}

	st_net_dev_j = st_net_dev[ref] + index;
	/* Since the name is not the same, reset all the structure */
	memset(st_net_dev_j, 0, sizeof(StatsNetDev));
	strcpy(st_net_dev_j->interface, st_net_dev_i->interface);

	return  index;
}


/*
 * Disks may be registered dynamically (true in /proc/stat file).
 * This is what we try to guess here.
 */
static int check_disk_reg(DiskStats st_disk[][MAX_DISK_NR],
		   short curr, short ref, int pos)
{
	DiskStats *st_disk_i, *st_disk_j;
	int index = 0;

	st_disk_i = st_disk[curr] + pos;

	while (index < g_disk_nr) {
		st_disk_j = st_disk[ref] + index;
		if ((st_disk_i->major == st_disk_j->major) &&
				(st_disk_i->minor == st_disk_j->minor)) {
			/*
			 * Disk found.
			 * If a counter has decreased, then we may assume that the
			 * corresponding device was unregistered, then registered again.
			 * NB: AFAIK, such a device cannot be unregistered with current
			 * kernels.
			 */
			if ((st_disk_i->nr_ios < st_disk_j->nr_ios) ||
					(st_disk_i->rd_sect < st_disk_j->rd_sect) ||
					(st_disk_i->wr_sect < st_disk_j->wr_sect)) {

				memset(st_disk_j, 0, sizeof(DiskStats));
				st_disk_j->major = st_disk_i->major;
				st_disk_j->minor = st_disk_i->minor;
			}
			return index;
		}
		index++;
	}

	/* Disk not found: Look for the first free structure */
	for (index = 0; index < g_disk_nr; index++) {
		st_disk_j = st_disk[ref] + index;
		if (!(st_disk_j->major + st_disk_j->minor)) {
			memset(st_disk_j, 0, sizeof(DiskStats));
			st_disk_j->major = st_disk_i->major;
			st_disk_j->minor = st_disk_i->minor;
			break;
		}
	}
	if (index >= g_disk_nr) {
		/* No free structure found: Default is structure of same rank */
		index = pos;
	}

	st_disk_j = st_disk[ref] + index;
	/* Since the device is not the same, reset all the structure */
	memset(st_disk_j, 0, sizeof(DiskStats));
	st_disk_j->major = st_disk_i->major;
	st_disk_j->minor = st_disk_i->minor;

	return index;
}


static int init()
{
	memset(stats_one_cpu, 0, sizeof(stats_one_cpu));
	memset(stats_net_dev, 0, sizeof(stats_net_dev));
	memset(disk_stats, 0, sizeof(disk_stats));

	g_cpu_nr = get_cpu_nr();
	g_disk_nr = get_diskstats_dev_nr(0, 1) + NR_DISK_PREALLOC;
	g_iface_nr = get_net_dev();
	g_hz = get_HZ();
	g_shift = get_kb_shift();

	if (g_cpu_nr > MAX_CPU_NR || g_disk_nr > MAX_DISK_NR || 
		g_iface_nr > MAX_IFACE_LEN || g_hz <= 0 || g_shift < 0) {
		return -1;
	}
	return 0;
}

template <typename T, typename U, typename Q>
static double s_value(T m, U n, Q p)
{
	return (((double) ((n) - (m))) / (p) * g_hz);
}

template <typename T, typename U, typename Q>
static double sp_value(T m, U n, Q p)
{
	return (((double) ((n) - (m))) / (p) * 100);
}

/*
 * Handle overflow conditions properly for counters which are read as
 * unsigned long long, but which can be unsigned long long or
 * unsigned long only depending on the kernel version used.
 * @value1 and @value2 being two values successively read for this
 * counter, if @value2 < @value1 and @value1 <= 0xffffffff, then we can
 * assume that the counter's type was unsigned long and has overflown, and
 * so the difference @value2 - @value1 must be casted to this type.
 * NOTE: These functions should no longer be necessary to handle a particular
 * stat counter when we can assume that everybody is using a recent kernel
 * (defining this counter as unsigned long long).
 */
static double ll_sp_value(unsigned long long value1, unsigned long long value2,
		unsigned long long itv)
{
	if ((value2 < value1) && (value1 <= 0xffffffff)) {
		/* Counter's type was unsigned long and has overflown */
		return ((double) ((value2 - value1) & 0xffffffff)) / itv * 100;
	}
	else {
		return sp_value(value1, value2, itv);
	}
}
static double ll_s_value(unsigned long long value1, unsigned long long value2,
		unsigned long long itv)
{
	if ((value2 < value1) && (value1 <= 0xffffffff)) {
		/* Counter's type was unsigned long and has overflown */
		return ((double) ((value2 - value1) & 0xffffffff)) / itv * g_hz;
	}
	else {
		return s_value(value1, value2, itv);
	}
}

int get_sar_info(SarInfo &sar_info) {
	if (init() < 0) {
		/* fatal error */
		return -1;
	}

	int ret = 0;
	FileStats file_stats[2];
	memset(file_stats, 0, sizeof(file_stats));

	int curr = 1, prev = 0;

	ret += read_stats(file_stats[prev], prev);
	poll(NULL, 0, 500);
	ret += read_stats(file_stats[curr], curr);

	/* all data is collected */
	const FileStats &f_prev = file_stats[prev];
	const FileStats &f_curr = file_stats[curr];
	unsigned long long itv = 0, g_itv = 0;

	get_itv_value(f_curr, f_prev, g_cpu_nr, &itv, &g_itv);
	if (itv == 0 || g_itv == 0) {
		return -1;
	}

	/* number of context switches per second */
	//double nr_processes = ll_s_value(f_prev.context_swtch, 
			//f_curr.context_swtch, itv);


	/* CPU usage */
	//double cpu_user = ll_sp_value(f_prev.cpu_user, f_curr.cpu_user, g_itv);
	//double cpu_nice = ll_sp_value(f_prev.cpu_nice, f_curr.cpu_nice, g_itv);
	//double cpu_system =	ll_sp_value(f_prev.cpu_system, f_curr.cpu_system, g_itv);
	//double cpu_iowait = ll_sp_value(f_prev.cpu_iowait, f_curr.cpu_iowait, g_itv);
	//double cpu_steal =	ll_sp_value(f_prev.cpu_steal, f_curr.cpu_steal, g_itv);
	//double cpu_idle = f_curr.cpu_idle < f_prev.cpu_idle ?  0.0 :
					//ll_sp_value(f_prev.cpu_idle, f_curr.cpu_idle, g_itv);

	/* paging statistics */
	//double pgpgin = s_value(f_prev.pgpgin, f_curr.pgpgin, itv);
	//double pgpgout = s_value(f_prev.pgpgout, f_curr.pgpgout, itv);
	//double pgfault = s_value(f_prev.pgfault, f_curr.pgfault, itv);
	//double pgmajfault =s_value(f_prev.pgmajfault, f_curr.pgmajfault, itv);

	/* number of swap pages brought in and out */
	//double pswpin = s_value(f_prev.pswpin, f_curr.pswpin, itv);
	//double pswpout = s_value(f_prev.pswpout, f_curr.pswpout, itv);

	/* I/O stats (no distinction made between disks) */
	//double tps = s_value(f_prev.dk_drive, f_curr.dk_drive, itv);
	//double rtps = s_value(f_prev.dk_drive_rio, f_curr.dk_drive_rio, itv);
	//double wtps = s_value(f_prev.dk_drive_wio, f_curr.dk_drive_wio, itv);
	//double bread = s_value(f_prev.dk_drive_rblk, f_curr.dk_drive_rblk, itv);
	//double bwrtn = s_value(f_prev.dk_drive_wblk, f_curr.dk_drive_wblk, itv);

	/* memory stats */
	//double frmpg = s_value((double) PG(f_prev.frmkb), (double) PG(f_curr.frmkb), itv);
	//double bufpg = s_value((double) PG(f_prev.bufkb), (double) PG(f_curr.bufkb), itv);
	//double campg = s_value((double) PG(f_prev.camkb), (double) PG(f_curr.camkb), itv);

	/* network interface statistics */
	StatsNetDev *sndi = stats_net_dev[curr], *sndj;
	sar_info.nr_sar_interface_info = 0;
	for (int i = 0; i < g_iface_nr; ++i, ++sndi) {
		if (!strcmp(sndi->interface, "?")) {
			continue;
		}
		int j = check_iface_reg(stats_net_dev, curr, prev, i);
		sndj = stats_net_dev[prev] + j;
		int &nr_sar_interface_info = sar_info.nr_sar_interface_info;
		SarInterfaceInfo &sar_interface_info = sar_info.sar_interface_info[nr_sar_interface_info];
		strncpy(sar_interface_info.interface,
		sndi->interface, MAX_IFACE_LEN-1);
		sar_interface_info.interface[MAX_IFACE_LEN-1] = '\0';
		sar_interface_info.rxpck = s_value(sndj->rx_packets, sndi->rx_packets, itv);
		sar_interface_info.txpck = s_value(sndj->tx_packets, sndi->tx_packets, itv);
		sar_interface_info.rxbyt = s_value(sndj->rx_bytes, sndi->rx_bytes, itv);
		sar_interface_info.txbyt = s_value(sndj->tx_bytes, sndi->tx_bytes, itv);
		sar_interface_info.rxcmp = s_value(sndj->rx_compressed, sndi->rx_compressed, itv);
		sar_interface_info.txcmp = s_value(sndj->tx_compressed, sndi->tx_compressed, itv);
		sar_interface_info.rxmcst = s_value(sndj->multicast, sndi->multicast, itv);

		/* network interface statistics (errors) */
		sar_interface_info.rxerr = s_value(sndj->rx_errors, sndi->rx_errors, itv);
		sar_interface_info.txerr = s_value(sndj->tx_errors, sndi->tx_errors, itv);
		sar_interface_info.coll = s_value(sndj->collisions, sndi->collisions, itv);
		sar_interface_info.rxdrop = s_value(sndj->rx_dropped, sndi->rx_dropped, itv);
		sar_interface_info.txdrop = s_value(sndj->tx_dropped, sndi->tx_dropped, itv);
		sar_interface_info.txcarr = s_value(sndj->tx_carrier_errors, sndi->tx_carrier_errors, itv);
		sar_interface_info.rxfram = s_value(sndj->rx_frame_errors, sndi->rx_frame_errors, itv);
		sar_interface_info.rxfifo = s_value(sndj->rx_fifo_errors, sndi->rx_fifo_errors, itv);
		sar_interface_info.txfifo = s_value(sndj->tx_fifo_errors, sndi->tx_fifo_errors, itv);

		++nr_sar_interface_info;
	}

	/* disk statistics */
	DiskStats *sdi = disk_stats[curr], *sdj;
	sar_info.nr_sar_disk_info = 0;
	for (int i = 0; i < g_disk_nr; i++, ++sdi) {
		double tput, util, await, svctm, arqsz;
		if (!(sdi->major + sdi->minor)) {
			continue;
		}
		int j = check_disk_reg(disk_stats, curr, prev, i);

		sdj = disk_stats[prev] + j;

		tput = ((double) (sdi->nr_ios - sdj->nr_ios)) * g_hz / itv;
		util = s_value(sdj->tot_ticks, sdi->tot_ticks, itv);
		svctm = tput ? util / tput : 0.0;
		await = (sdi->nr_ios - sdj->nr_ios) ?
			((sdi->rd_ticks - sdj->rd_ticks) + (sdi->wr_ticks - sdj->wr_ticks)) /
			((double) (sdi->nr_ios - sdj->nr_ios)) : 0.0;
		arqsz  = (sdi->nr_ios - sdj->nr_ios) ?
			((sdi->rd_sect - sdj->rd_sect) + (sdi->wr_sect - sdj->wr_sect)) /
			((double) (sdi->nr_ios - sdj->nr_ios)) : 0.0;

		int &nr_sar_disk_info = sar_info.nr_sar_disk_info;
		SarDiskInfo &sar_disk_info = sar_info.sar_disk_info[nr_sar_disk_info];

		sar_disk_info.tps = s_value(sdj->nr_ios, sdi->nr_ios,  itv);
		sar_disk_info.rd_sec = ll_s_value(sdj->rd_sect, sdi->rd_sect, itv);
		sar_disk_info.wr_sec = ll_s_value(sdj->wr_sect, sdi->wr_sect, itv);
		/* See iostat for explanations */
		sar_disk_info.avgrq_sz = arqsz;
		sar_disk_info.avgqu_sz = s_value(sdj->rq_ticks, sdi->rq_ticks, itv) / 1000.0;
		sar_disk_info.await = await;
		sar_disk_info.svctm = svctm;
		sar_disk_info.util = util / 10.0;

		++nr_sar_disk_info;
	}

	return ret;
}

}

//#ifdef SAR_UNIT_TEST
int main(int argc, char *argv[])
{
	Sar::SarInfo si;

	int ret;
	if ((ret = Sar::get_sar_info(si)) < 0) {
		//printf("error: %d\n", ret);
	}

	for (int i = 0; i < si.nr_sar_interface_info; ++i) {
		printf("%s: ", si.sar_interface_info[i].interface);
		printf("%f %f %f %f\n\n",
		si.sar_interface_info[i].rxpck,
		si.sar_interface_info[i].txpck,
		si.sar_interface_info[i].rxbyt,
		si.sar_interface_info[i].txbyt);
	}

	
	return 0;
}
//#endif


#undef PG

