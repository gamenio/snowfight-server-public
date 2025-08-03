
option(WITH_COREDEBUG   "Include additional debug-code in core"                       0)
set(WITH_SOURCE_TREE    "hierarchical" CACHE STRING "Build the source tree for IDE's.")
set_property(CACHE WITH_SOURCE_TREE PROPERTY STRINGS no flat hierarchical hierarchical-folders)
