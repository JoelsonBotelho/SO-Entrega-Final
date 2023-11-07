#include "commandlinereader.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>

#define COMANDO_DEBITAR "debitar"
#define COMANDO_CREDITAR "creditar"
#define COMANDO_LER_SALDO "lerSaldo"
#define COMANDO_SIMULAR "simular"
#define COMANDO_SAIR "sair"
#define COMANDO_SAIR_AGORA "sair agora"
#define COMANDO_TRANSFERIR "transferir"
#define COMANDO_SAIR_TERMINAL "sair-terminal"

#define OPERACAO_DEBITAR 1
#define OPERACAO_CREDITAR 2
#define OPERACAO_LER_SALDO 3
#define OPERACAO_SAIR 4
#define OPERACAO_TRANSFERIR 5
#define OPERACAO_SAIR_AGORA 6
#define OPERACAO_SIMULAR 7
#define OPERACAO_LIGAR_PIPE 8

#define MAXARGS 4
#define BUFFER_SIZE 100

void tratarSignalPipe(int signal);
int ler_pipeIn(char pipeInName[]);
void sair_terminal();

typedef struct {
  int operacao;
  int idConta;
  int idConta2;
  int valor;
  int numAnos;
  int pipeID;
} pedido_t;
char pipeInName[50];
int pipeOut;

int main(int argc, char** argv) {
    
    char *args[MAXARGS + 1];
    char buffer[BUFFER_SIZE];
    int pid;
    
    clock_t begin, end;

    signal(SIGPIPE, tratarSignalPipe);

    pipeOut = open(argv[1], O_WRONLY);
    if(pipeOut < 0) {
    	fprintf(stderr, "Erro a abrir banco-pipe\n");
    	exit(-1);
    }

    pid = getpid();

    sprintf(pipeInName, "pipe-%d", pid);
    unlink(pipeInName);
  
  	if (mkfifo(pipeInName, 0666) < 0) {
    	fprintf(stderr, "Erro a criar o pipe.\n");
    	exit(-1);
  	}
      
    pedido_t pedido;
   
    while(1) {
        
        int numargs, res;
        double time;
        numargs = readLineArguments(args, MAXARGS + 1, buffer, BUFFER_SIZE);
        
        if (numargs == 0)
          /* Nenhum argumento; ignora e volta a pedir */
          continue;
    
        /* Debitar */
        else if (strcmp(args[0], COMANDO_DEBITAR) == 0) {
          if (numargs < 3) {
            printf("%s: Sintaxe inválida, tente de novo.\n", COMANDO_DEBITAR);
            continue;
          }
    
          pedido.operacao = OPERACAO_DEBITAR;
          pedido.idConta = atoi(args[1]);
          pedido.valor = atoi(args[2]);
          pedido.pipeID = pid;
    	  
          begin = clock();
	      write(pipeOut, &pedido, sizeof(pedido_t));
          res = ler_pipeIn(pipeInName);
          end = clock();
          time = (double)(end-begin)/CLOCKS_PER_SEC;
          if(res == -1)
         	printf("%s(%d, %d): Erro.\n", COMANDO_DEBITAR, pedido.idConta, pedido.valor);
          else 
          	printf("%s(%d, %d): OK\n", COMANDO_DEBITAR, pedido.idConta, pedido.valor);
          printf("Tempo de execução: %lf\n\n", time);
        }
    
        
        else if (strcmp(args[0], COMANDO_CREDITAR) == 0) {
    
          if (numargs < 3) {
            printf("%s: Sintaxe inválida, tente de novo.\n", COMANDO_CREDITAR);
            continue;
          }
    
          pedido.operacao = OPERACAO_CREDITAR;
          pedido.idConta = atoi(args[1]);
          pedido.valor = atoi(args[2]);
          pedido.pipeID = pid;
    	  
          begin = clock();
	      write(pipeOut, &pedido, sizeof(pedido_t));
          res = ler_pipeIn(pipeInName);
          end = clock();
          time = (double)(end-begin)/CLOCKS_PER_SEC;
          if(res == -1)
         	printf("%s(%d, %d): Erro.\n", COMANDO_CREDITAR, pedido.idConta, pedido.valor);
          else 
          	printf("%s(%d, %d): OK\n", COMANDO_CREDITAR, pedido.idConta, pedido.valor);
          printf("Tempo de execução: %lf\n\n", time);
        }
    
        /* Transferir */
        else if (strcmp(args[0], COMANDO_TRANSFERIR) == 0) {
    
          if (numargs < 4 || atoi(args[1]) == atoi(args[2])) {
            printf("%s: Sintaxe inválida, tente de novo.\n", COMANDO_TRANSFERIR);
            continue;
          }
    
          pedido.operacao = OPERACAO_TRANSFERIR;
          pedido.idConta = atoi(args[1]);
          pedido.idConta2 = atoi(args[2]);
          pedido.valor = atoi(args[3]);
          pedido.pipeID = pid;
    	  
          begin = clock();
	      write(pipeOut, &pedido, sizeof(pedido_t));
          res = ler_pipeIn(pipeInName);
          end = clock();
          time = (double)(end-begin)/CLOCKS_PER_SEC;
          if(res == -1)
         	printf("Erro ao transferir %d da conta %d para a conta %d\n\n", pedido.valor, 
         		pedido.idConta, pedido.idConta2);
          else 
          	printf("%s(%d, %d, %d): OK\n\n", COMANDO_TRANSFERIR, 
              pedido.idConta, pedido.idConta2, pedido.valor);
          printf("Tempo de execução: %lf\n\n", time);
        }
    
        /* Ler Saldo */
        else if (strcmp(args[0], COMANDO_LER_SALDO) == 0) {
    
          if (numargs < 2) {
            printf("%s: Sintaxe inválida, tente de novo.\n", COMANDO_LER_SALDO);
            continue;
          }
    
          pedido.operacao = OPERACAO_LER_SALDO;
          pedido.idConta = atoi(args[1]);
          pedido.valor = 0;
    
          pedido.pipeID = pid;
    	  
          begin = clock();
	      write(pipeOut, &pedido, sizeof(pedido_t));
          res = ler_pipeIn(pipeInName);
          end = clock();
          time = (double)(end-begin)/CLOCKS_PER_SEC;
          if(res == -1)
         	printf("%s(%d): Erro.\n\n", COMANDO_LER_SALDO, pedido.idConta);
          else 
          	printf("%s(%d): O saldo da conta é %d.\n\n", COMANDO_LER_SALDO, 
          		pedido.idConta, res);
          printf("Tempo de execução: %lf\n\n", time);
        }
    
        /* Simular */
        else if (strcmp(args[0], COMANDO_SIMULAR) == 0) {
          int numAnos;
    
          if (numargs < 2) {
            printf("%s: Sintaxe inválida, tente de novo.\n", COMANDO_SIMULAR);
            continue;
          }
          numAnos = atoi(args[1]);
          pedido.operacao = OPERACAO_SIMULAR;
          pedido.numAnos = numAnos;
          write(pipeOut, &pedido, sizeof(pedido_t)); 
          }
    
        /* Sair */
        else if (numargs > 0 && (strcmp(args[0], COMANDO_SAIR) == 0)) {
           	if (numargs == 1 || numargs < 0) {
           		pedido.operacao = OPERACAO_SAIR;
           		write(pipeOut, &pedido, sizeof(pedido_t));
           		continue;
           	}

           	else if (numargs == 2 && strcmp(args[1], "agora") == 0) {
           		pedido.operacao = OPERACAO_SAIR_AGORA;
           		write(pipeOut, &pedido, sizeof(pedido_t));
           		continue;
           	}

           	else {
		        printf("%s: Sintaxe inválida, tente de novo.\n", COMANDO_SAIR);
        		continue;
      		}
      	}

      	/* Sair Terminal */
      	else if (numargs < 0 ||
           (numargs > 0 && (strcmp(args[0], COMANDO_SAIR_TERMINAL) == 0))) {
      		sair_terminal();
      	}

        
        else {
          printf("Comando desconhecido. Tente de novo.\n");
          continue;
        }
      }
    
    return 0;
}

void tratarSignalPipe(int signal) {
	close(pipeOut);
    unlink(pipeInName);
    exit(EXIT_SUCCESS);
} 

int ler_pipeIn(char pipeInName[]) {
	int fd, res;
	fd = open(pipeInName, O_RDONLY);
	if (fd == -1)
		fprintf(stderr, "Erro a abrir client-pipe\n");
	read(fd, &res, sizeof(res));
	return res;
}

void sair_terminal() {
	close(pipeOut);
    unlink(pipeInName);
    exit(EXIT_SUCCESS);
}