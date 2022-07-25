
include_directories(include)
set(xerxes_files
    src/protocol.c
    src/serialize.c 
    )
add_library(xerxes ${xerxes_files})