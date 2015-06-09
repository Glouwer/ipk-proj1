#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdbool.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <netinet/in.h>

//funkce k odstraneni prvni casti prijate zpravy (prefixu)
void substring(char s[], char sub[], int p) {
   int c = 0;
 
   while (c < strlen(s)) {
      sub[c] = s[p+c];
      c++;
   }
   sub[c] = '\0';
}

int main(int argc, char **argv)
{
	char* final_prep; 	//string prepinacu
	char prepinace[50];	//prepinace(N,G,L atd.)
	int prepinace_id=0; //indexer prepinacu
	char* loginy[50];   
	int loginy_id = 0;  
	char* uid[50];
	int uid_id = 0;
	char* host;
	int port;

	char msg[50]; //zprava na odeslani
    bool login_set = false;
    bool user_set = false;
    bool h_set = false;
    bool p_set = false;

  	struct sockaddr_in sin;
 	struct hostent *hptr;
  	int sock;
	memset(prepinace, 0, 50);
	memset(loginy, 0, 50);
	memset(uid, 0, 50);
	
	for(int i = 0; i < argc; i++){
		if(strcmp(argv[i], "-l") == 0){

			// nacita loginy			
			for(int l = i + 1; l < argc; l++){
				// nacitat dokud nenarazim na dalsi parametr
			
				if(argv[l][0] == '-'){
					// jedna se o dalsi parametr
					i = l - 1;
					break;
				}
				else {
					// nachystani pole pro parametr
					loginy[loginy_id] = malloc(sizeof(char) * (strlen(argv[l])+1));
					// skopirovani parametru do pole
					strcpy(loginy[loginy_id], argv[l]);
					loginy[loginy_id][strlen(argv[l])] = '\0';
					// inkrementace velikosti
					loginy_id++;
				}
			}
			
			login_set = true;
			user_set = false;		
		}
		//zpracovavam parametr -h
		else if(strcmp(argv[i], "-h") == 0){
			h_set = true;
			host = argv[i + 1];
		}
		//zpracovavam parametr -p
		else if(strcmp(argv[i], "-p") == 0){
			p_set = true;
			if((port = atoi(argv[i + 1])) == 0){
				fprintf(stderr, "Error - port neni cislo\n");
				return -1;
			}			
		}
		//zpracovavam parametr -u
		else if(strcmp(argv[i], "-u") == 0){
			// nacitas UID
			for(int u = i + 1; u < argc; u++){
				if(argv[u][0] == '-'){
					i = u - 1;
					break;				
				}
				else {
					uid[uid_id] = malloc(sizeof(char) * (strlen(argv[u])+1));
					// skopirovani parametru do pole
					strcpy(uid[uid_id], argv[u]);
					uid[uid_id][strlen(argv[u])] = '\0';

					//kontrolu zda UID je urcite cislo, pokud ne tak koncim s chybou	
					if(atoi(uid[uid_id]) == 0){
						fprintf(stderr, "Error - jedna z hodnot parametru -u NENI cislo\n");
						return -1;

					}
					// inkrementace velikosti
					uid_id++;
					
				}			
			}	
			
			login_set = false;
			user_set = true;	
		}
		// hledam jednotlive prepinace a postupne jak jdou si je ukladam do pole 
		else if((strcmp(argv[i], "-L") == 0) || (strcmp(argv[i], "-U") == 0) || 
			(strcmp(argv[i], "-N") == 0) || (strcmp(argv[i], "-G") == 0) ||
			(strcmp(argv[i], "-S") == 0) || (strcmp(argv[i], "-H") == 0)){
			prepinace[prepinace_id] = argv[i][1];
			prepinace_id++;
		}
		//osetreni pro pripad -NLG...
		else if(argv[i][0] == '-'){
			for(int p = 1; p < strlen(argv[i]); p++){
				if((argv[i][p] == 'L') || (argv[i][p] == 'U') || 
				(argv[i][p] == 'N') || (argv[i][p] == 'S') ||
				(argv[i][p] == 'G') || (argv[i][p] == 'H')){
					prepinace[prepinace_id] = argv[i][p];				
					prepinace_id++;
				}
				else{
					fprintf(stderr, "Error - neznamy prepinac pro vyhledavani\n");
					return -1;
				}
			}		
		}

	}
	
	//kontrola ze je zadan alespon jeden vyhledavaci parametr
	if(!login_set && !user_set){
		fprintf(stderr, "Error - je nutne mit alespon 1 vyhl. parametr\n");
		return -1;
	}

	//kontrola parametru - jsou 3 povinne parametry
	if((p_set == false) || (h_set == false)){
		fprintf(stderr, "Error - chybi jeden z parametru\n");
		return -1;
	}

/* KONTROLNI VYPISY
	printf("========================================\n");
	
	printf("LOGINY:\n");
	for(int i = 0; i < loginy_id; i++){
		printf("\t%s\n", loginy[i]);	
	}
	printf("UID:\n");
	for(int i = 0; i < uid_id; i++){
		printf("\t%s\n", uid[i]);	
	}
	
	printf("Prepinace:\n");
	final_prep = malloc(sizeof(char)*(5 + prepinace_id));
	strncpy(final_prep, "001:", 4);
	for(int i = 0; i < prepinace_id; i++){
		final_prep[i+4] = prepinace[i];
		final_prep[i+5] = '\0';
	}
	printf("\t%s\n", final_prep);
		
	printf("port: %d\n",port);
	printf("host: %s\n",host);
	printf("========================================\n");
*/



//******************  ZPRACOVAVAM KOMUNIKACI ***********************

	if((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0){
		fprintf(stderr, "Error - chyba vytvoreni socketu\n");
		return -1;
	}

	// nastaveni spojeni
	sin.sin_family = PF_INET;
	sin.sin_port = htons(port);
	
	// ziskani dat z DNS
	if((hptr = gethostbyname(host)) == NULL){
		fprintf(stderr, "Error - chyba ziskani dat z DNS\n");
		return -1;
	}

	// kopie dat z DNS do lokalni promenne
	memcpy(&sin.sin_addr, hptr->h_addr_list[0], hptr->h_length);

	if(connect(sock, (struct sockaddr*)&sin, sizeof(sin)) < 0){
		fprintf(stderr, "Error - chyba pripojeni k serveru\n");
		return -1;
	}

  	//pomocna zprava
  	char zprava[500];
  	memset(zprava, 0, 500);

  	//cti dokud mi chodi zpravy
  	if(read(sock, zprava , 500) > 0){
  		//uvitaci zprava
  		if(strcmp(zprava, "000:WELCOME") != 0){
  			fprintf(stderr, "Error - Server neposlal uvitaci zpravu!\n");
  			return -1;
  		}
  	}
  	
  	// odeslani prepinacu jako 1 string - zprava 001 pr. 001:NLG
  	final_prep = malloc(sizeof(char)*(5 + prepinace_id));
	strncpy(final_prep, "001:", 4);
	for(int i = 0; i < prepinace_id; i++){
		final_prep[i+4] = prepinace[i];
		final_prep[i+5] = '\0';
	}
  	write(sock, final_prep, strlen(final_prep));
  	free(final_prep);
  	memset(zprava, 0, 500);
  	if(read(sock, zprava, 500) > 0){
   		if(strcmp(zprava, "200:ACK") != 0){
  			fprintf(stderr, "Error - Serveru nejaka zprava neprisla!\n");
  			return -1;
  		}
  	}


	//pokud byl nastaveny login budu posilat zpravy 002:
	if(login_set){
		// odesilani loginu
		for(int i = 0; i < loginy_id; i++){		
			memset(msg, 0 , 50);
			sprintf(msg, "002:%s", loginy[i]);
		
			// uvolnit hned po odeslani
			free(loginy[i]);
		
			// zapsat na socket
			write(sock, msg, strlen(msg));
		
			memset(zprava, 0, 500);
			// prijmout vypis od serveru
			if(read(sock, zprava, 500) > 0){
				if(strncmp(zprava, "201:", 4) == 0){
					// prisel response od serveru vypsat vsechno od 4indexu
					char out[128];
					substring(zprava, out, 4);
					printf("%s\n", out);			
				}
				else if(strncmp(zprava, "400:",4) == 0){
					fprintf(stderr, "Error - neznamy uzivatel\n");
				}
				else {
					fprintf(stderr, "Error - neznam tuto zpravu\n");	
					return -1;
				}
			}
		}
	}
	// pokud byl nastaveny -u, budu vyhledavat podle nej
	if(user_set){
		// odesilani UID
		for(int i = 0; i < uid_id; i++){		
			memset(msg, 0 , 50);
			sprintf(msg, "003:%s", uid[i]);
			
			// uvolnit po odeslani
			free(uid[i]);
		
			// zapsat na socket
			write(sock, msg, strlen(msg));
		
			memset(zprava, 0, 500);
			// prijmout vypis od serveru
			if(read(sock, zprava, 500) > 0){
				if(strncmp(zprava, "201:", 4) == 0){
					// prisel response od serveru vypsat vsechno od 4indexu
					char out[128];
					substring(zprava, out, 4);
					printf("%s\n", out);			
				}
				else if(strncmp(zprava, "400:",4)==0){
					fprintf(stderr, "Error - neznamy uzivatel\n");
				}
				else {
					fprintf(stderr, "Error - neznam tuto zpravu\n");
					return -1;
				}
			}
		}
	}
	// odeslani ukonceni spojeni
	memset(msg, 0, 50);
	sprintf(msg, "202:BYE");
	write(sock, msg, strlen(msg));

	memset(zprava, 0, 500);
	if(read(sock, zprava, 500) > 0){
 		//printf("%s\n", zprava);
		if(strcmp(zprava, "202:BYE") == 0){
			// vsechno probehlo OK, je mozne zavrit socket
			// server ho uz zavrel
			close(sock);
		}
		else {
			fprintf(stderr, "Error - ocekaval jsem rozlouceni\n");
			return -1;
		}
	}	
		
	//uvolneni pameti
	for(int i = 0; i < loginy_id; i++){
		if(loginy[i] != NULL)
			free(loginy[i]);	
	}
	/*for(int i = 0; i < uid_id; i++){
		if(uid[i] != NULL)
			free(uid[i]);	
	}
	*/
	// vsechno OK
	return 0;
}
