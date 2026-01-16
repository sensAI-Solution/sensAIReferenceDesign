/* Example C++ file project illustrating the use of the installed HUB package */
/* Assumption: The HUB .deb package has been installed */

#include <iostream>

extern "C" {
#include "hub.h"
}

using namespace std;

int main()
{
	hub_handle_t      hub;
	enum hub_ret_code ret;

	cout << "HUB version is " << hub_get_version_string() << endl;

	char host_cfg_file[] = "/opt/hub/config/host_config.json";
	char gard_path[]     = "/opt/hub/config";

	ret                  = hub_preinit(host_cfg_file, gard_path, &hub);
	cout << "ret for hub_preinit = " << ret << endl;

	ret = hub_discover_gards(hub);
	cout << "ret for hub_discover_gards = " << ret << endl;

	ret = hub_init(hub);
	cout << "ret for hub_init = " << ret << endl;

	hub_fini(hub);

	return 0;
}