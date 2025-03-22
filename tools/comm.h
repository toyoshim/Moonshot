#ifndef __comm_h__
#define __comm_h__

#include "layout_config.h"

enum {
  COMM_MODE_MOONSHOT = 0,
  COMM_MODE_MOONSHOT_SLOW = 1,
  COMM_MODE_JOYPAD_MAPPER_Z = 2,

  COMM_SAVE_ONLY = 0,
  COMM_SAVE_AND_COMMIT = 1,
};

/*
 * Connect to Moonshot device.
 * Return 0 if it succeeds. Otherwise returns non-zero value.
 *
 * arguments:
 *   mode pointer to receive communication mode
 *   major pointer to receive major version
 *   minor pointer to receive minor version
 *   patch pointer to receive patch version
 */
int comm_connect(unsigned char* mode,
                 unsigned char* major,
                 unsigned char* minor,
                 unsigned char* patch);

/**
 * Load / Save layout configuration data.
 * Return 0 if it succeeds. Otherwise returns non-zero value.
 *
 * arguments:
 *   mode communication mode
 *   transaction COMM_SAVE_ONLY or COMM_SAVE_AND_COMMIT
 *   config pointer to receive configuration data
 */
int comm_load_config(unsigned char mode, struct layout_config* config);
int comm_save_config(unsigned char mode,
                     unsigned char transaction,
                     const struct layout_config* config);

/**
 * Get input status.
 * Return 0 if it succeeds. Otherwise returns non-zero value.
 *
 * arguments
 *   mode communication mode
 *   status pointer to receive status data
 */
int comm_get_status(unsigned char mode, struct layout_status* status);

/**
 * Get / Set current running layout mode on Joypad Mapper Z.
 * Return 0 on Moonshot. Otherwise returns the layout mode.
 */
unsigned char comm_get_layout_mode(unsigned char mode);
void comm_set_layout_mode(unsigned char mode, unsigned char layout_mode);

#endif /* __comm_h__ */