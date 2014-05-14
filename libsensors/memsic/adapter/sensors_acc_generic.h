#ifndef __SENSORS_ACC_GENERIC_H
#define __SENSORS_ACC_GENERIC_H

int acc_generic_init(void);

int acc_generic_open(void);
int acc_generic_close(int fd);

int acc_generic_read_data(int fd, int *data);
int acc_generic_get_offset(int fd, int *offset_xyz);
int acc_generic_set_new_offset(int fd, int *offset_xyz);
int acc_generic_get_sensitivity(int fd, int *sensit_xyz);
int acc_generic_get_install_dir(void);

#endif
