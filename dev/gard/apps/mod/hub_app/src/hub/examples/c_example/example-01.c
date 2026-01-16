/* Example C file project illustrating the use of the installed HUB package */
/* Assumption: The HUB .deb package has been installed */

#include "hub.h"

int main()
{
	hub_handle_t      hub;
	enum hub_ret_code ret;

	printf("HUB version is %s\n", hub_get_version_string());

	ret = hub_preinit("/opt/hub/config/host_config.json", "/opt/hub/config",
					  &hub);
	printf("ret for preinit = %d\n", ret);

	ret = hub_discover_gards(hub);
	printf("ret for hub_discover_gards = %d\n", ret);

	ret = hub_init(hub);
	printf("ret for hub_init = %d\n", ret);

	hub_fini(hub);

	return 0;
}