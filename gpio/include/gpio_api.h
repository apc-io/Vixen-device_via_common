#ifndef GPIO_API_H
/* To assert that only one occurrence is included */
#define GPIO_API_H

#define WMT_GPIO_2	0x4
#define WMT_GPIO_3	0x8
#define WMT_GPIO_4	0x10
#define WMT_GPIO_5	0x20
#define WMT_GPIO_6	0x40
#define WMT_GPIO_8	0x100
#define WMT_GPIO_9	0x200
#define WMT_GPIO_10	0x400
#define WMT_GPIO_14	0x4000
#define WMT_GPIO_15	0x8000

#define FAIL	-1
#define OK		0

#define GPIO_HIGH 1
#define GPIO_LOW 0

#define DIRECTION_OUTPUT 1
#define DIRECTION_INPUT 0

extern int gpio_direction(unsigned int gpio_index, int direction);
extern int gpio_direction_input(unsigned int gpio_index);
extern int gpio_direction_output(unsigned int gpio_index, int value);
extern int gpio_request(unsigned int gpio_index);
extern int gpio_free(unsigned int gpio_index);
#endif