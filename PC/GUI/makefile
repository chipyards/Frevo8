INCS= `pkg-config --cflags gtk+-2.0`

OBJS=  i2c_udp_mast.o dialu.o diali.o fpilot.o \
       crc32.o process.o xmlpe.o dirlist.o modpop2.o modadj.o catalog.o rview.o textgui.o artgui.o plot.o appli.o

HEADERS=  ../UDP/i2c_udp_mast.h ../UDP/dialu.h ../diali.h ../fpilot.h \
          ../crc32.h ../process.h ../xmlpe.h ../dirlist.h modpop2.h modadj.h catalog.h rview.h glostru.h textgui.h artgui.h plot.h

# cibles

ALL : $(OBJS) 
	g++ -o gui83 $(OBJS) `pkg-config --libs gtk+-2.0` -lm

clean : 
	rm *.o

i2c_udp_mast.o : ../UDP/i2c_udp_mast.c ${HEADERS}
	gcc -Wall -c -O3 ../UDP/i2c_udp_mast.c

dialu.o : ../UDP/dialu.c ${HEADERS}
	gcc -Wall -Wno-misleading-indentation -c -O3 ../UDP/dialu.c

diali.o : ../diali.c ${HEADERS}
	gcc -Wall -Wno-misleading-indentation -c -O3 ../diali.c

fpilot.o : ../fpilot.c ${HEADERS}
	gcc -Wall -c -O3 -fno-strict-aliasing ../fpilot.c

crc32.o : ../crc32.c ${HEADERS}
	gcc -Wall -c -O3 ../crc32.c

process.o : ../process.cpp  ${HEADERS}
	g++ -Wall -c -O3 ../process.cpp

xmlpe.o : ../xmlpe.cpp ${HEADERS}
	g++ -Wall -Wno-misleading-indentation -c -O3 ../xmlpe.cpp

dirlist.o : ../dirlist.cpp  ${HEADERS}
	g++ -Wall -Wno-unused-result -c -O3 ../dirlist.cpp

modpop2.o : modpop2.c ${HEADERS}
	gcc -Wall $(INCS) -c -O3 modpop2.c

modadj.o : modadj.c ${HEADERS}
	gcc -Wall $(INCS) -c -O3 modadj.c

catalog.o : catalog.cpp  ${HEADERS}
	g++ -Wall -Wno-unused-result $(INCS) -c -O3 catalog.cpp

rview.o : rview.cpp  ${HEADERS}
	g++ -Wall $(INCS) -c -O3 rview.cpp

textgui.o : textgui.cpp  ${HEADERS}
	g++ -Wall $(INCS) -c -O3 textgui.cpp

artgui.o : artgui.cpp  ${HEADERS}
	g++ -Wall $(INCS) -c -O3 artgui.cpp

plot.o : plot.cpp  ${HEADERS}
	g++ -Wall -Wno-unused-result $(INCS) -c -O3 -fno-strict-aliasing plot.cpp

appli.o : appli.cpp ${HEADERS}
	g++ -Wall $(INCS) -c -O3 appli.cpp
