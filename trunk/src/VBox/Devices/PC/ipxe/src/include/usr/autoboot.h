#ifndef _USR_AUTOBOOT_H
#define _USR_AUTOBOOT_H

/** @file
 *
 * Automatic booting
 *
 */

FILE_LICENCE ( GPL2_OR_LATER );

#include <ipxe/in.h>
struct net_device;
struct uri;
struct settings;

/** uriboot() flags */
enum uriboot_flags {
	URIBOOT_NO_SAN_DESCRIBE = 0x0001,
	URIBOOT_NO_SAN_BOOT = 0x0002,
	URIBOOT_NO_SAN_UNHOOK = 0x0004,
};

#define URIBOOT_NO_SAN ( URIBOOT_NO_SAN_DESCRIBE | \
			 URIBOOT_NO_SAN_BOOT |	   \
			 URIBOOT_NO_SAN_UNHOOK )

extern int uriboot ( struct uri *filename, struct uri *root_path, int drive,
		     unsigned int flags );
extern struct uri *
fetch_next_server_and_filename ( struct settings *settings );
extern int netboot ( struct net_device *netdev );
extern int autoboot ( void );

extern int pxe_menu_boot ( struct net_device *netdev );

#endif /* _USR_AUTOBOOT_H */
