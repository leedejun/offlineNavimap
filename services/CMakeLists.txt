# This subproject implements integration tests.
# This tests are launched on the whole world dataset.

# It is recommended to place tests here in the following cases:
# - tests are written to be launch on the whole world dataset;
# - tests covers significant number of subsystems;

project(services_tool)


# include_directories(
#         # ${OMIM_ROOT}/3party/freetype/include
#         # ${OMIM_ROOT}/3party/glm
#         # ${OMIM_ROOT}/3party/jansson/src
#         # ${OMIM_ROOT}/3party/protobuf/protobuf/src
#         # ${OMIM_ROOT}/3party/Vulkan-Headers/include
#         # ${OMIM_ROOT}/3party/vulkan_wrapper
#         ${OMIM_ROOT}/3party/liboauthcpp
# )

set(
  SRC
  routing_wrapper.cpp
  Config.cpp
  main.cpp
)

omim_add_executable(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  map
  tracking
  drape_frontend
  shaders
  routing
  search
  ge0
  storage
  mwm_diff
  traffic
  routing_common
  transit
  descriptions
  #ugc
  drape
  #partners_api
  #web_api
  #local_ads
  # platform_tests_support
  kml
  editor
  indexer
  metrics
  platform
  oauthcpp
  geometry
  coding
  base
  gflags
  jansson
  protobuf
  bsdiff
  succinct
  #stats_client
  minizip
  pugixml
  opening_hours
  expat
  freetype
  icu
  agg
  search_tests_support
  #sdf_image
  #stb_image
  #vulkan_wrapper
  #${CMAKE_DL_LIBS}
  ${LIBZ}
)

if (PLATFORM_LINUX)
  omim_link_libraries(
    ${PROJECT_NAME}
    dl
  )
endif()

#link_opengl(${PROJECT_NAME})
# link_qt5_core(${PROJECT_NAME})
# link_qt5_network(${PROJECT_NAME})
# link_qt5_core(${PROJECT_NAME})
# link_qt5_network(${PROJECT_NAME})
