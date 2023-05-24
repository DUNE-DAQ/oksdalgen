
include(DAQ)

# ######################################################################
# daq_generate_dal(sources... 
#                      NAMESPACE ns
#		       [DEP_PKGS pkg1 pkg2 ...]
# ######################################################################
function(daq_generate_dal)

   cmake_parse_arguments(config_opts "" "NAMESPACE" "DEP_PKGS" ${ARGN})

   set(srcs ${config_opts_UNPARSED_ARGUMENTS})

   set(LIB_PATH "codegen")

   set(TARGETNAME DAL_${PROJECT_NAME})

   if(TARGET ${TARGETNAME})
     message(FATAL_ERROR "You are using more than one daq_generate_dal() command inside this package; this is not allowed. Exiting...")
   endif()

   set(LIST GENCONFIG_INCLUDES ${CMAKE_CURRENT_BINARY_DIR}/genconfig_${TARGETNAME}/ )

   set(package ${PROJECT_NAME})

   set(cpp_dir ${CMAKE_CODEGEN_BINARY_DIR}/src)
   set(hpp_dir ${CMAKE_CODEGEN_BINARY_DIR}/include/${PROJECT_NAME})

   set(NAMESPACE)
   if(NOT config_opts_NAMESPACE)
      set(NAMESPACE dunedaq::${PROJECT_NAME})
   else()
      set(NAMESPACE ${config_opts_NAMESPACE})
   endif()

   set(config_dependencies)

   set(dep_paths ${CMAKE_CURRENT_SOURCE_DIR} )

   if (DEFINED config_opts_DEP_PKGS)
     foreach(dep_pkg ${config_opts_DEP_PKGS})

       list(APPEND config_dependencies DAL_${dep_pkg})

       if (EXISTS ${CMAKE_SOURCE_DIR}/${dep_pkg})
         list(APPEND dep_paths "${CMAKE_SOURCE_DIR}/${dep_pkg}")
	 list(APPEND GENCONFIG_INCLUDES ${CMAKE_CURRENT_BINARY_DIR}/../${dep_pkg}/genconfig_DAL_${dep_pkg} )
       else()      					
         if (NOT DEFINED "${dep_pkg}_DAQSHARE")
           if (NOT DEFINED "${dep_pkg}_CONFIG")
             message(FATAL_ERROR "ERROR: package ${dep_pkg} not found/imported.")
           else()
             message(FATAL_ERROR "ERROR: package ${dep_pkg} does not provide the ${dep_pkg}_DAQSHARE path variable.")
           endif()
         endif()
        
         list(APPEND dep_paths "${${dep_pkg}_DAQSHARE}")
	 list(APPEND GENCONFIG_INCLUDES "${${dep_pkg}_DAQSHARE}/genconfig_DAL_${dep_pkg}")
       endif()
     endforeach()
   endif()

   set(schemas)
   foreach(src ${srcs})
     set(schemas ${schemas} ${CMAKE_CURRENT_SOURCE_DIR}/schema/${PROJECT_NAME}/${src})
   endforeach()
   
   foreach(schema ${schemas}) 

     execute_process(
       COMMAND grep "[ \t]*<class name=\"" ${schema}
       COMMAND sed "s;[ \t]*<class name=\";;"
       COMMAND sed s:\".*::
       COMMAND tr "\\n" " "
       OUTPUT_VARIABLE class_out 
       )

     separate_arguments(class_out)

     foreach(s ${class_out})
       set(cpp_source ${cpp_source} ${cpp_dir}/${s}.cpp ${hpp_dir}/${s}.hpp)
     endforeach()

   endforeach()
   
   separate_arguments(cpp_source)

   if (NOT TARGET genconfig)
     add_custom_target( genconfig )
     add_dependencies( genconfig genconfig::genconfig)
   endif()

   set(GENCONFIG_DEPENDS genconfig)

   # Notice we need to locally-override DUNEDAQ_SHARE_PATH since this
   # variable typically refers to installed directories, but
   # installation only happens after building is complete

   string(JOIN ":" PATHS_TO_SEARCH ${dep_paths})

   add_custom_command(
     OUTPUT genconfig_${TARGETNAME}/genconfig.info ${cpp_source}
     COMMAND mkdir -p ${cpp_dir} ${hpp_dir} genconfig_${TARGETNAME}
     COMMAND ${CMAKE_COMMAND} -E env DUNEDAQ_SHARE_PATH=${PATHS_TO_SEARCH} ${GENCONFIG_BINARY} -i ${hpp_dir} -n ${NAMESPACE} -d ${cpp_dir} -p ${package}  -I ${GENCONFIG_INCLUDES} -s ${schemas}
     COMMAND cp -f ${cpp_dir}/*.hpp ${hpp_dir}/
     COMMAND cp genconfig.info genconfig_${TARGETNAME}/
     DEPENDS ${schemas} ${config_dependencies} ${GENCONFIG_DEPENDS} 
)

   add_custom_target(${TARGETNAME} ALL DEPENDS ${cpp_source} genconfig_${TARGETNAME}/genconfig.info)
   add_dependencies( ${PRE_BUILD_STAGE_DONE_TRGT} ${TARGETNAME})

   set(libname ${PROJECT_NAME}_oks)
   add_library(${libname} SHARED ${cpp_source})
   target_link_libraries(${libname} PUBLIC oksdbinterfaces::oksdbinterfaces)
   _daq_set_target_output_dirs( ${libname} ${LIB_PATH} )


   target_include_directories(${libname} PUBLIC
     $<BUILD_INTERFACE:${CMAKE_CODEGEN_BINARY_DIR}/include>
     $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
   )

   install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${hpp_dir} DESTINATION include FILES_MATCHING PATTERN *.hpp)
   install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/genconfig_${TARGETNAME} DESTINATION ${PROJECT_NAME}/share/)

   _daq_define_exportname()
  install(TARGETS ${libname} EXPORT ${DAQ_PROJECT_EXPORTNAME} )

  set(DAQ_PROJECT_INSTALLS_TARGETS true PARENT_SCOPE)
  set(DAQ_PROJECT_GENERATES_CODE true PARENT_SCOPE)

endfunction()

