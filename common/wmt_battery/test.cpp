#include "battery.h"

extern "C" {
	extern int wmt_getsyspara(const char *varname, char *varval, int *varlen);
}

void procFunc(int p){

}

int main(int argc, char * argv[]) {
	BatteryThread *tThread = new BatteryThread();
	tThread->setInterval(2);
	tThread->registerHandler(procFunc);
	tThread->run("BatteryThread", PRIORITY_DEFAULT);
	tThread->join();
	return 0;
}

