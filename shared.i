
%{
    /* This macro ensures that return vectors remain a vector also in python and are not converted to tuples */
    #define SWIG_PYTHON_EXTRA_NATIVE_CONTAINERS
    #include <kolabevent.h>
%}
%include "std_vector.i"
%import(module="kolabformat") <kolabevent.h>
namespace std {
    %template(vectorevent) vector<Kolab::Event>;
    %template(vectorevent2) vector< vector<Kolab::Event> >;
};
