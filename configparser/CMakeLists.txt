set(example_source 
    example.cpp)

# Find the library 
find_library(configparser
    NAME configparser
    HINTS [${LIBDIR} ${INCLUDEDIR}]
)

add_executable(example ${example_source})

# and link to executable defined in example.cpp
target_link_libraries(example configparser)

