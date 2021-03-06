/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>

#define PORT "3490"  // the port users will be connecting to

#define BACKLOG 10     // how many pending connections queue will hold

#define BUFSIZE 20

#define MAXDATASIZE 100 // max number of bytes we can get at once 

#define MAX 100

int initiateServer();
FILE* openBD();
void listaISBN(int tamanho);
void zeraBuffer(char *buf);
void popula_banco();
void responde();
void readSocket(int socket, char* buf);
void writeSocket(int socket, char* buf);
void avaliaOpcao(char *buf, struct timeval *tv1, struct timeval *tv2);
void closeBD();
void lista_titulo(char *buf, FILE *bd, int quant);
void lista_descricao(char *buf, FILE *bd, int quant, char ISBN[], int ret);
void imprime(int col, int lin, FILE *bd, char *buf);
void altera_bd(char *buf, FILE *bd, int quant, char ISBN[]);

void sigchld_handler(int s){
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa){
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


typedef struct Livro {
	char isbn[13];
	char titulo[200];
	char descricao[300];
	char autores[100];
	char editora[50];
	char ano[4];
	char quant[3];
} Serv_livro;

Serv_livro livros[MAX];/*vetor contendo os livros*/
int TAM = 0;/*guarda o numero de livros*/
int col;
int main(void){
	int opcao;
	
	initiateServer();
	
}

void zerabuffer(char *buffer){
	int i;
	for(i=0;i<BUFSIZE;i++)
    buffer[i] = '\0';
}


int initiateServer() {

	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	char buf[MAXDATASIZE];
	int numbytes,i;
	FILE *fp;
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
	for(i=0;i<MAXDATASIZE;i++) 
		buf[i]='\0';
    int rv;
    int menu1;
    
	struct timeval {
		time_t      tv_sec;     /* seconds */
		suseconds_t tv_usec;    /* microseconds */
	};
	
	struct timeval *tv1, *tv2;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP
		//Faz a população do banco
		
  	//fp = fopen("bd.txt","w");
		//popula_banco(fp);
		//fclose(fp);
		//fp = fopen("bd.txt","r");
		//printf("entre com o menu\n");
		//scanf("%d", &menu1);
		//responde(menu1, fp, 2);
		//listaISBN(3);

	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
	return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
	if ((sockfd = socket(p->ai_family, p->ai_socktype,
	        p->ai_protocol)) == -1) {
	    perror("server: socket");
	    continue;
	}

	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
	        sizeof(int)) == -1) {
	    perror("setsockopt");
	    exit(1);
	}

	if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
	    close(sockfd);
	    perror("server: bind");
	    continue;
	}

	break;
    }

    if (p == NULL)  {
	fprintf(stderr, "server: failed to bind\n");
	return 2;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (listen(sockfd, BACKLOG) == -1) {
	perror("listen");
	exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
	perror("sigaction");
	exit(1);
    }

    printf("server: waiting for connections...\n");

    while(1) {  // main accept() loop
	sin_size = sizeof their_addr;
	new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
	if (new_fd == -1) {
	    perror("accept");
	    continue;
	}

	inet_ntop(their_addr.ss_family,
	    get_in_addr((struct sockaddr *)&their_addr),
	    s, sizeof s);
	printf("server: got connection from %s\n", s);

	//continuar fazendo recv

	 if (!fork()) { // this is the child process
            close(sockfd); // child doesn't need the listener

			int i;
			for(i = 0;i < 50; i++) {

			   readSocket(new_fd, buf);
			    
				tv1 = malloc(sizeof(struct timeval));
				tv2 = malloc(sizeof(struct timeval));

			   avaliaOpcao(buf, tv1, tv2);

				//printf("time1: %d\n", (*tv1).tv_usec);
				//printf("time2: %d\n", (*tv2).tv_usec);
				suseconds_t time = (*tv2).tv_usec - (*tv1).tv_usec;
				printf("i: %d\n", i);
				printf("timeTotal: %d\n",time);            
			    
			    writeSocket(new_fd, buf);
			}
            
            close(new_fd);
            exit(0);
        }
        close(new_fd);  // parent doesn't need this
    }


	return 0;
}

void avaliaOpcao(char *buf, struct timeval *tv1, struct timeval *tv2) {

	char opcao;
	char *isbn;
	char *qde;
	int qdet;
	int achou = 0;
	int i, j;
	FILE *bd;

	gettimeofday(tv1, NULL);
	
	opcao = buf[0];
	
	if (opcao == '2' || opcao == '3' || opcao == '5' || opcao == '6') {
		
		isbn = malloc(10*sizeof(char));
		
		for (i=0;i<10;i++) {
			if (buf[i+2] == '-')
				break;
			isbn[i] = buf[i+2];
		}
	}
	
	if (opcao == '5') {
	
		qde = malloc(10*sizeof(char));
		j = 0;
		for (i = 2; i < strlen(buf); i++) {
			if (achou == 0) {
				if (buf[i] != '-' && achou == 0)
					continue;
				else {
					achou = 1;
					continue;
				}
			}
			qde[j] = buf[i];
			j++;
		}
		
		qdet = atoi(qde);
	}
		bd= openBD();
		responde(buf, opcao, bd, qde,isbn);

		gettimeofday(tv2, NULL);
}


void readSocket(int socket, char* buf) {

	int bytes;

	if ((bytes = recv(socket, buf, MAXDATASIZE-1, 0)) == -1) {
		perror("recv");
		exit(1);
	}
	
	buf[bytes] = '\0';
}

void writeSocket(int socket, char* buf) {

	if (send(socket, buf, strlen(buf), 0) == -1)
                perror("send");

}


void responde(char *buf, int menu, FILE *bd, int quant,char *isbn){
  char parametro_id[2], c;
  int indice;
 // strcpy(parametro_id,buf+2);

int i = 0;
int files;
int count =1,mod;
  switch(menu){
   case 1:/*listar titulos e ISBN*/
		lista_titulo(buf, bd,quant);
		//closeBD(bd);
    break;   
  case 2:/*Descrição a partir do ISBN*/
		lista_menu(buf, bd,quant,isbn,3);
		//closeBD(bd);
    break;
  case 3:/*Todas as informações a partir do ISBN*/
		lista_menu(buf, bd,quant,isbn,0);
		//closeBD(bd);
    break;
  case 4:/*Lista todas as informações de todos os livros*/
   		lista_tudo(buf,bd,quant);
   		//closeBD(bd);
    break;
  case 5:/*Altera o número de exemplares em estoque*/
    		altera_bd(buf ,bd,quant,isbn);
    break;
  case 6:/*Número de exemplares em estoque a partir do ISBN*/
		lista_menu(buf, bd,quant,isbn,7);
		//closeBD(bd);
    break;
  case 7:/*encerra*/
    //strcpy(saida,"Encerra");
    break;

  }
	return;  

}

FILE* openBD() {

	FILE *fp;
	fp = fopen("bd.txt", "r");
	if (fp == NULL) {
		printf("Problema ao abrir o arquivo\n");
	} else {
		//printf("Arquivo aberto com sucesso\n");
		return fp;
	}
}

void closeBD(FILE *bd) {
	fseek(bd, 0, SEEK_SET);
}

void zeraBuffer(char *buf){
  int i;
  for(i=0;i<MAXDATASIZE;i++)
    buf[i] = '\0';
}

void imprime(int col, int lin, FILE *bd, char *buf){
	int indice;
	char c;
	int i , j, valida =0, k;
	int files;
	int count =0;
	char campo[300];
	char linha[500];	
	char *result;
	for(i=0;i<300;i++) {
		campo[i]	=' ';	
		}
	for(i=0;i<500;i++) {
		linha[i]	=' ';
		}

		for (i = 1; i<=lin; i++) 
			fgets(linha, 500, bd);						
		for (j=0,k=0;j<100&&(count != col);j++){
			if (linha[j] != ';'){
				campo[k++] = linha[j];
			}else{
				count++;
				k=0;
			}
		}
		if (col== 1) {
			strcat(buf, "-");
			strcat(buf, campo);
			//printf("ISBN: %s\n", campo);
			}
		
		if (col == 2) {
			strcat(buf, "-");
			strcat(buf, campo);
			//printf("Título: %s\n",campo);
			}
		if (col == 3) {
			strcat(buf, "-");
			strcat(buf, campo);
			//printf("Descricao: %s\n",campo);
			}
		if (col == 4) {
			strcat(buf, "-");
			strcat(buf, campo);
			//printf("Autores: %s\n",campo);
			}
		if (col == 5) {
			strcat(buf, "-");
			strcat(buf, campo);
			//printf("Editora: %s\n",campo);
			}
		if (col == 6) {
			strcat(buf, "-");
			strcat(buf, campo);
			//printf("Ano: %s\n",campo);
			}
		if (col == 7) {
			strcat(buf, "-");
			strcat(buf, campo);
			//printf("Quantidade: %s\n",campo);
			}
		closeBD(bd);
}

void lista_titulo(char *buf, FILE *bd, int quant){
  int indice;
    //strcpy(parametro_id,buffer+2);
	char c;
	int i = 0;
	int files;
	int count =1;
	int j;
	for (j=0;j<quant;j++){//para todas as linhas
		imprime(1, j, bd, buf);
		imprime(2, j, bd, buf);
	}
}

void lista_menu(char *buf, FILE *bd, int quant, char ISBN[], int ret){

  int indice;
    //strcpy(parametro_id,buffer+2);
	char c;
	int i,j,k, valida =0;
	int files;
	int count =0;
	int mod;
	char linha[100];
	char *result;	
	fseek(bd, 0, SEEK_SET);

		//while (!feof(bd)){//enquanto tiver arquivo e nao achar o isbn
			for (j=0;j<quant;j++){//para todas as linhas
				for (i=0;i<100 && c!='\n';i++){//percorre a linha inteira
						c = getc(bd);//le o caractere e coloca em linha[]
						linha[i] = c;
				}
				for (k=0;k<2 && count!=2;k++){//para o tamanho do ISBN
						if (ISBN[k]==linha[k]){//se for igual ao procurado
							valida =1;
							count++;
						}else{
							valida = 0;
							break;
						}
				}
			}
		//}

			if (valida==1){
				if (ret == 3) {
					imprime(1,j,bd,buf);
					imprime(3,j,bd,buf);
					}
				if (ret == 7) {
					imprime(1,j,bd, buf);
					imprime(7,j,bd,buf);
					}
				if (ret == 0){
					imprime(1,j,bd, buf);
					imprime(2,j,bd,buf);
					imprime(3,j,bd, buf);
					imprime(4,j,bd, buf);
					imprime(5,j,bd, buf);
					imprime(6,j,bd, buf);
					imprime(7,j,bd, buf);
				}
			}else{
					printf("Não achou ISBN");
			}
}

/*funcao para insercao de um novo filme no banco*/
void popula_banco(FILE *fp){
int i;
  if (!fp){
  	printf("Erro na abertura do arquivo");
		exit(0);
  }else{
		for(i=0;i<2;i++){
			
			fprintf(stderr, "%s","entre com ISBN\n"); 
			scanf(" %[^\n]", livros[TAM].isbn);
			fprintf(fp, "%s;",livros[TAM].isbn);

			fprintf(stderr, "%s","entre com titulo\n"); 
			scanf(" %[^\n]", livros[TAM].titulo);
			fprintf(fp, "%s;",livros[TAM].titulo);

			fprintf(stderr, "%s","entre com descricao\n"); 
			scanf(" %[^\n]", livros[TAM].descricao);
			fprintf(fp, "%s;",livros[TAM].descricao);  

			fprintf(stderr, "%s","entre com autores\n"); 
			scanf(" %[^\n]", livros[TAM].autores);
			fprintf(fp, "%s;",livros[TAM].autores);

			fprintf(stderr, "%s","entre com editora\n"); 
			scanf(" %[^\n]", livros[TAM].editora);
			fprintf(fp, "%s;",livros[TAM].editora);

			fprintf(stderr, "%s","entre com ano\n"); 
			scanf(" %[^\n]", livros[TAM].ano);
			fprintf(fp, "%s;",livros[TAM].ano);

			fprintf(stderr, "%s","entre com a quantidade em estoque\n"); 
			scanf(" %[^\n]", livros[TAM].quant);
			fprintf(fp, "%s\n",livros[TAM].quant);
			TAM++; 
		}
	}
	  
 return;
}

void lista_tudo(char *buf, FILE *bd, int quant){
  int indice;
    //strcpy(parametro_id,buffer+2);
	char c;
	int i,j,k, valida =0;
	int files;
	int count =0,mod;
	char linha[100];
	char *result;	
	fseek(bd, 0, SEEK_SET);
	//while (!feof(bd)){//enquanto tiver arquivo e nao achar o isbn
		for (j=0;j<quant;j++){//para todas as linhas
			for (i=0;i<100 && c!='\n';i++){//percorre a linha inteira
				c = getc(bd);//le o caractere e coloca em linha[]
				linha[i] = c;
			}
				imprime(1,j,bd,buf);
				imprime(2,j,bd,buf);
				imprime(3,j,bd,buf);
				imprime(4,j,bd,buf);
				imprime(5,j,bd,buf);
				imprime(6,j,bd,buf);
				imprime(7,j,bd,buf);
			}
	//}
			
			
}

void altera_bd(char *buf, FILE *bd, int quant, char ISBN[]){

	}
