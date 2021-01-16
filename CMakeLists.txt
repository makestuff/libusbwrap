project(usbwrap)

# Create a shared library
file(GLOB SOURCES src/*.cpp src/*.c)
add_library(${PROJECT_NAME} SHARED ${SOURCES})

# Ensure clients can find the includes
target_include_directories(${PROJECT_NAME} PUBLIC include)

# Dependencies
set(LIB_DEPENDS common error usb-1.0)
target_link_libraries(${PROJECT_NAME} PUBLIC ${LIB_DEPENDS})

# What to install
install(TARGETS ${PROJECT_NAME}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(
  DIRECTORY include/makestuff DESTINATION include
)

# List endpoints tool
add_subdirectory(lsep)

# Maybe build tests
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()

# On Windows we need to fetch libusb itself
if(MSVC)
  include(FetchContent)
  FetchContent_Declare(
    libusb
    GIT_REPOSITORY https://github.com/libusb/libusb.git
    GIT_TAG        v1.0.23
    GIT_SHALLOW    ON
    PATCH_COMMAND  git apply --ignore-space-change --ignore-whitespace ${CMAKE_CURRENT_SOURCE_DIR}/libusb.patch
  )
  if(NOT libusb_POPULATED)
    FetchContent_Populate(libusb)
  endif()
  add_subdirectory(${libusb_SOURCE_DIR} ${libusb_BINARY_DIR})
endif()
