project: Gs Player 

Player:
		g++ -o Player.o -Wall -ggdb3 Player.cpp
		
Gs:
		g++ -o Gs.o -Wall -ggdb3 Gs.cpp

clean:
		rm *.o