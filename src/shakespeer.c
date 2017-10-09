#include "peer.h"

int connect_ring(node client, int port)
{
	int i;
	int fbd = 1;
	
	int service_ext;
	int service_prec;
	
	struct hostent * he;
	he = gethostbyname("localhost");
	
	/* Initialisation des chaines utilisées pour les échanges de message */
	char to_send[BUF_SIZE];
	char recu[BUF_SIZE];
	char ** cmd;
	
	/* Mes sockets */
	client.ring_prec = socket(AF_INET, SOCK_STREAM,0);
	client.ring_next = socket(AF_INET, SOCK_STREAM,0);
	
	/* Adresse où on se connecte */
	memset((char *)&client.nextaddr,'0', sizeof(struct sockaddr_in));
	client.nextaddr.sin_family = AF_INET;
	client.nextaddr.sin_port = htons(7000);
	client.nextaddr.sin_addr.s_addr = inet_addr(ADDRESS);
	
	/* Mon adresse */
	memset((char *)&client.selfaddr, '0', sizeof(struct sockaddr_in));
	client.selfaddr.sin_family = AF_INET;
	memcpy(&client.selfaddr.sin_addr, he->h_addr, sizeof(client.selfaddr.sin_addr));
	client.selfaddr.sin_port = htons(port);
	
	/* Je bind mon prec pour mon futur prédécesseur */
	bind(client.ring_prec, (struct sockaddr *)&client.selfaddr, sizeof(client.selfaddr));
	listen(client.ring_prec,5);
	
	/* Connexion au pair init */
	printf("Tentative de connexion à l'hôte : %s \n", inet_ntoa(client.nextaddr.sin_addr));
	while(connect(client.ring_next, (struct sockaddr *)&client.nextaddr, sizeof(client.nextaddr)) == -1);
	printf("Client connecté à : %s \n", inet_ntoa(client.nextaddr.sin_addr));
	
	bzero(to_send, BUF_SIZE);
	bzero(recu, BUF_SIZE);
	
	/* Attente de la demande de JOIN */
	do
	{
		scanf(" %s", to_send);
	}
	while(!checkcmd("JOIN",to_send));
	
	write(client.ring_next, &to_send, strlen(to_send)+1);
	
	/* Réception de mon ID */
	bzero(recu, BUF_SIZE);
	read(client.ring_next, recu, BUF_SIZE);
	client.ID = atoi(recu);
	printf("Mon ID est %d \n",client.ID);
	printf("Attente de connexion au RING \n");
	sleep(3);
	bzero(to_send, BUF_SIZE);
	sprintf(to_send,"%d",port);
	write(client.ring_next, &to_send, strlen(to_send)+1);
	
	struct sockaddr_in ringaddr;
	
	socklen_t addrlen;
	addrlen = sizeof(ringaddr);
	
	memset((char *)&ringaddr,'0', sizeof(struct sockaddr_in));
	
	printf("J'accepte la connexion de mon prédécesseur \n");
	service_prec = accept(client.ring_prec, (struct sockaddr *)&ringaddr, &addrlen);
	printf("Connexion à mon prédécesseur réussie \n");
	read(service_prec, recu, BUF_SIZE);
	printf("Message de mon prédécesseur : %s \n",recu);
	
	struct timeval timeout;
	
	fd_set readfd, writefd, tempr, tempw;
	FD_ZERO(&readfd);
	FD_ZERO(&writefd);
	FD_SET(0, &readfd);
	FD_SET(client.ring_prec,&readfd);
	FD_SET(client.ring_next,&readfd);
	FD_SET(service_prec, &readfd);
	FD_SET(client.ring_prec,&writefd);
	FD_SET(client.ring_next,&writefd);
	FD_SET(service_prec, &writefd);
		
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
				bzero(to_send, BUF_SIZE);
				sprintf(to_send, "RING:%d", client.ID);
				write(client.ring_next, &to_send, strlen(to_send)+1);
			}
			if(checkcmd("BROADCAST", recu))
			{
				bzero(to_send, BUF_SIZE);
				
				fgets(to_send, BUF_SIZE, stdin);
				
				if(fbd && checkcmd("\0",to_send))
				{
					fgets(to_send, BUF_SIZE, stdin);
					fbd = 0;
				}
				
				strcat(recu,temp);
				strcat(recu,":");
				strcat(recu,to_send);
				
				write(client.ring_next, &recu, strlen(recu)+1);
			}
			if(checkcmd("GET", recu))
			{
				printf("TO DO \n");
				/*
				 * Lire le nom du fichier
				 * Envoyer GET:MON_ID:NOM_FICHIER ainsi que mon adresse à mon successeur
				 */
			}
			if(checkcmd("EXIT",recu))
			{
				bzero(to_send, BUF_SIZE);
				
				/* J'envoie les infos du nouveau client à mon prédécesseur */
				sprintf(to_send,"SUCC");
				write(service_prec, &to_send, strlen(to_send)+1);
				
				write(service_prec, &client.nextaddr, sizeof(client.nextaddr));
				
				/* Je me coupe de mon prédécesseur */
				close(service_prec);
				close(client.ring_prec);
				
				bzero(to_send, BUF_SIZE);
				
				/* Je dis à mon successeur que je m'en vais */
				sprintf(to_send,"EXIT");
				write(client.ring_next, &to_send, strlen(to_send)+1);
				
				close(client.ring_next);
				
				printf("Au revoir ! \n");
				
				return 0;
			}
		}
		/* Message reçu de mon prédécesseur */
		if( FD_ISSET(service_prec, &tempr) )
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
			if(checkcmd("GET",cmd[0]))
			{
				printf("TO DO \n");
				/*
				 * read(service_prec) pour récupérer l'adresse
				 * SI atoi(cmd[1]) == client.ID
				 * ALORS printf("Personne n'a le fichier");
				 * SINON
				 * {
				 * 		OpenRepertoire
				 * 		while(FICHIER = LireRepertoire)
				 * 		{
				 * 			SI FICHIER == NULL
				 * 			ALORS 	pasfichier = 1;
				 * 					BREAK
				 * 			SI FICHIER.NOM == cmd[2]
				 * 			ALORS	Connexion à l'adresse récupérée
				 * 					Envoi de NOTIFY
				 * 					OpenFichier
				 * 					LectureBufferisée
				 * 					EcritureBufferisée vers socket
				 * 					CloseFichier
				 * 					CloseSocket
				 * 					BREAK;
				 * 		}
				 * 		SI pasfichier = 1
				 * 			Envoyer le tout à mon successeur
				 * 	}
				 */
			}
			if(checkcmd("EXIT",cmd[0]))
			{
				printf("Mon prédécesseur change \n");
				/* Je me coupe de mon prédécesseur */
				FD_CLR(service_prec, &readfd);
				FD_CLR(service_prec, &writefd);
				close(service_prec);
				
				/* Je reçois un nouveau prec */
				service_prec = accept(client.ring_prec, (struct sockaddr *)&ringaddr, &addrlen);
				FD_SET(service_prec,&readfd);
				FD_SET(service_prec,&writefd);
				bzero(recu, BUF_SIZE);
				read(service_prec, recu, BUF_SIZE);
				printf("Message de mon nouveau prec : %s \n",recu);
			}
		}
		if( FD_ISSET(client.ring_next, &tempr) )
		{
			bzero(recu,BUF_SIZE);
			read(client.ring_next, recu, BUF_SIZE);
			/* Changement de successeur */
			if(checkcmd("SUCC",recu))
			{
				printf("Mon successeur change \n");
				read(client.ring_next, &client.nextaddr, BUF_SIZE);
				
				FD_CLR(client.ring_next, &readfd);
				FD_CLR(client.ring_next, &writefd);
				close(client.ring_next);
				client.ring_next = socket(AF_INET, SOCK_STREAM,0);
				printf("Connexion au nouveau successeur \n");
				while(connect(client.ring_next, (struct sockaddr *)&client.nextaddr, sizeof(client.nextaddr)) == -1);
				printf("Nouveau successeur en place \n");
				sprintf(to_send,"Je suis %d, ton nouveau prédécesseur",client.ID);
				write(client.ring_next, &to_send, strlen(to_send)+1);
				FD_SET(client.ring_next, &readfd);
				FD_SET(client.ring_next, &writefd);
			}
		}
	}
}


int main(int argc, char ** argv)
{
	node client;
	
	int port = atoi(argv[1]);
	
	connect_ring(client,port);
	
	return 0;
}


