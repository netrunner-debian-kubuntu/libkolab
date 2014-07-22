find_program(SWIG swig /usr/bin/)
if(SWIG)
    set(SWIG_FOUND ON)
    message("SWIG found")
endif()

#abort if any of the requireds are missing
find_package_handle_standard_args(SWIG  DEFAULT_MSG
                                  SWIG_FOUND SWIG)