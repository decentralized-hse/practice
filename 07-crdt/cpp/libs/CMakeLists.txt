add_subdirectory(ll)
add_subdirectory(tlv)
add_subdirectory(cmn)

# combine
add_library(libs INTERFACE)
target_link_libraries(libs INTERFACE ll tlv cmn)
