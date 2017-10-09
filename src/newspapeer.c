#include "peer.h"

int new_ring(node client)
{
	int i;
	int ID_MAX = 0;
	
	int seul = 1;
	
	int service_ext;
	int service_prec;
	
	struct hostent * he;
	he = gethostbyname("localhost");
	
	/* Initialisation des chaines utilisées pour les échanges de message */
	char recu[BUF_SIZE];
	char to_send[BUF_SIZE];
	char ** cmd;
	
	/* Mes sockets */
	client.ext = socket(AF_INET,SOCK_STREAM,0);
	client.ring_next = socket(AF_INET, SOCK_STREAM,0);
	
	/* Mon addresse */
	memset((char*)&client.selfaddr,'0',sizeof(struct sockaddr_in));
	client.selfaddr.sin_family = AF_INET;
	memcpy(&client.selfaddr.sin_addr, he->h_addr, sizeof(client.selfaddr.sin_addr));
	client.selfaddr.sin_port = htons(7000);

	/* Je bind ma socket externe */
	bind(client.ext, (struct sockaddr *)&client.selfaddr, sizeof(client.selfaddr));
	listen(client.ext,5);
	
	struct sockaddr_in inc_client;
	
	socklen_t addrlen;
	addrlen = sizeof(inc_client);
	
	memset((char *)&inc_client,'0', sizeof(struct sockaddr_in));
	
	struct timeval timeout;
	
	fd_set readfd, writefd, tempr, tempw;
	FD_ZERO(&readfd);
	FD_ZERO(&writefd);
	FD_SET(0, &readfd);
	FD_SET(client.ext,&readfd);
	FD_SET(client.ext,&writefd);
		
	int error;
	
	char temp[20];
	sprintf(temp,":%d",client.ID);
	
	while(1)
	{
		tempr = readfd;
		tempw = writefd;
		
		timeout.tv_sec = 5;
		timeout.tv_usec = 0;
		
		bzero(to_send, BUF_SIZE);
		bzero(recu,BUF_SIZE);
		
		if( (error = select(FD_SETSIZE, &tempr, &tempw, NULL, &timeout)) == -1)
			perror("");
		
		/* Message reçu sur l'entrée standard */
		if( FD_ISSET(0, &tempr) )
		{
			bzero(recu, BUF_SIZE);
			read(0, recu, BUF_SIZE);
			if(checkcmd("RING",recu))
			{
				if(seul)
				{
					printf("Ring is : {%d} \n",client.ID);
				}
				else
				{
					bzero(to_send, BUF_SIZE);
					sprintf(to_send, "RING:%d", client.ID);
					write(client.ring_next, &to_send, strlen(to_send)+1);
				}
			}
			if(checkcmd("BROADCAST", recu))
			{
				bzero(to_send, BUF_SIZE);
				
				fgets(to_send, BUF_SIZE, stdin);
				
				if(seul)
				{
					printf("  >  %d dit : %s \n", client.ID, to_send);
				} 
				else
				{
					strcat(recu,temp);
					strcat(recu,":");
					strcat(recu,to_send);
				
					write(client.ring_next, &recu, strlen(recu)+1);
				}
			}
			if(checkcmd("EXIT", recu) && seul)
			{
				close(client.ext);
				close(service_ext);
				close(service_prec);
				close(client.ring_next);
				return 0;
			}
		}
		/* Message reçu de mon successeur */
		if( !(seul) && FD_ISSET(client.ring_next, &tempr) )
		{
			bzero(recu,BUF_SIZE);
			if(!seul && (read(client.ring_next, recu, BUF_SIZE) != -1))
			{
				/* 
				 * Changement de successeur 
				 * Réception des nouvelles données
				 * Connexion au nouveau successeur
				 */
				if(checkcmd("SUCC",recu))
				{
					printf("Mon successeur change \n");
					read(client.ring_next, &client.nextaddr, BUF_SIZE);
					
					/* Vérification pour savoir si je ne suis pas mon propre successeur */
					int test1 = checkcmd(inet_ntoa(client.nextaddr.sin_addr),inet_ntoa(client.selfaddr.sin_addr));
					int test2 = ( (int) ntohs(client.nextaddr.sin_port) == (int) ntohs(client.selfaddr.sin_port) );
				
					if(test1 && test2)
					{
						ID_MAX--;
						printf("Je suis de nouveau seul \n");
						seul = 1;
						FD_CLR(client.ring_next, &readfd);
						FD_CLR(client.ring_next, &writefd);
						close(client.ring_next);
						close(service_ext);
						FD_CLR(service_prec, &readfd);
						FD_CLR(service_prec, &writefd);
						close(service_prec);
						client.ring_next = socket(AF_INET, SOCK_STREAM,0); 
					}
					else
					{
						FD_CLR(client.ring_next, &readfd);
						FD_CLR(client.ring_next, &writefd);
						close(client.ring_next);
						client.ring_next = socket(AF_INET, SOCK_STREAM,0);
						while(connect(client.ring_next, (struct sockaddr *)&client.nextaddr, sizeof(client.nextaddr)) == -1);
						sprintf(to_send,"Je suis %d, ton nouveau prédécesseur",client.ID);
						write(client.ring_next, &to_send, strlen(to_send)+1);
						FD_SET(client.ring_next, &readfd);
						FD_SET(client.ring_next, &writefd);
					}
				}
			}
		}
		/* Message reçu de mon prédécesseur */
		if( !(seul) && FD_ISSET(service_prec, &tempr) )
		{
			bzero(recu,BUF_SIZE);
			read(service_prec, recu, BUF_SIZE);
			cmd = split(recu, ':');
			
			if(checkcmd("BROADCAST",cmd[0]))
			{
				printf("  >  %d dit : %s \n",atoi(cmd[1]),cmd[2]);
				if(atoi(cmd[1]) != client.ID)
					write(client.ring_next, &recu, strlen(recu)+1);
			}
			if(checkcmd("RING",cmd[0]))
			{	
				strcat(recu,temp);
				cmd = split(recu,':');
				
				if(atoi(cmd[1]) == client.ID)
				{
					bzero(recu, BUF_SIZE);
					sprintf(recu, "%d",client.ID);
					for(i=2; atoi(cmd[i]) != client.ID; i++)
					{
						strcat(recu,", ");
						strcat(recu,cmd[i]);
					}
					printf("Ring is {%s}\n",recu);
				}
				else
				{
					write(client.ring_next, &recu, strlen(recu)+1);
				}
			}
			if(checkcmd("EXIT",cmd[0]))
			{
				printf("Mon prec s'en va \n");
				/* Je me coupe de mon prédécesseur */
				FD_CLR(service_prec, &readfd);
				FD_CLR(service_prec, &writefd);
				close(service_prec);
				
				ID_MAX--;
				
				if(ID_MAX == 0 || seul)
				{
					printf("Je me retrouve seul \n");
					seul = 1;
				}
				else
				{
					/* Je reçois un nouveau prec */
					printf("Attente de mon nouveau prec \n");
					service_prec = accept(client.ext, (struct sockaddr *)&inc_client, &addrlen);
					FD_SET(service_prec,&readfd);
					FD_SET(service_prec,&writefd);
					printf("Nouveau prédécesseur en place \n");
				}
			}
		}
		/* Si il y a une tentative de connexion sur ma socket d'écoute */
		if( FD_ISSET(client.ext,&tempr) )
		{
			printf("Tentative de connexion \n");
			service_ext = accept(client.ext, (struct sockaddr *)&inc_client, &addrlen);
			printf("Connexion acceptée \n");
			
			read(service_ext, recu, BUF_SIZE);
			/* Si je reçois JOIN */
			if(checkcmd("JOIN",recu))
			{
				ID_MAX++;
				
				bzero(to_send, BUF_SIZE);
				sprintf(to_send, "%d", ID_MAX);
				write(service_ext, &to_send, strlen(to_send)+1);
				
				/* réception de son port d'écoute */
				bzero(recu,BUF_SIZE);
				read(service_ext, recu, BUF_SIZE);
				int pecoute = atoi(recu);
				printf("Port d'écoute récupéré \n");
				inc_client.sin_port = htons(pecoute);
								
				if(!seul)
				{
					/* Cas où j'ai un prédécesseur et un successeur */
					/* J'envoie les infos du nouveau client à mon prédécesseur */
					sprintf(to_send,"SUCC");
					write(service_prec, &to_send, strlen(to_send)+1);
					write(service_prec, &inc_client, sizeof(inc_client));
					
					/* Je me coupe de mon prédécesseur */
					FD_CLR(service_prec, &readfd);
					FD_CLR(service_prec, &writefd);
					close(service_prec);
					printf("Client ajouté au RING \n");
				}
				else
				{
					/* Cas où je suis pour l'instant seul dans le RING */
					memset((char*)&client.nextaddr,'0',sizeof(struct sockaddr_in));
					
					client.nextaddr = inc_client;
					
					/* Connexion à mon successeur */
					printf("Je me connecte au nouveau client \n");
					while(connect(client.ring_next, (struct sockaddr *)&client.nextaddr, sizeof(client.nextaddr)) == -1);
					FD_SET(client.ring_next, &readfd);
					FD_SET(client.ring_next, &writefd);
					printf("Je ne suis plus seul. \n");
					bzero(to_send, BUF_SIZE);
					sprintf(to_send,"Je suis le pair initiateur, bienvenue dans le RING.");
					write(client.ring_next, &to_send, strlen(to_send)+1);
					
					seul = 0;
				}
				
				/* Je change mon prédécesseur */
				service_prec = service_ext;
				service_ext = -1;
				FD_SET(service_prec, &readfd);
				FD_SET(service_prec, &writefd);
			}
		}
	}
}


int main(int argc, char ** argv)
{	
	node client;
	
	client.ID = 0;

	new_ring(client);
	
	return 0;
}
