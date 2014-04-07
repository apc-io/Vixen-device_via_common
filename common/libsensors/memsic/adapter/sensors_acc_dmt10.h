#ifndef __SENSORS_ACC_DMT10_H
#define __SENSORS_ACC_DMT10_H

int acc_dmt10_init(void);

int acc_dmt10_open(void);
int acc_dmt10_close(int fd);

int acc_dmt10_read_data(int fd, int *data);
int acc_dmt10_get_offset(int fd, int *offset_xyz);
int acc_dmt10_set_new_offset(int fd, int *offset_xyz);
int acc_dmt10_get_sensitivity(int fd, int *sensit_xyz);
int acc_dmt10_get_install_dir(void);

#endif
