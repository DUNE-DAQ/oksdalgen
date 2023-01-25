

# ######################################################################
# daq_generate_dal(sources... 
#                      NAMESPACE ns
#                      [TARGET target]
#                      [INCLUDE_DIRECTORIES ...] 
#                      [CLASSES ...] 
#                      [PACKAGE name] 
#                      [INCLUDE dir] 
#                      [CPP dir] 
#                      [CPP_OUTPUT var1] 
#                      [DUMP_OUTPUT var2])
# ######################################################################
function(daq_generate_dal)

   cmake_parse_arguments(config_opts "" "TARGET;PACKAGE;NAMESPACE;CPP;INCLUDE;CPP_OUTPUT;DUMP_OUTPUT" "INCLUDE_DIRECTORIES;CLASSES" ${ARGN})
   set(srcs ${config_opts_UNPARSED_ARGUMENTS})

   if(NOT config_opts_TARGET)
     set(config_opts_TARGET DAL_${PROJECT_NAME})
   endif()

   list(APPEND DAQ_PROJECT_GENCONFIG_INCLUDES ${CMAKE_CURRENT_BINARY_DIR}/genconfig_${config_opts_TARGET})
   set(DAQ_PROJECT_GENCONFIG_INCLUDES ${DAQ_PROJECT_GENCONFIG_INCLUDES} PARENT_SCOPE)

   if(config_opts_CLASSES)
     set(class_option -c ${config_opts_CLASSES})
   endif()

   set(package ${PROJECT_NAME})
   if(config_opts_PACKAGE)
      set(package ${config_opts_PACKAGE})
   endif()

   set(cpp_dir ${config_opts_TARGET}.tmp.cpp)
   if(config_opts_CPP)
      set(cpp_dir ${config_opts_CPP})
   endif()

   if(NOT config_opts_NAMESPACE)
      message(ERROR "NAMESPACE option is required")
   endif()

   string(REPLACE "::" "__" dump_suffix ${config_opts_NAMESPACE})
   if(config_opts_DUMP_OUTPUT)
     set(dump_srcs ${cpp_dir}/dump/dump_${dump_suffix}.cpp)
   endif()

   set(hpp_dir)
   if(config_opts_INCLUDE)
      set(hpp_dir ${config_opts_INCLUDE})
   else()
      string(REPLACE "::" "/" hpp_dir ${config_opts_NAMESPACE})
   endif()

   set(config_dependencies)

   if(DAQ_PROJECT_GENCONFIG_INCLUDES OR config_opts_INCLUDE_DIRECTORIES)
     set(config_includes -I ${DAQ_PROJECT_GENCONFIG_INCLUDES})
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

     if(config_opts_CLASSES)
       set(out)
       foreach(cand ${class_out})
         list(FIND config_opts_CLASSES ${cand} found)
         if(NOT ${found} EQUAL -1)
           set(out ${out} ${cand})
         endif()
       endforeach()
       set(class_out ${out})
     endif()

     foreach(s ${class_out})
       set(cpp_source ${cpp_source} ${cpp_dir}/${s}.cpp ${hpp_dir}/${s}.hpp)
     endforeach()

 endforeach()
   
   separate_arguments(cpp_source)

   set(GENCONFIG_DEPENDS genconfig)

   set(tmp_target MKTMP_${config_opts_TARGET})
   if(TARGET ${tmp_target})
     message(SEND_ERROR "You are using more than one daq_generate_dal() command inside this package. Please use the TARGET <name> argument to distinguish them")
   endif()

   add_custom_target(${tmp_target}
     COMMAND mkdir -p ${cpp_dir} ${cpp_dir}/dump ${hpp_dir} genconfig_${config_opts_TARGET})

   add_custom_command(
     OUTPUT genconfig_${config_opts_TARGET}/genconfig.info ${cpp_source} ${dump_srcs}
     COMMAND ${GENCONFIG_BINARY} -i ${hpp_dir} -n ${config_opts_NAMESPACE} -d ${cpp_dir} -p ${package} ${class_option} ${config_includes} -s ${schemas}
     COMMAND cp -f ${cpp_dir}/*.hpp ${hpp_dir}/
     COMMAND cp -f ${cpp_dir}/dump*.cpp ${cpp_dir}/dump
     COMMAND cp genconfig.info genconfig_${config_opts_TARGET}/
     DEPENDS ${schemas} ${config_dependencies} ${tmp_target})

   add_custom_target(${config_opts_TARGET} ALL DEPENDS ${cpp_source} )

   if(config_opts_CPP_OUTPUT)
     set(${config_opts_CPP_OUTPUT} ${cpp_source} PARENT_SCOPE)
   endif()

   if(config_opts_DUMP_OUTPUT)
     set(${config_opts_DUMP_OUTPUT} ${dump_srcs} PARENT_SCOPE)
   endif()

   if(config_opts_CPP_OUTPUT)
     install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${hpp_dir} OPTIONAL COMPONENT ${TDAQ_COMPONENT_NOARCH} DESTINATION include FILES_MATCHING PATTERN *.hpp)
   endif()

   # Always install genconfig.info files, independent of NOINSTALL option
   install(FILES ${CMAKE_CURRENT_BINARY_DIR}/genconfig_${config_opts_TARGET}/genconfig.info OPTIONAL COMPONENT ${TDAQ_COMPONENT_NOARCH} DESTINATION share/data/${PROJECT_NAME})

   set(DAQ_PROJECT_GENERATES_CODE true PARENT_SCOPE)

endfunction()

