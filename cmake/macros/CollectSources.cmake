
function(CollectSourceFiles current_dir variable)
	file(GLOB_RECURSE COLLECTED_SOURCES 
		${current_dir}/*.c
		${current_dir}/*.cc
		${current_dir}/*.cpp
		${current_dir}/*.inl
		${current_dir}/*.def
		${current_dir}/*.h
		${current_dir}/*.hh
		${current_dir}/*.hpp)
	set(${variable} ${COLLECTED_SOURCES} PARENT_SCOPE)
endfunction()