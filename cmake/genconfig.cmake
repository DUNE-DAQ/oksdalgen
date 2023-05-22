
include(DAQ)

# ######################################################################
# daq_generate_dal(sources... 
#                      NAMESPACE ns
#                      [INCLUDE_DIRECTORIES ...] 
#		       [DEP_PKGS pkg1 pkg2 ...]
#                      [DUMP_OUTPUT var2])
# ######################################################################
function(daq_generate_dal)

   cmake_parse_arguments(config_opts "" "NAMESPACE;DUMP_OUTPUT" "DEP_PKGS;INCLUDE_DIRECTORIES" ${ARGN})
   set(srcs ${config_opts_UNPARSED_ARGUMENTS})

   set(LIB_PATH "codegen")

   set(TARGETNAME DAL_${PROJECT_NAME})

   list(APPEND DAQ_PROJECT_GENCONFIG_INCLUDES ${CMAKE_CURRENT_BINARY_DIR}/genconfig_${TARGETNAME})
   set(DAQ_PROJECT_GENCONFIG_INCLUDES ${DAQ_PROJECT_GENCONFIG_INCLUDES} PARENT_SCOPE)

   set(package ${PROJECT_NAME})

   set(cpp_dir ${CMAKE_CODEGEN_BINARY_DIR}/src)

   if(NOT config_opts_NAMESPACE)
      message(ERROR "NAMESPACE option is required")
   endif()

   string(REPLACE "::" "__" dump_suffix ${config_opts_NAMESPACE})
   if(config_opts_DUMP_OUTPUT)
     set(dump_srcs ${cpp_dir}/dump/dump_${dump_suffix}.cpp)
   endif()

   set(hpp_dir ${CMAKE_CODEGEN_BINARY_DIR}/include/${PROJECT_NAME})

   set(dep_paths ${CMAKE_CURRENT_SOURCE_DIR} )
   if (DEFINED config_opts_DEP_PKGS)
     foreach(dep_pkg ${config_opts_DEP_PKGS})

       if (EXISTS ${CMAKE_SOURCE_DIR}/${dep_pkg})
         list(APPEND dep_paths "${CMAKE_SOURCE_DIR}/${dep_pkg}")
       else()      					
         if (NOT DEFINED "${dep_pkg}_DAQSHARE")
           if (NOT DEFINED "${dep_pkg}_CONFIG")
             message(FATAL_ERROR "ERROR: package ${dep_pkg} not found/imported.")
           else()
             message(FATAL_ERROR "ERROR: package ${dep_pkg} does not provide the ${dep_pkg}_DAQSHARE path variable.")
           endif()
         endif()
        
         list(APPEND dep_paths "${${dep_pkg}_DAQSHARE}")
       endif()
     endforeach()
   endif()

   set(config_dependencies)

   if(DAQ_PROJECT_GENCONFIG_INCLUDES OR config_opts_INCLUDE_DIRECTORIES)
     set(config_includes -I ${DAQ_PROJECT_GENCONFIG_INCLUDES})
     foreach(inc ${config_opts_INCLUDE_DIRECTORIES})
       list(APPEND config_includes ${inc})
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

   set(tmp_target MKTMP_${TARGETNAME})
   if(TARGET ${tmp_target})
     message(SEND_ERROR "You are using more than one daq_generate_dal() command inside this package. Please use the TARGET <name> argument to distinguish them")
   endif()

   add_custom_target(${tmp_target}
     COMMAND mkdir -p ${cpp_dir} ${cpp_dir}/dump ${hpp_dir} genconfig_${TARGETNAME})

   string(JOIN ":" PATHS_TO_SEARCH ${dep_paths})

   add_custom_command(
     OUTPUT genconfig_${TARGETNAME}/genconfig.info ${cpp_source} ${dump_srcs}
     COMMAND ${CMAKE_COMMAND} -E env TDAQ_DB_PATH=${PATHS_TO_SEARCH} ${GENCONFIG_BINARY} -i ${hpp_dir} -n ${config_opts_NAMESPACE} -d ${cpp_dir} -p ${package} ${config_includes} -s ${schemas}
     COMMAND cp -f ${cpp_dir}/*.hpp ${hpp_dir}/
     COMMAND cp -f ${cpp_dir}/dump*.cpp ${cpp_dir}/dump
     COMMAND cp genconfig.info genconfig_${TARGETNAME}/
     DEPENDS ${schemas} ${config_dependencies} ${tmp_target} ${GENCONFIG_DEPENDS})

   add_custom_target(${TARGETNAME} ALL DEPENDS ${cpp_source} )

   set(libname ${PROJECT_NAME}_oks)
   add_library(${libname} SHARED ${cpp_source})
   target_link_libraries(${libname} PUBLIC oksdbinterfaces::oksdbinterfaces)
   _daq_set_target_output_dirs( ${libname} ${LIB_PATH} )


   target_include_directories(${libname} PUBLIC
     $<BUILD_INTERFACE:${CMAKE_CODEGEN_BINARY_DIR}/include>
     $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
   )

   if(config_opts_DUMP_OUTPUT)
     set(${config_opts_DUMP_OUTPUT} ${dump_srcs} PARENT_SCOPE)
   endif()

   install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${hpp_dir} OPTIONAL COMPONENT ${TDAQ_COMPONENT_NOARCH} DESTINATION include FILES_MATCHING PATTERN *.hpp)

   # Always install genconfig.info files, independent of NOINSTALL option
   install(FILES ${CMAKE_CURRENT_BINARY_DIR}/genconfig_${TARGETNAME}/genconfig.info OPTIONAL COMPONENT ${TDAQ_COMPONENT_NOARCH} DESTINATION share/data/${PROJECT_NAME})

   _daq_define_exportname()
  install(TARGETS ${libname} EXPORT ${DAQ_PROJECT_EXPORTNAME} )

  set(DAQ_PROJECT_INSTALLS_TARGETS true PARENT_SCOPE)
  set(DAQ_PROJECT_GENERATES_CODE true PARENT_SCOPE)

endfunction()

