#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <time.h>
#include <sys/time.h>
#include <pwd.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/un.h>
#include <errno.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <string.h>

//funkce k odstraneni prvni casti prijate zpravy (prefixu)
void substring(char s[], char sub[], int p) {
   int c = 0;
 
   while (c < strlen(s)) {
      sub[c] = s[p+c];
      c++;
   }
   sub[c] = '\0';
}
// funkce na obslouzeni clienta - odesilani zprav
int start_child_process(int sock_client){
	if(sock_client < 0){
		fprintf(stderr, "Error - chyba socketu\n");
		return -1;
	}
	
	
	char msg[1024];  //zprava
	char params[10]; // parametry - prepinace
	int params_id = 0;	
	
	int n;
	
	memset(params, 0, 10);
	
	// odeslani uvitaci zpravy
	sprintf(msg, "000:WELCOME");
	write(sock_client, msg, strlen(msg));	  

	//dokud je otevreny socket
	while (sock_client > 0){		
		memset(msg, 0, 1024);
		// nacitani zprav a porovnavani jake zpravy mi prisly
		if((n = read(sock_client, msg, 1024)) > 0){
			//pokud je to 001 jsou to prepinace
			if(strncmp(msg, "001", 3) == 0){
				//zpracovani parametru
				for(int i = 4; i < n; i++){
					params[params_id] = msg[i];
					params_id++;
		    	}
		    	
		    	// potvrzeni prijeti, vracim ACK
		    	memset(msg, 0, 1024);
		    	sprintf(msg, "200:ACK");
		    	write(sock_client, msg, strlen(msg));
			}
			//pokud je to 002 vim ze budu hledat podle loginu
			else if(strncmp(msg,"002", 3) == 0){
				//zpracovani loginu
			  	// 002:<login>
				char login[128];
				substring(msg, login, 4);

				memset(msg, 0, 1024);
				sprintf(msg, "201:");
				
				struct passwd* pwd;
				pwd = getpwnam(login);
				if(pwd != NULL){
					//dostal vysledek
					bool space = false;
					char tmp[16];
					for(int i = 0; i < params_id; i++){
						if(i > 0)
							space = true;
						//kontrola jake prepinace mi prisly pro vraceni vysledku
						switch(params[i]){
							case 'N':
								if(space)
									strcat(msg, " ");
								
								strcat(msg, pwd->pw_gecos);
								break;
							case 'L':
								// vratit login
								if(space)
									strcat(msg, " ");
								
								strcat(msg, pwd->pw_name);
								break;
							case 'G':
								// pwd.pw_gid
								memset(tmp, 0, 16);
								sprintf(tmp, "%d", pwd->pw_gid);
								
								if(space)
									strcat(msg, " ");
								
								strcat(msg, tmp);
								break;
							case 'S': 
								// pwd.pw_shell
								if(space)
									strcat(msg, " ");
								
								strcat(msg, pwd->pw_shell);
								break;							
							case 'H':
								// pwd.pw_dir
								if(space)
									strcat(msg, " ");
								
								strcat(msg, pwd->pw_dir);
								break;
							case 'U':
								// pwd.pw_uid
								memset(tmp, 0, 16);
								sprintf(tmp, "%d", pwd->pw_uid);
								
								if(space)
									strcat(msg, " ");								
								strcat(msg, tmp);
								break;
							default:
								break;
						}				
					}
					//posilam clientovi vysledek
					write(sock_client, msg, strlen(msg));							
				}											 
				else {
					// nenasel vyskyt
					memset(msg, 0, 1024);
					sprintf(msg, "400:Chyba");
					write(sock_client, msg, strlen(msg));			
				}
			}
			//budu vyhledavat podle uid 
			else if(strncmp(msg,"003", 3) == 0){
				char u[128];
				int uid;

				substring(msg, u, 4);

				memset(msg, 0, 1024);
				sprintf(msg, "201:"); //zprava 201 vraci vysledek
				
				struct passwd* pwd;

				//prevedu na cislo a dam vyhledavat
				uid = atoi(u);
				pwd = getpwuid(uid);
				if(pwd != NULL){
					//dostal vysledek
					bool space = false;
					char tmp[16];
					for(int i = 0; i < params_id; i++){
						if(i > 0)
							space = true;
						//kontrola jake prepinace mi prisly pro vraceni vysledku
						switch(params[i]){
							case 'N':
								if(space)
									strcat(msg, " ");
								
								strcat(msg, pwd->pw_gecos);
								break;
							case 'L':
								// vratit login
								if(space)
									strcat(msg, " ");
								
								strcat(msg, pwd->pw_name);
								break;
							case 'G':
								// pwd.pw_gid
								memset(tmp, 0, 16);
								sprintf(tmp, "%d", pwd->pw_gid);
								
								if(space)
									strcat(msg, " ");
								
								strcat(msg, tmp);
								break;
							case 'S': 
								// pwd.pw_shell
								if(space)
									strcat(msg, " ");
								
								strcat(msg, pwd->pw_shell);
								break;							
							case 'H':
								// pwd.pw_dir
								if(space)
									strcat(msg, " ");
								
								strcat(msg, pwd->pw_dir);
								break;
							case 'U':
								// pwd.pw_uid
								memset(tmp, 0, 16);
								sprintf(tmp, "%d", pwd->pw_uid);
								
								if(space)
									strcat(msg, " ");								
								strcat(msg, tmp);
								break;
							default:
								break;
						}				
					}
					//posilam clientovi vysledek
					write(sock_client, msg, strlen(msg));							
				}											 
				else {
					// nenasel vyskyt
					memset(msg, 0, 1024);
					sprintf(msg, "400:Chyba");
					write(sock_client, msg, strlen(msg));			
				}


			}
			//pokud mi prislo rozlouceni, rozloucim se taky
			else if(strncmp(msg,"202", 3) == 0){
				memset(msg, 0, 1024);
				sprintf(msg, "202:BYE");
				write(sock_client, msg, strlen(msg));
				// client poslal bye
				// uzavrit spojeni
				return 0;
			}
		}
	}
	
	return 0;
}

int main(int argc, char **argv) 
{
    int port;

    char buffer[100];
	memset(buffer, 0, 100);

	if (argc < 3){
		fprintf(stderr, "Error - chybi parametr -p\n");
		return -1;
	}
    else if (argc > 3) { 
    	fprintf(stderr, "Error - Bylo zadano prilis mnoho argumentu\n");
        return -1;
    } else {
		if (strcmp (argv[1],"-p") == 0) {
		    if((port = atoi(argv[2])) == 0){
		    	fprintf(stderr, "Error - chyba parametru -p\n");
		    	return -1;
		    }
		}
	}

	//kontrola jestli byl zadan parametr p

  	int sock, sock_client;
  	struct sockaddr_in sin;

	if((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0){
		fprintf(stderr, "Error - nebylo mozne vytvorit socket\n");
		return -1;
	}

	sin.sin_family = PF_INET;
	// localhost
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(port);
  	
  	if(bind(sock, (struct sockaddr*)&sin, sizeof(sin)) < 0){
  		fprintf(stderr, "Error - chyba na navazani na local\n");
  		close(sock);
  		return -1;
  	}
		
	if ((listen(sock, 400)) == -1) {
		fprintf(stderr, "Error - chyba v naslouchani pro socket\n");
    	close(sock);
    	return -1;
    }
	
	socklen_t len = sizeof(sin);
	//dokud je otevreny
 	while (sock > 0){
		if((sock_client = accept(sock, (struct sockaddr*)&sin, &len)) < 0){
			fprintf(stderr, "Error - chyba pri prijimani spojeni\n");
			close(sock);
			return -1;		
		}

		pid_t child = fork();

		if(child == 0){ // child proces
			start_child_process(sock_client);

			close(sock_client);
			return 0;
		}
		else if(child > 0){
			// parent proces
		  	continue;			
		}
		else {
			fprintf(stderr, "Error - chyba forku\n");
			close(sock);
			return -1;
		}
	}
	
	close(sock);
	return 0;
}
