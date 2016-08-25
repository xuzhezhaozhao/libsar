#include "sar.h"

#include <cstdio>
#include <iostream>


int main(int argc, char *argv[])
{
	SarInfo si;
	get_sar_info(si);

    printf(" rxpcx txpck rxbyt txbyt rxcmp txcmp rxmcst\n ");
    printf(" %5.2f %5.2f %5.2f %5.2f %5.2f %5.2f %5.2f\n\n", 
            si.rxpck(), si.txpck(), si.rxbyt(), 
            si.txbyt(), si.rxcmp(), si.txcmp(), si.rxmcst());

    printf (" rxerr txerr coll rxdrop txdrop txcarr rxfram rxfifo txfifo\n");
    printf(" %5.2f %5.2f %5.2f %5.2f %5.2f %5.2f %5.2f %5.2f %5.2f\n\n", 
            si.rxerr(), si.txerr(), si.coll(), si.rxdrop(), si.txdrop(),
            si.txcarr(), si.rxfram(), si.rxfifo(), si.txfifo());

    printf("processes \n");
    printf(" %5.2f \n\n", si.nr_processes());


    printf("user nice system iowait steal idle\n");
    printf(" %5.2f %5.2f %5.2f %5.2f %5.2f %5.2f\n\n",
            si.cpu_user(), si.cpu_nice(), si.cpu_system(),
            si.cpu_iowait(), si.cpu_steal(), si.cpu_idle() );


    printf("pgpgin pgpgout pgfault pgmajfault\n");
    printf(" %5.2f %5.2f %5.2f %5.2f\n\n",
            si.pgpgin(), si.pgpgout(), si.pgfault(), si.pgmajfault());


    printf("pswpin pswpout\n");
    printf("%5.2f %5.2f\n\n", si.pswpin(), si.pswpout());


    printf("tps rtps wtps bread bwrtn\n");
    printf(" %5.2f %5.2f %5.2f %5.2f %5.2f\n\n",
            si.tps(), si.rtps(), si.wtps(), si.bread(), si.bwrtn());

    printf("frmpg bufpg campg\n");
    printf("%5.2f %5.2f %5.2f\n\n", si.frmpg(), si.bufpg(), si.campg());


    printf("DEV     tps rd_sec wr_sec avgrq_sz avgqu_sz await svctm util\n");
    for (int i = 0; i < si.sar_disk_info_size(); ++i) {
        const SarInfo_SarDiskInfo &s = si.sar_disk_info(i);
        printf("%s %5.2f %f %f %f %f %f %f %f\n", 
                s.dev_name().c_str(),
                s.tps(), s.rd_sec(), s.wr_sec(), s.avgrq_sz(),
                s.avgqu_sz(), s.await(), s.svctm(), s.util());
    }
    printf("\n");
	
	return 0;
}
