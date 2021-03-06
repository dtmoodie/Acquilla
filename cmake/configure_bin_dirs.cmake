MACRO(APPEND_BIN_DIR ARG1 ARG2)
	IF(EXISTS "${${ARG1}}" AND IS_DIRECTORY "${${ARG1}}")
		LIST(APPEND ${ARG2} ${${ARG1}})
	ELSE()
		if(RCC_VERBOSE_CONFIG)
		  MESSAGE("${ARG1} : ${${ARG1}} is not a valid directory")
		endif()
	ENDIF()
ENDMACRO(APPEND_BIN_DIR)
  
macro(configure_bin_dirs target_name)
	set(${target_name}_BIN_DIRS_DEBUG "")
	set(${target_name}_BIN_DIRS_RELEASE "")
	set(${target_name}_BIN_DIRS_RELWITHDEBINFO "")
	# bin_dirs is the global list of binaries that may be useful
	FOREACH(dir ${BIN_DIRS})
		APPEND_BIN_DIR(${dir}_BIN_DIR_DBG ${target_name}_BIN_DIRS_DEBUG)
		APPEND_BIN_DIR(${dir}_BIN_DIR_OPT ${target_name}_BIN_DIRS_RELEASE)
		APPEND_BIN_DIR(${dir}_BIN_DIR_OPT ${target_name}_BIN_DIRS_RELWITHDEBINFO)
		APPEND_BIN_DIR(${dir}_BIN_DIR ${target_name}_BIN_DIRS_DEBUG)
		APPEND_BIN_DIR(${dir}_BIN_DIR ${target_name}_BIN_DIRS_RELEASE)
		APPEND_BIN_DIR(${dir}_BIN_DIR ${target_name}_BIN_DIRS_RELWITHDEBINFO)
	ENDFOREACH(dir ${BIN_DIRS})
	if(RCC_VERBOSE_CONFIG)
		message(STATUS "Binary runtime directories for ${target_name}:\n ${${target_name}_BIN_DIRS_DEBUG}")
	endif()
endmacro()