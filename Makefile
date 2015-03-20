NAME_UFS =	ufs
SRCS_UFS =	main_ufs.c	\
		UFS.c		\
		disque.c	\

NAME_UFS_TEST =	ufsTest
SRCS_UFS_TEST =	mainTest.c	\
		UFS.c		\
		disque.c	\

NAME_TEST_ETU =	TestEtudiant
SRCS_TEST_ETU =	mainScriptTest.c	\
		UFS.c			\
		disque.c		\

NAME_GLOFS =	glofs
SRCS_GLOFS =	glofs.c		\
		UFS.c		\
		disque.c	\

SRCS ?=	$(SRCS_UFS) #Default Behaviour
NAME ?= $(NAME_UFS) #Default Behaviour



F.C	     +=	-W -Wall
F.I	     += 
F.L	     += 
F.D	     +=

all:	start ufs glofs


ufs:	start
	$(MAKE) compile SRCS="$(SRCS_UFS)" NAME="$(NAME_UFS)"


ufsTest: start
	$(MAKE) compile SRCS="$(SRCS_UFS_TEST)" NAME="$(NAME_UFS_TEST)"

TestEtudiant: start
	$(MAKE) compile SRCS="$(SRCS_TEST_ETU)" NAME="$(NAME_TEST_ETU)"

glofs: start
	$(MAKE) compile SRCS="$(SRCS_GLOFS)" NAME="$(NAME_GLOFS)" F.D="-D_FILE_OFFSET_BITS=64" F.L="-lfuse"


clean:		clean_ufs

clean_ufs:	SRCS := $(SRCS_UFS)
clean_ufs: 	remove_objects

clean_glofs:	SRCS = $(SRCS_GLOFS)
clean_glofs: 	remove_objects

fclean:		fclean_ufs

fclean_ufs:	NAME := $(NAME_UFS)
fclean_ufs:	clean_ufs remove_name

fclean_glofs:	NAME := $(NAME_GLOFS)
fclean_glofs:	clean_glofs remove_name

re:		fclean all

include .libmake
