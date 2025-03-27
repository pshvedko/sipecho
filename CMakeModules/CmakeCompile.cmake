#
# @brief Макро для сборки бинарников и библиотек  
#
 
macro(CMAKE_COMPILE _type _name)

    message(STATUS "Configuring ${_type} ${_name}")
    
    # разбор списока параметров
    #
    foreach(${_name}_arg ${ARGN})
        if( ${_name}_arg STREQUAL "SOURCES" OR 
            ${_name}_arg STREQUAL "LIBRARIES" OR
            ${_name}_arg STREQUAL "VERSION" OR
            ${_name}_arg STREQUAL "DEFINES" OR
            ${_name}_arg STREQUAL "INSTALL" OR
            ${_name}_arg STREQUAL "CONFIGS" OR
            ${_name}_arg STREQUAL "FLAGS")
            set(${_name}_mode ${${_name}_arg})
        else()
            if(${_name}_mode STREQUAL "SOURCES")
                list(APPEND ${_name}_sources ${${_name}_arg})
            elseif(${_name}_mode STREQUAL "LIBRARIES")
                list(APPEND ${_name}_libraries ${${_name}_arg})
            elseif(${_name}_mode STREQUAL "VERSION")
                set( ${_name}_version ${${_name}_arg})
            elseif(${_name}_mode STREQUAL "INSTALL")
                set( ${_name}_install ${${_name}_arg})
            elseif(${_name}_mode STREQUAL "DEFINES")
                list(APPEND ${_name}_defines ${${_name}_arg})
            elseif(${_name}_mode STREQUAL "CONFIGS")
                list(APPEND ${_name}_configs ${${_name}_arg})
            elseif(${_name}_mode STREQUAL "FLAGS")
                list(APPEND ${_name}_flags ${${_name}_arg})
            endif()
        endif()
    endforeach()

    # ключи компиляции
    #
    foreach(${_name}_define ${${_name}_defines})
        add_definitions(-D${${_name}_define})
    endforeach()

    foreach(${_name}_flag ${${_name}_flags})
        add_definitions(-${${_name}_flag})
    endforeach()
    
    # сборка бинарника
    #
    if(${_type} STREQUAL "PROGRAM") 
        add_executable(${_name} ${${_name}_sources})
        target_link_libraries(${_name} ${${_name}_libraries})
        if(${_name}_install STREQUAL "yes")
            install(PROGRAMS ${_name} 
                 DESTINATION bin
                 PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                             GROUP_READ GROUP_EXECUTE)  
                    
        endif()
        
    # сборка статической библтотеки
    #
    elseif(${_type} STREQUAL "ARCHIVE")
        add_definitions(-fPIC)
        add_library(${_name} STATIC ${${_name}_sources})
        target_link_libraries(${_name} ${${_name}_libraries})
        if(${_name}_install STREQUAL "yes")
            install(TARGETS ${_name} 
                    ARCHIVE DESTINATION lib)
        endif()
        
    # сборка динамической библтотеки
    #   
    elseif(${_type} STREQUAL "LIBRARY")
        add_library (${_name} SHARED ${${_name}_sources})
        target_link_libraries(${_name} ${${_name}_libraries})
        if(${_name}_install STREQUAL "yes")
            install(TARGETS ${_name} 
                    SHARED DESTINATION lib)
        endif()
    
    # сборка модуля
    #
    elseif(${_type} STREQUAL "MODULE")
        add_library(${_name} MODULE ${${_name}_sources})
        target_link_libraries(${_name} ${${_name}_libraries})
        if(${_name}_install STREQUAL "yes")
            install(TARGETS ${_name} 
                    SHARED DESTINATION lib)
        endif()
        
    # неизвестный тип
    #
    else()
        message(FATAL_ERROR "Not valid build type: " ${_type})
    endif()

endmacro(CMAKE_COMPILE)
