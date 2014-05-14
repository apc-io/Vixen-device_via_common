#include "battery.h"

int main(int argc, char * argv[]) {
	BatteryServer *server = new BatteryServer();
	server->setInterval(4);
	server->run("BatteryServer", PRIORITY_DEFAULT);
	server->join();
	return 0;
}

