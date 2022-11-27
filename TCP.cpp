#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

int main(){
    char input[3];
    while (1){
        fgets(input, 3, stdin); /*3 primeiras letras para verificar instrução*/
        switch (*input){
            case 'GSB':
                /*Following the scoreboard command, the Player application opens a TCP connection with the GS and asks to receive the top-10 scoreboard.*/
                break;

            case 'RSB':
                /*In reply to a GSB request the GS sends a status response. If the scoreboard is still empty (no game was yet won by any player) the GS replies with status =
            EMPTY, otherwise the GS replies with status = OK, and sends a text file containing the top-10 scores of the game. The information sent includes:
                    • the filename Fname;
                    • the file size Fsize, in bytes;
                    • the contents of the selected file (Fdata).
                
                The text file contains one score per line consisting of the player ID PLID, followed by the number of plays needed, and the word length; these fields are
            separated by spaces. The file starts with the top score and can contain up to 10 lines, each line terminated with a ‘\n’. These scores are displayed as a
            numbered list by the Player.
                A local copy of the score board is stored using the filename Fname.
                After receiving the reply message, the Player closes the TCP connection with the GS.*/
                break;

            case 'GHL':
                /*Following the hint command, the Player application opens a TCP connection with the GS and asks to receive an image illustrating the class to which the word
            belongs.*/
            
                break;


            case 'RHL':
                /*In reply to a GHL request the GS replies with status = OK and sends a file containing the image illustrative of the word class. The information sent
            includes:
                    • the filename Fname;
                    • the file size Fsize, in bytes;
                    • the contents of the selected file (Fdata).
                The file is locally stored using the filename Fname.
                The Player displays the name and size of the stored file.
                If there is no file to be sent, or some other problem, the GS replies with status = NOK.
                After receiving the reply message, the Player closes the TCP connection with the GS.*/
                break;

            case 'STA':
                /*Following the state command, the Player application opens a TCP connection with the GS and asks about the state of the ongoing game at the Player.*/

                break;

            case 'RST':
                /*In reply to a STA request the GS replies depending on whether player PLID has an ongoing game or not.
                If there is an ongoing game, then status = ACT and the GS server sends a text file containing the summary game so far. Upon receiving this summary, the
            player displays the information and continues with the game.
                If there is no ongoing game for this player then status = FIN and the GS server responds with a file containing the summary of the most recently finished
            game for this player. Upon receiving this summary, the player displays the information and considers the current game as terminated.
                If the GS server finds no games (active or finished) for this player, then status = NOK.
                When replying with the game state information, a text file is sent. The reply message then includes:
                    • the filename Fname;
                    • the file size Fsize, in bytes;
                    • the contents of the selected file (Fdata).
                The text file includes the game state and a local copy of the game state is stored using the filename Fname. The name and size of the stored file are displayed.
                After receiving the reply message, the Player closes the TCP connection with the GS.
                If an unexpected protocol message is received, the reply will be ERR.
                In the above messages the separation between any two items consists of a single space.
                Each request or reply message ends with the character “\n”.*/
                break;
            default:
                /*gerar erro*/
                break;
        }
                
    }
}
