#ifndef _RESETMSMICE_H
#define _RESETMSMICE_H

#define RESETMSMICE_BUS_NUM_SET (1 << 1)
#define RESETMSMICE_DEV_NUM_SET (1 << 2)
#define RESETMSMICE_DAEMONIZE (1 << 3)
#define RESETMSMICE_ONLY_KNOWN_MODELS (1 << 4)
#define RESETMSMICE_PRINT_INTERFACES (1 << 5)

int resetmsmice(int busnum, int devnum, int options);
void resetmsmice_set_logger(void (*new_logger)(const char*, void*), void * data);
void puts_logger(const char* text, void* data);
const char * resetmsmice_about();

#endif
