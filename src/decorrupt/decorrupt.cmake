# SPDX-License-Identifier: GPL-3.0-only

if(NOT DEFINED ${INVADER_DECORRUPT})
    set(INVADER_DECORRUPT true CACHE BOOL "Build invader-decorrupt (decorrupts cache files)")
endif()

if(${INVADER_DECORRUPT})
    add_executable(invader-decorrupt
        src/decorrupt/decorrupt.cpp
    )
    target_link_libraries(invader-decorrupt invader)

    set(TARGETS_LIST ${TARGETS_LIST} invader-decorrupt)
endif()

