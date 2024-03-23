add_subdirectory(tlv)
add_subdirectory(utils)

# combine
add_library(libs INTERFACE)
target_link_libraries(libs INTERFACE tlv utils)
