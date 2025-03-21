#ifndef __mscmd_h__
#define __mscmd_h__

#include "layout_config.h"

/*
 * Enter slow mode to communicate with Moonshot over U-kun/X68000Z.
 */
void ms_enter_slow_mode();

/*
 * Get Moonshot firmware version.
 * Return 0 if it succeeds. Otherwise returns non-zero value.
 *
 * arguments:
 *   major pointer to receive major version
 *   minor pointer to receive minor version
 *   patch pointer to receive patch versiaon
 */
int ms_get_version(unsigned char* major, unsigned char* minor, unsigned char* patch);

/*
 * Load / Save layout configuration data.
 * Return 0 if it succeeds. Otherwise returns non-zero value.
 */
int ms_load_config(struct layout_config* config);
int ms_save_config(const struct layout_config* config);
int ms_commit_config();


#endif  /* __mscmd_h__ */
