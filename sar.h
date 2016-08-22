#ifndef _SAR_H
#define _SAR_H


namespace Sar {

/* Get IFNAMSIZ */
#ifndef IFNAMSIZ
#define IFNAMSIZ	16
#endif

/* Maximum length of network interface name */
const int MAX_IFACE_LEN = IFNAMSIZ;

const int MAX_CPU_NR = 128;
const int MAX_NET_DEV_NR = 16;
const int MAX_DISK_NR = 64;

const int MAX_PF_NAME = 1024;

const int NR_IFACE_PREALLOC = 2;
const int NR_DEV_PREALLOC = 4;
const int NR_DISK_PREALLOC = 3;

/* Files */
const char * const STAT = "/proc/stat";
const char * const PPARTITIONS = "/proc/partitions";
const char * const DISKSTATS = "/proc/diskstats";
const char * const INTERRUPTS = "/proc/interrupts";
const char * const SYSFS_BLOCK = "/sys/block";
const char * const SYSFS_DEVCPU = "/sys/devices/system/cpu";
const char * const S_STAT = "stat";

const char * const PROC = "/proc";
const char * const PSTAT = "stat";
const char * const MEMINFO = "/proc/meminfo";
const char * const PID_STAT = "/proc/%ld/stat";
const char * const SERIAL = "/proc/tty/driver/serial";
const char * const FDENTRY_STATE = "/proc/sys/fs/dentry-state";
const char * const FFILE_NR	= "/proc/sys/fs/file-nr";
const char * const FINODE_STATE = "/proc/sys/fs/inode-state";
const char * const FDQUOT_NR = "/proc/sys/fs/dquot-nr";
const char * const FDQUOT_MAX = "/proc/sys/fs/dquot-max";
const char * const FSUPER_NR = "/proc/sys/fs/super-nr";
const char * const FSUPER_MAX = "/proc/sys/fs/super-max";
const char * const FRTSIG_NR = "/proc/sys/kernel/rtsig-nr";
const char * const FRTSIG_MAX = "/proc/sys/kernel/rtsig-max";
const char * const NET_DEV = "/proc/net/dev";
const char * const NET_SOCKSTAT = "/proc/net/sockstat";
const char * const NET_RPC_NFS = "/proc/net/rpc/nfs";
const char * const NET_RPC_NFSD = "/proc/net/rpc/nfsd";
const char * const SADC ="sadc";
const char * const LOADAVG = "/proc/loadavg";
const char * const VMSTAT = "/proc/vmstat";



struct SarInterfaceInfo {
	/* network interface statistics */
	double rxpck;
	double txpck;
	double rxbyt;
	double txbyt;
	double rxcmp;
	double txcmp;
	double rxmcst;

	/* network interface statistics (errors) */
	double rxerr;
	double txerr;
	double coll;
	double rxdrop;
	double txdrop;
	double txcarr;
	double rxfram;
	double rxfifo;
	double txfifo;

	char interface[MAX_IFACE_LEN];
};


struct SarDiskInfo {
	double tps;
	double rd_sec;
	double wr_sec;
	double avgrq_sz;
	double avgqu_sz;
	double await;
	double svctm;
	double util;
};

struct SarInfo {
	int nr_sar_interface_info;
	SarInterfaceInfo sar_interface_info[MAX_NET_DEV_NR];

	int nr_sar_disk_info;
	SarDiskInfo sar_disk_info[MAX_DISK_NR];
};


int get_sar_info(SarInfo &sar_info);

}

#endif 	/* _SAR_H */
