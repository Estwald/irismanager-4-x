#ifndef CONTROLFAN_H
#define CONTROLFAN_H

int test_controlfan_compatibility();
int load_ps3_controlfan_payload();
int load_ps3_controlfan_sm_sprx();
void set_fan_mode(int mode);
void set_device_wakeup_mode(u32 flags);
void set_usleep_sm_main(u32 us);

void load_controlfan_config();

void draw_controlfan_options();

#endif