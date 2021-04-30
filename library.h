#ifndef HIKVISION_PYTHON_LIBRARY_H
#define HIKVISION_PYTHON_LIBRARY_H

extern void init(char *ip, char *usr, char *password);

extern void getSize(int *size, int n);

extern void getFrame(unsigned char *frame, int DIM1, int DIM2, int DIM3);

extern int getHeight();

extern int getWidth();

extern void release();

#endif //HIKVISION_PYTHON_LIBRARY_H
