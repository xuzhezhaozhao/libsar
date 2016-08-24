#include "sar.h"

#include <cstdio>
#include <iostream>


int main(int argc, char *argv[])
{
	SarInfo si;
	get_sar_info(si);

    for (int i = 0; i < si.sar_disk_info_size(); ++i) {
        std::cout << si.sar_disk_info(i).dev_name() << std::endl;
    }
	
	return 0;
}
