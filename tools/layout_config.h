#ifndef __layout_config_h__
#define __layout_config_h__

struct layout_config {
  struct {
    unsigned char map1;
    unsigned char map2;
    unsigned char rapid_fire;
    unsigned char padding;
  } digital[16];
  unsigned char analog[6];
};

struct layout_status {
  unsigned char cyber_map[6];
  unsigned char raw_digital[2];
  unsigned char raw_analog[6];
};

#endif  /* __layout_config_h__ */