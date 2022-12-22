project: Gs Player 

Player:
		g++ -o Player -Wall -ggdb3 Player.cpp
		
Gs:
		g++ -o Gs -Wall -ggdb3 Gs.cpp

clean:
		rm Player Gs