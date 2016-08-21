#include "sar.h"

#include <cstdio>
#include <ctime>
#include <cstring>

#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>

#include <algorithm>

namespace Sar {

const int MAX_PF_NAME = 1024;

static const int MAX_CPU_NR = 128;
static const int MAX_NET_DEV_NR = 16;
static const int MAX_DISK_NR = 32;

static const int NR_IFACE_PREALLOC = 2;
static const int NR_DEV_PREALLOC = 4;
static const int NR_DISK_PREALLOC = 3;

static const int STATS_NET_DEV_SIZE	= sizeof(StatsNetDev);

static StatsOneCpu stats_one_cpu[MAX_CPU_NR];
static StatsNetDev stats_net_dev[MAX_NET_DEV_NR];
static DiskStats disk_stats[MAX_DISK_NR];

static int g_cpu_nr;		/* number of processors on this machine */ 
static int g_disk_nr;	/* number of devices in /proc/stat */
static int g_iface_nr;	/* number of network devices (interfaces) */

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
			for (int pos = 9; pos < strlen(line) - 1; 
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
static int read_proc_stat(FileStats &file_stats)
{
	FILE *fp;
	if ((fp = fopen(STAT, "r")) == NULL) {
		/* fprintf(stderr, _("Cannot open %s: %s\n"), STAT, strerror(errno)); */
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
					st_cpu_i = stats_one_cpu + proc_nb;
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
static int read_net_dev_stat(FileStats &file_stats)
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
		if (pos < strlen(line)) {
			stats_net_dev_i = stats_net_dev + dev;
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
		memset(stats_net_dev + dev, 0, STATS_NET_DEV_SIZE * (g_iface_nr - dev));

		while (dev < g_iface_nr) {
			/*
			 * Nb of network interfaces has changed, or appending data to an
			 * old file with more interfaces than are actually available now.
			 */
			StatsNetDev *stats_net_dev_i = stats_net_dev + dev++;
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
static int read_diskstats_stat(FileStats &file_stats)
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
			DiskStats *disk_stats_i = disk_stats + dsk++;
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
		DiskStats *disk_stats_i = disk_stats + dsk++;
		disk_stats_i->major = disk_stats_i->minor = 0;
	}
	return 0;
}

/* Read stats from /proc/partitions */
/* TODO */
/* static int read_ppartitions_stat(FileStats &file_stats) */

static int read_stats(FileStats &file_stats)
{
	int ret = 0;
	ret += read_proc_stat(file_stats);
	ret += read_proc_meminfo(file_stats);
	ret += read_proc_loadavg(file_stats);
	ret += read_proc_vmstat(file_stats);
	ret += read_ktables_stat(file_stats);
	ret += read_net_sock_stat(file_stats);
	ret += read_net_nfs_stat(file_stats);
	ret += read_net_nfsd_stat(file_stats);
	ret += read_diskstats_stat(file_stats);
	ret += read_net_dev_stat(file_stats);

	if (ret < 0) {
		ret = -1;
	}



	return ret;
}


static void init()
{
	memset(stats_one_cpu, 0, sizeof(stats_one_cpu));
	memset(stats_net_dev, 0, sizeof(stats_net_dev));
	memset(disk_stats, 0, sizeof(disk_stats));

	g_cpu_nr = get_cpu_nr();
	g_disk_nr = get_diskstats_dev_nr(0, 1) + NR_DISK_PREALLOC;
	g_iface_nr = get_net_dev();
}

int get_file_stats(FileStats &file_stats) {
	struct tm rectime;
	file_stats.ust_time = get_time(rectime);
	file_stats.hour = rectime.tm_hour;
	file_stats.minute = rectime.tm_min;
	file_stats.second = rectime.tm_sec;

	init();

	return 0;
}

}
