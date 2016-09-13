##
## Provide macro to generate documentation files ( html / manpage )
## from the asciidoc format
##

include(CMakeParseArguments)


find_program(A2X_EXECUTABLE 
                NAMES a2x a2x.py
                HINTS ${ASCIIDOC_ROOT} ${ASCIIDOC_ROOT}/bin)
                
find_program(ASCIIDOC_EXECUTABLE 
                NAMES asciidoc
                HINTS ${ASCIIDOC_ROOT} ${ASCIIDOC_ROOT}/bin)                
                
set(ASCIIDOC_OPTS_MANPAGE
  -d manpage
  -f manpage
  -L
)


set(ASCIIDOC_OPTS_HTML
  -b html5
)

set(ASCIIDOC_OPTS_PDF
  -d manpage
  -f pdf
  -L
)


function(generate_asciidoc)
    set(options MANPAGE HTML PDF)
    set(oneValueArgs DESTINATION SOURCE NAME)
    cmake_parse_arguments(GENERATE_ASCIIDOC "${options}" "${oneValueArgs}" "" ${ARGN} )

    if((NOT GENERATE_ASCIIDOC_DESTINATION) OR (NOT GENERATE_ASCIIDOC_SOURCE))
        message(SEND_ERROR "Generate_asciidoc requires both DESTINATION and SOURCE")
    endif()
    
    message(STATUS "Generate asciidoc doc for ${GENERATE_ASCIIDOC_SOURCE} into ${GENERATE_ASCIIDOC_DESTINATION}")    
    
    if(NOT GENERATE_ASCIIDOC_NAME)
        get_filename_component(GENERATE_ASCIIDOC_NAME ${GENERATE_ASCIIDOC_SOURCE} NAME)
    endif()

    if(GENERATE_ASCIIDOC_MANPAGE)
        message(STATUS " --> manpage")
          set(GENERATE_ASCIIDOC_${GENERATE_ASCIIDOC_NAME}_FILE_MANPAGE "${GENERATE_ASCIIDOC_DESTINATION}/${GENERATE_ASCIIDOC_NAME}.1")
          add_custom_command(OUTPUT ${GENERATE_ASCIIDOC_${GENERATE_ASCIIDOC_NAME}_FILE_MANPAGE}
            COMMAND ${A2X_EXECUTABLE} ${ASCIIDOC_OPTS_MANPAGE} -D ${GENERATE_ASCIIDOC_DESTINATION}/ ${GENERATE_ASCIIDOC_SOURCE}
            DEPENDS ${GENERATE_ASCIIDOC_SOURCE}
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
            COMMENT "Building manpage doc for ${GENERATE_ASCIIDOC_SOURCE}"
            VERBATIM)        
    endif()
    
    if(GENERATE_ASCIIDOC_PDF)
        message(STATUS " --> pdf")  
          set(GENERATE_ASCIIDOC_${GENERATE_ASCIIDOC_NAME}_FILE_PDF "${GENERATE_ASCIIDOC_DESTINATION}/${GENERATE_ASCIIDOC_NAME}.pdf")
          add_custom_command(OUTPUT ${GENERATE_ASCIIDOC_${GENERATE_ASCIIDOC_NAME}_FILE_PDF}
            COMMAND ${A2X_EXECUTABLE} ${ASCIIDOC_OPTS_PDF} -D ${GENERATE_ASCIIDOC_DESTINATION}/ ${GENERATE_ASCIIDOC_SOURCE}
            DEPENDS ${GENERATE_ASCIIDOC_SOURCE}
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
            COMMENT "Building pdf doc for ${GENERATE_ASCIIDOC_SOURCE}"
            VERBATIM)        
    endif()    
    
    if(GENERATE_ASCIIDOC_HTML)
      message(STATUS " --> html") 
      set(GENERATE_ASCIIDOC_${GENERATE_ASCIIDOC_NAME}_FILE_HTML "${GENERATE_ASCIIDOC_DESTINATION}/${GENERATE_ASCIIDOC_NAME}.html")           
      add_custom_command(OUTPUT ${GENERATE_ASCIIDOC_${GENERATE_ASCIIDOC_NAME}_FILE_HTML}
        COMMAND ${ASCIIDOC_EXECUTABLE} ${ASCIIDOC_OPTS_HTML} -o ${GENERATE_ASCIIDOC_DESTINATION}/${GENERATE_ASCIIDOC_NAME}.html ${GENERATE_ASCIIDOC_SOURCE}
        DEPENDS ${GENERATE_ASCIIDOC_SOURCE}
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        COMMENT "Building html doc for ${GENERATE_ASCIIDOC_SOURCE}"
        VERBATIM)        
    endif()    

add_custom_target(doc ALL 
                  DEPENDS  ${GENERATE_ASCIIDOC_${GENERATE_ASCIIDOC_NAME}_FILE_HTML} 
						   ${GENERATE_ASCIIDOC_${GENERATE_ASCIIDOC_NAME}_FILE_PDF}
						   ${GENERATE_ASCIIDOC_${GENERATE_ASCIIDOC_NAME}_FILE_MANPAGE})
                

endfunction()


include(FindPackageHandleStandardArgs)
find_package_handle_standard_ARGS(ASCIIDOC REQUIRED_VARS A2X_EXECUTABLE ASCIIDOC_EXECUTABLE)
mark_as_advanced(A2X_EXECUTABLE ASCIIDOC_EXECUTABLE)
