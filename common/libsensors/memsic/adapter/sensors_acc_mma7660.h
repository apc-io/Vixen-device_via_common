#ifndef __SENSORS_ACC_MMA7660_H
#define __SENSORS_ACC_MMA7660_H

int acc_mma7660_init(void);

int acc_mma7660_open(void);
int acc_mma7660_close(int fd);

int acc_mma7660_read_data(int fd, int *data);
int acc_mma7660_get_offset(int fd, int *offset_xyz);
int acc_mma7660_set_new_offset(int fd, int *offset_xyz);
int acc_mma7660_get_sensitivity(int fd, int *sensit_xyz);
int acc_mma7660_get_install_dir(void);

#endif
