if(EXISTS "/home/runner/work/ThemisDB/ThemisDB/build3/cmake/test_ann_index")
  if(NOT EXISTS "/home/runner/work/ThemisDB/ThemisDB/build3/cmake/test_ann_index[1]_tests.cmake" OR
     NOT "/home/runner/work/ThemisDB/ThemisDB/build3/cmake/test_ann_index[1]_tests.cmake" IS_NEWER_THAN "/home/runner/work/ThemisDB/ThemisDB/build3/cmake/test_ann_index" OR
     NOT "/home/runner/work/ThemisDB/ThemisDB/build3/cmake/test_ann_index[1]_tests.cmake" IS_NEWER_THAN "${CMAKE_CURRENT_LIST_FILE}")
    include("/usr/local/share/cmake-3.31/Modules/GoogleTestAddTests.cmake")
    gtest_discover_tests_impl(
      TEST_EXECUTABLE [==[/home/runner/work/ThemisDB/ThemisDB/build3/cmake/test_ann_index]==]
      TEST_EXECUTOR [==[]==]
      TEST_WORKING_DIR [==[/home/runner/work/ThemisDB/ThemisDB/build3/cmake]==]
      TEST_EXTRA_ARGS [==[--gtest_color=yes;CONFIGURATIONS;Release;Debug]==]
      TEST_PROPERTIES [==[]==]
      TEST_PREFIX [==[]==]
      TEST_SUFFIX [==[]==]
      TEST_FILTER [==[]==]
      NO_PRETTY_TYPES [==[FALSE]==]
      NO_PRETTY_VALUES [==[FALSE]==]
      TEST_LIST [==[test_ann_index_TESTS]==]
      CTEST_FILE [==[/home/runner/work/ThemisDB/ThemisDB/build3/cmake/test_ann_index[1]_tests.cmake]==]
      TEST_DISCOVERY_TIMEOUT [==[5]==]
      TEST_DISCOVERY_EXTRA_ARGS [==[]==]
      TEST_XML_OUTPUT_DIR [==[]==]
    )
  endif()
  include("/home/runner/work/ThemisDB/ThemisDB/build3/cmake/test_ann_index[1]_tests.cmake")
else()
  add_test(test_ann_index_NOT_BUILT test_ann_index_NOT_BUILT)
endif()
