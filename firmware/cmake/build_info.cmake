# Writes OUT with the build time; run at every build by the build_info target.
string(TIMESTAMP now "%s" UTC)
file(WRITE "${OUT}.tmp" "// Generated at build time by cmake/build_info.cmake.\n#include \"build_info.h\"\n\nconst uint32_t fw_build_time = ${now}u;\n")
execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different "${OUT}.tmp" "${OUT}")
