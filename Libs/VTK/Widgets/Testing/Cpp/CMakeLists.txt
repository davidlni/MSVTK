set(KIT VTKWidgets)

set(KIT_TEST_SRCS
  msvVTKProp3DButtonRepresentationTest1.cxx
  )

create_test_sourcelist(Tests ${KIT}CxxTests.cxx
  ${KIT_TEST_SRCS}
  )

#set(TestsToRun ${Tests})
#list(REMOVE_ITEM TestsToRun ${KIT}CxxTests.cxx)

set(LIBRARY_NAME msv${KIT})

add_executable(${KIT}CxxTests ${Tests})
target_link_libraries(${KIT}CxxTests ${LIBRARY_NAME})

#
# Add Tests
#

simple_test( msvVTKProp3DButtonRepresentationTest1 )
