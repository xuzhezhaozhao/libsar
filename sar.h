#ifndef _SAR_H
#define _SAR_H

#include <net/if.h>

namespace Sar {

/* Get IFNAMSIZ */
#ifndef IFNAMSIZ
#define IFNAMSIZ	16
#endif

/* Maximum length of network interface name */
const int MAX_IFACE_LEN = IFNAMSIZ;


/* Files */
#define STAT		"/proc/stat"
#define PPARTITIONS	"/proc/partitions"
#define DISKSTATS	"/proc/diskstats"
#define INTERRUPTS	"/proc/interrupts"
#define SYSFS_BLOCK	"/sys/block"
#define SYSFS_DEVCPU	"/sys/devices/system/cpu"
#define S_STAT		"stat"

#define PROC		"/proc"
#define PSTAT		"stat"
#define MEMINFO		"/proc/meminfo"
#define PID_STAT	"/proc/%ld/stat"
#define SERIAL		"/proc/tty/driver/serial"
#define FDENTRY_STATE	"/proc/sys/fs/dentry-state"
#define FFILE_NR	"/proc/sys/fs/file-nr"
#define FINODE_STATE	"/proc/sys/fs/inode-state"
#define FDQUOT_NR	"/proc/sys/fs/dquot-nr"
#define FDQUOT_MAX	"/proc/sys/fs/dquot-max"
#define FSUPER_NR	"/proc/sys/fs/super-nr"
#define FSUPER_MAX	"/proc/sys/fs/super-max"
#define FRTSIG_NR	"/proc/sys/kernel/rtsig-nr"
#define FRTSIG_MAX	"/proc/sys/kernel/rtsig-max"
#define NET_DEV		"/proc/net/dev"
#define NET_SOCKSTAT	"/proc/net/sockstat"
#define NET_RPC_NFS	"/proc/net/rpc/nfs"
#define NET_RPC_NFSD	"/proc/net/rpc/nfsd"
#define SADC		"sadc"
#define LOADAVG		"/proc/loadavg"
#define VMSTAT		"/proc/vmstat"

/*
 * kB -> number of pages.
 * Page size depends on machine architecture (4 kB, 8 kB, 16 kB, 64 kB...)
 */
#define PG(k)	((k) >> (g_shift))

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


struct Tstamp {
	int tm_sec;
	int tm_min;
	int tm_hour;
	int use;
};

struct SarInfo {
};


int get_sar_info(SarInfo &sar_info);

}

#endif 	/* _SAR_H */
