#ifndef __SENSORS_ACC_MC3230_H
#define __SENSORS_ACC_MC3230_H

int acc_mc3230_init(void);

int acc_mc3230_open(void);
int acc_mc3230_close(int fd);

int acc_mc3230_read_data(int fd, int *data);
int acc_mc3230_get_offset(int fd, int *offset_xyz);
int acc_mc3230_set_new_offset(int fd, int *offset_xyz);
int acc_mc3230_get_sensitivity(int fd, int *sensit_xyz);
int acc_mc3230_get_install_dir(void);

#endif
