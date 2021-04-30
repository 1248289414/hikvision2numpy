%module hikvisionpy
%{
    #define SWIG_FILE_WITH_INIT
    #include "library.h"
%}
%include "numpy.i"
%init %{
    import_array();
%}
%apply (int* IN_ARRAY1, int DIM1) {(int *size, int n)};
%apply (unsigned char * INPLACE_ARRAY3, int DIM1, int DIM2, int DIM3) {(unsigned char *frame, int DIM1, int DIM2, int DIM3)};
%include "library.h"