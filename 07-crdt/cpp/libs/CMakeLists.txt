add_subdirectory(ll)
add_subdirectory(tlv)

# combine
add_library(libs INTERFACE)
target_link_libraries(libs INTERFACE tlv ll)
