#include "commandlinereader.h"
#include "contas.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>

#define COMANDO_DEBITAR "debitar"
#define COMANDO_CREDITAR "creditar"
#define COMANDO_LER_SALDO "lerSaldo"
#define COMANDO_SIMULAR "simular"
#define COMANDO_SAIR "sair"
#define COMANDO_TRANSFERIR "transferir"

#define OPERACAO_DEBITAR 1
#define OPERACAO_CREDITAR 2
#define OPERACAO_LER_SALDO 3
#define OPERACAO_SAIR 4
#define OPERACAO_TRANSFERIR 5
#define OPERACAO_SAIR_AGORA 6
#define OPERACAO_SIMULAR 7
#define OPERACAO_LIGAR_PIPE 8

#define FALSE 0
#define TRUE 1

#define MAXFILHOS 16
#define MAXARGS 4
#define BUFFER_SIZE 100
#define NUM_TRABALHADORAS 3
#define PEDIDO_BUFFER_DIM (NUM_TRABALHADORAS * 2)
#define PIPENAME "banco-pipe"

void tratarSignalPipe(int signal) {}


typedef struct {
  int operacao;
  int idConta;
  int idConta2;
  int valor;
  int numAnos;
  int pipeID;
} pedido_t;


void colocarPedido(pedido_t pedido);
pedido_t obterPedido();
void *threadFunc();

int posColocar = 0;
int posObter = 0;
int countBuffer = 0;
int client;

char filename[50];
char pipeNumber[50]; 

pedido_t pedido_buffer[PEDIDO_BUFFER_DIM];
pthread_t threads[NUM_TRABALHADORAS];
sem_t semObter, semColocar;
pthread_mutex_t mutex;
pthread_mutex_t mutexSimular;
pthread_cond_t podeSimular;

int main(int argc, char** argv) {

  int numFilhos = 0;
  int pidTable[MAXFILHOS];
  int posPid;
  int i; 
  int pipe;
  
  pedido_t pedido;

  signal(SIGUSR1, tratarSignal);
  signal(SIGPIPE, tratarSignalPipe);

  inicializarContas();

  /* Inicializar semaforos */
  if (sem_init(&semObter, 0, 0) != 0) {
    fprintf(stderr, "Erro a criar semaforo\n");
    exit(EXIT_FAILURE);
  }
  if (sem_init(&semColocar, 0, PEDIDO_BUFFER_DIM) != 0) {
    fprintf(stderr, "Erro a criar semaforo\n");
    exit(EXIT_FAILURE);
  }
  /* Inicializar threads */
  for (i = 0; i < NUM_TRABALHADORAS; i++)
    if (pthread_create(&threads[i], NULL, &threadFunc, NULL) != 0) {
      fprintf(stderr, "Erro a criar thread\n");
      exit(EXIT_FAILURE);
    }

  /* Inicializar mutexes banco.c */
  if(pthread_mutex_init(&mutex, NULL) != 0) {
  	fprintf(stderr, "Erro a inicializar mutex\n");
  	exit(EXIT_FAILURE);
  }
  if (pthread_mutex_init(&mutexSimular, NULL) != 0) {
  	fprintf(stderr, "Erro a inicializar mutex\n");
  	exit(EXIT_FAILURE);
  }

  /* Inicializar mutex contas.c */
  mutex_contas_init();

  /* Inicializar cond_t */
  if (pthread_cond_init(&podeSimular, NULL) != 0) {
    fprintf(stderr, "Erro a incializar cond\n");
    exit(EXIT_FAILURE);
  }

  abrir_ficheiro(); /*Abre o ficheiro log.txt*/
  
  unlink(PIPENAME);
  
  if (mkfifo(PIPENAME, 0666) < 0) {
    fprintf(stderr, "Erro a criar o pipe.\n");
    exit(-1);
  }
  
  pipe = open(PIPENAME, O_RDONLY);
  if (pipe == -1) {
    fprintf(stderr, "Erro a abrir pipe.\n");
    exit(-1);
  }
  
//Inicia Banco Comercila

  printf("Bem-vinda/o ao Banco Comercial\n\n");

  while (1) {
    int status;

    read(pipe, &pedido, sizeof(pedido_t));

    /* EOF (end of file) do stdin ou comando "sair" */
    if (pedido.operacao == OPERACAO_SAIR) {
      int filho, pid;
      /* Sair normalmente */
      for (i = 0; i < NUM_TRABALHADORAS; i++)
        colocarPedido(pedido);
      printf("O Banco Comercial vai terminar.\n--\n");
      for (filho = 0; filho < numFilhos; filho++) {
        pid = wait(&status);
        printf("FILHO TERMINADO (PID=%d; terminou %s)\n", pid,
          WIFSIGNALED(status) ? "abruptamente" : "normalmente");
      }
      for (i = 0; i < NUM_TRABALHADORAS; i++) {
        if (pthread_join(threads[i], NULL) != 0)
          fprintf(stderr, "Erro a sair da thread\n");
        }
        printf("--\nBanco Comercial terminou.\n");
        close(pipe);
        unlink(PIPENAME);
      	exit(EXIT_SUCCESS);
      }

      /* Sair Agora */
      else if (pedido.operacao == OPERACAO_SAIR_AGORA) {
        int filho, pid;
        pedido.operacao = OPERACAO_SAIR;
        for(i = 0; i < NUM_TRABALHADORAS; i++)
          colocarPedido(pedido);
        printf("Banco Comercial vai terminar.\n--\n");
        for (posPid = 0; posPid < numFilhos; posPid++) {
          kill(pidTable[posPid], SIGUSR1);
        }
        for (filho = 0; filho < numFilhos; filho++) {
          pid = wait(&status);
          printf("Simulacao terminada por signal\n");
          printf("FILHO TERMINADO (PID=%d; terminou %s)\n", pid,
            WIFSIGNALED(status) ? "abruptamente" : "normalmente");
        }
        for (i = 0; i < NUM_TRABALHADORAS; i++) {
          if (pthread_join(threads[i], NULL) != 0)
            fprintf(stderr, "Erro a sair da thread\n");
        }
      
      	printf("--\nBanco Comercial terminou.\n");
      	close(pipe);
      	unlink(PIPENAME);
      	exit(EXIT_SUCCESS);
 	}

    /* Simular */
    else if (pedido.operacao == OPERACAO_SIMULAR) {
      int numAnos, pid, file_simular;

      numAnos = pedido.numAnos;

      if (numFilhos < MAXFILHOS){
        if(pthread_mutex_lock(&mutexSimular) != 0)
          fprintf(stderr, "Erro a fechar o mutex");
        while(countBuffer > 0) {
          pthread_cond_wait(&podeSimular, &mutexSimular);
        }
        pid = fork();

        if (pid == 0) {
          sprintf(filename, "banco-sim-%d.txt", getpid());
          file_simular = open(filename, O_CREAT | O_WRONLY, 0666);
          setFileDescriptor(-1);
          if(file_simular == -1) {
            fprintf(stderr, "Erro a abrir o ficheiro");
            close(file_simular);
            exit(-1);
          }
          if (dup2(file_simular, 1) == 0)
            fprintf(stderr, "Erro a redirecionar output\n");
          close(file_simular);
          simular(numAnos);
          exit(0);
        }
        else if (pid == -1) {
          fprintf(stderr, "Erro no fork\n");
          continue;
        }
        else {
          pidTable[numFilhos++] = pid;
          if(pthread_mutex_unlock(&mutexSimular) != 0)
            fprintf(stderr, "Erro a abrir mutex");
          continue;
        }
      }
    }

    /* Caso nenhum comando funcione */
    else {
      colocarPedido(pedido);
      continue;
    }
    close(client);
  }
}

void colocarPedido(pedido_t pedido) {
  if(sem_wait(&semColocar) != 0) {
    fprintf(stderr, "Erro espera semaforo");
  	exit(EXIT_FAILURE);
  }
  if(pthread_mutex_lock(&mutex) != 0) {
    fprintf(stderr, "Erro fechar mutex");
    exit(EXIT_FAILURE);
  }
  pedido_buffer[posColocar] = pedido;
  posColocar = (posColocar + 1) % PEDIDO_BUFFER_DIM;
  countBuffer++;
  if(pthread_mutex_unlock(&mutex) != 0) {
    fprintf(stderr, "Erro abrir mutex");
    exit(EXIT_FAILURE);
  }
  if(sem_post(&semObter) != 0) {
    fprintf(stderr, "Erro assinalar semaforo");
    exit(EXIT_FAILURE);
  }
}

pedido_t obterPedido() {
  pedido_t pedido;
  if(sem_wait(&semObter) != 0) {
    fprintf(stderr, "Erro espera semaforo");
    exit(EXIT_FAILURE);
  }
  if(pthread_mutex_lock(&mutex) != 0) {
    fprintf(stderr, "Erro fechar mutex");
  	exit(EXIT_FAILURE);
  }
  pedido = pedido_buffer[posObter];
  posObter = (posObter + 1) % PEDIDO_BUFFER_DIM;
  if(pthread_mutex_unlock(&mutex) != 0) {
    fprintf(stderr, "Erro abrir mutex");
    exit(EXIT_FAILURE);
  }
  if(sem_post(&semColocar) != 0) {
    fprintf(stderr, "Erro assinalar semaforo");
    exit(EXIT_FAILURE);
  }

  return pedido;
}

void *threadFunc() {
  int res, client;

  while(TRUE) {
    pedido_t p = obterPedido();

    sprintf(pipeNumber, "pipe-%d", p.pipeID);
  	client = open(pipeNumber, O_WRONLY);

    if (p.operacao == OPERACAO_DEBITAR) {
      res = debitar(p.idConta, p.valor);
      write(client, &res, sizeof(res));
    }

    else if (p.operacao == OPERACAO_CREDITAR) {
      res = creditar(p.idConta, p.valor);
      write(client, &res, sizeof(res));
   	}

    else if (p.operacao == OPERACAO_LER_SALDO) {
      res = lerSaldo(p.idConta);
      write(client, &res, sizeof(res));
    }

    else if (p.operacao == OPERACAO_TRANSFERIR) {
      res = transferir(p.idConta, p.idConta2, p.valor);
      write(client, &res, sizeof(res));
    }

    else if (p.operacao == OPERACAO_SAIR)
      return NULL;
    
    close(client);    
    
    if(pthread_mutex_lock(&mutexSimular) != 0)
      fprintf(stderr, "Erro fechar mutex");
    countBuffer--;
    if(countBuffer == 0)
      if(pthread_cond_signal(&podeSimular) != 0)
      fprintf(stderr, "Erro a assinalar variavel podeSimular");
    if(pthread_mutex_unlock(&mutexSimular) != 0)
      fprintf(stderr, "Erro abrir mutex");
  }
}

