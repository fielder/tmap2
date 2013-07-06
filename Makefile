SDLCFLAGS = -I/usr/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT
SDLLDFLAGS = -lSDL -lpthread

CC = gcc
CFLAGS = -Wall -O2 $(SDLCFLAGS)
LDFLAGS = -lm $(SDLLDFLAGS)
OBJDIR = obj
TARGET = $(OBJDIR)/tmap2

OBJS =	$(OBJDIR)/tmap2.o \
	$(OBJDIR)/vec.o \
	$(OBJDIR)/render.o \
	$(OBJDIR)/clip.o \
	$(OBJDIR)/edge.o

all: $(TARGET)

clean:
	rm -f $(OBJDIR)/*.o
	rm -f $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

########################################################################

$(OBJDIR)/tmap2.o: tmap2.c
	$(CC) -c $(CFLAGS) $? -o $@
$(OBJDIR)/vec.o: vec.c
	$(CC) -c $(CFLAGS) $? -o $@
$(OBJDIR)/render.o: render.c
	$(CC) -c $(CFLAGS) $? -o $@
$(OBJDIR)/clip.o: clip.c
	$(CC) -c $(CFLAGS) $? -o $@
$(OBJDIR)/edge.o: edge.c
	$(CC) -c $(CFLAGS) $? -o $@
