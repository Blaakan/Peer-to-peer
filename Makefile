vpath %.h include
vpath %.c src
vpath %.o obj

CC = gcc
CFLAGS = -g -Wall
IFLAGS = -Iinclude
ALL = all
INIT = newspapeer
CLIENT = shakespeer
OBJINIT = obj/newspapeer.o obj/peer.o
OBJCLIENT = obj/shakespeer.o obj/peer.o
OPATH = obj
CPATH = src
BPATH = bin

$(ALL) : $(INIT) $(CLIENT)

$(INIT) : $(OBJINIT)
	$(CC) -o $@ $^ 
	mv $@ $(BPATH)

$(CLIENT) : $(OBJCLIENT)
	$(CC) -o $@ $^ 
	mv $@ $(BPATH)
	
$(OPATH)/%.o : $(CPATH)/%.c
	$(CC) $(CFLAGS) -c $< $(IFLAGS) -o $@


clean : 
	rm -r $(OPATH)/* $(BPATH)/* 

