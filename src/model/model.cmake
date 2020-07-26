# SPDX-License-Identifier: GPL-3.0-only

if(NOT DEFINED ${INVADER_MODEL})
    set(INVADER_MODEL true CACHE BOOL "Build invader-model (builds model files)")
endif()

if(${INVADER_MODEL})
    add_executable(invader-model
        src/model/model.cpp
    )
    target_link_libraries(invader-model invader)

    set(TARGETS_LIST ${TARGETS_LIST} invader-model)
endif()
