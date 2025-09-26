configure_file(config.h.in "config.h")

set(SRCS
    "RsOprf.cpp"
    "RsPsi.cpp"
    "SimpleIndex.cpp"
    "fileBased.cpp"
    "LPsi.cpp"
)

if(VOLE_PSI_ENABLE_GMW)
    list(APPEND SRCS
        "GMW/Circuit.cpp"
        "GMW/Gmw.cpp"
        "GMW/SilentTripleGen.cpp"
    )
endif()

if(VOLE_PSI_ENABLE_CPSI)
    list(APPEND SRCS "RsCpsi.cpp")
endif()

if(VOLE_PSI_ENABLE_OPPRF)
    list(APPEND SRCS "RsOpprf.cpp")
endif()

add_library(volePSI STATIC ${SRCS})

target_link_libraries(volePSI
    PUBLIC oc::libOTe sparsehash libdivide relic
    PRIVATE gmp
)

target_include_directories(volePSI PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
)

# 컴파일 옵션들
if(APPLE)
    target_compile_options(volePSI PRIVATE
        "-std=c++${VOLE_PSI_STD_VER}"
        "-fcoroutines"
        "-Wno-pessimizing-move"
        "-Wno-delete-non-abstract-non-virtual-dtor"
    )
elseif(MSVC)
    set_target_properties(volePSI PROPERTIES
        CXX_STANDARD ${VOLE_PSI_STD_VER}
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
    )
    target_compile_definitions(volePSI PUBLIC "_ENABLE_EXTENDED_ALIGNED_STORAGE")
    if(VOLE_PSI_ENABLE_ASAN)
        message(WARNING "volepsi has ASAN enabled")
        target_compile_options(volePSI PUBLIC "/fsanitize=address")
        target_link_options(volePSI PUBLIC "/fsanitize=address")
    endif()
else()
    target_compile_options(volePSI PUBLIC 
        "-std=c++${VOLE_PSI_STD_VER}"
        "-fcoroutines"
    )
    if(VOLE_PSI_ENABLE_SSE)
        target_compile_options(volePSI PUBLIC -mavx)
    endif()
    if(VOLE_PSI_ENABLE_PIC)
        target_compile_options(volePSI PUBLIC "-fPIC")
    endif()
    if(VOLE_PSI_ENABLE_ASAN)
        message(WARNING "volepsi has ASAN enabled")
        target_compile_options(volePSI PUBLIC "-fsanitize=address")
        target_link_options(volePSI PUBLIC "-fsanitize=address")
    endif()
endif()