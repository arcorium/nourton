add_library(tinyfd STATIC
	tinyfiledialogs.h
	tinyfiledialogs.c
	tinyfd_moredialogs.h
	tinyfd_moredialogs.c)

target_include_directories(tinyfd INTERFACE .)