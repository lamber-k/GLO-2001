NAME =	libUFS.a

SRCS =	UFS.c		\
	disque.c	\
	utils.c		\
	io_operation.c	\
	block_tools.c	\

F.C	     +=	-W -Wall
F.I	     += 
F.L	     += 
F.D	     +=

all:	start lib_compile

clean:	remove_objects

fclean:	clean remove_name

re:		fclean all

include .libmake
