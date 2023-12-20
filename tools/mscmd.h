#ifndef __mscmd_h__
#define __mscmd_h__

struct ms_config {
  struct {
    unsigned char map1;
    unsigned char map2;
    unsigned char rapid_fire;
    unsigned char padding;
  } digital[16];
  unsigned char analog[6];
};

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
int ms_load_config(struct ms_config* config);
int ms_save_config(const struct ms_config* config);
int ms_commit_config();


#endif  /* __mscmd_h__ */
