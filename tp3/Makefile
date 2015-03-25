NAME.UFS =	ufs
NAME.GLOFS =	glofs

SRCS.UFS = 	main_ufs.c
SRCS.GLOFS = 	glofs.c

DIR.LIBUFS =	./libUFS/

F.C	     +=	-W -Wall
F.I	     += -I$(DIR.LIBUFS)/include
F.L	     += -L$(DIR.LIBUFS) -lUFS
F.D	     +=

all:	start compile_libufs $(NAME.UFS) $(NAME.GLOFS)

compile_libufs:
	$(ECHO) "Compile libUFS..."
	@$(MAKE)  --no-print-directory -C $(DIR.LIBUFS) F.C="$(F.C)" all
	$(ECHO) "Compile libUFS Done\n"

########### ufs #############

$(NAME.UFS): start
	$(ECHO) "Compile $(NAME.UFS)..."
	@$(MAKE) --no-print-directory SRCS="$(SRCS.UFS)" NAME="$(NAME.UFS)" compile

fclean_ufs:
	@$(RM) $(NAME.UFS)

########## glofs ############

$(NAME.GLOFS): start
	$(ECHO) "Compile $(NAME.GLOFS)..."
	@$(MAKE)  --no-print-directory SRCS="$(SRCS.GLOFS)" NAME="$(NAME.GLOFS)" F.L="$(F.L) -lfuse" F.D+="-D_FILE_OFFSET_BITS=64" compile 

fclean_glofs:
	@$(RM) $(NAME.GLOFS)

########## Default rules #########

clean:	remove_objects
	@$(MAKE)  --no-print-directory -C $(DIR.LIBUFS) clean

fclean:	fclean_ufs fclean_glofs
	@$(MAKE)  --no-print-directory -C $(DIR.LIBUFS) fclean

re:	fclean all

include .libmake
