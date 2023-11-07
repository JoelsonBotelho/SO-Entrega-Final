#include "contas.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define atrasar() sleep(ATRASO)
#define COMANDO_DEBITAR "debitar"
#define COMANDO_CREDITAR "creditar"
#define COMANDO_LER_SALDO "lerSaldo"
#define COMANDO_SIMULAR "simular"
#define COMANDO_SAIR "sair"
#define COMANDO_TRANSFERIR "transferir"
#define FILENAME "log.txt"

int contasSaldos[NUM_CONTAS];
int flagSair = 0;
pthread_mutex_t mutexC[NUM_CONTAS];
int ficheiro;
char message[50];

void mutex_contas_init() {
   int i;
   for (i = 0; i < NUM_CONTAS; i++)
      if (pthread_mutex_init(&mutexC[i], NULL) != 0)
         fprintf(stderr, "Erro a inicializar mutex\n");
}

void abrir_ficheiro(){
   ficheiro = open(FILENAME, O_CREAT | O_TRUNC | O_WRONLY, 0666);
   if (ficheiro == -1){
      fprintf(stderr, "Erro a abrir ficheiro\n");
      exit(-1);
   }
}

int contaExiste(int idConta) {
   return (idConta > 0 && idConta <= NUM_CONTAS);
}

void inicializarContas() {
   int i;
   for (i = 0; i < NUM_CONTAS; i++)
      contasSaldos[i] = 0;
}

void lock_conta(int idConta) {
   if (pthread_mutex_lock(&mutexC[idConta - 1]) != 0) {
      fprintf(stderr, "Erro a fechar mutex\n");
      exit(EXIT_FAILURE);
   }
}

void unlock_conta(int idConta) {
   if (pthread_mutex_unlock(&mutexC[idConta - 1]) != 0) {
      fprintf(stderr, "Erro a abrir mutex\n");
      exit(EXIT_FAILURE);
   }
}

void setFileDescriptor(int valor) {
   ficheiro = valor;
}

int debitar(int idConta, int valor) {
   atrasar();
   lock_conta(idConta);
   if (!contaExiste(idConta)){
      sprintf(message, "%lu: %s(%d, %d): Erro.\n\n", pthread_self(), COMANDO_DEBITAR, idConta, valor);
      write(ficheiro, &message, strlen(message));
      unlock_conta(idConta);
      return -1; 
   }
   if (contasSaldos[idConta - 1] < valor) {
      sprintf(message, "%lu: %s(%d, %d): Erro.\n\n", pthread_self(), COMANDO_DEBITAR, idConta, valor);
      write(ficheiro, &message, strlen(message));
      unlock_conta(idConta);
      return -1;
   }
   atrasar();
   contasSaldos[idConta - 1] -= valor;
   sprintf(message, "%lu: %s(%d, %d): OK.\n\n", pthread_self(), COMANDO_DEBITAR, idConta, valor);
   write(ficheiro, &message, strlen(message));
   unlock_conta(idConta);
   return 0;
}

int creditar(int idConta, int valor) {
   atrasar();
   lock_conta(idConta);
   if (!contaExiste(idConta)) {
      sprintf(message, "%lu: %s(%d, %d): Erro.\n\n", pthread_self(), COMANDO_CREDITAR, idConta, valor);
      write(ficheiro, &message, strlen(message));
      unlock_conta(idConta);
      return -1;
   }
   contasSaldos[idConta - 1] += valor;
   sprintf(message, "%lu: %s(%d, %d): OK.\n\n", pthread_self(), COMANDO_CREDITAR, idConta, valor);
   write(ficheiro, &message, strlen(message));
   unlock_conta(idConta);
   return 0;
}

int lerSaldo(int idConta) {
   int saldo;
   atrasar();
   lock_conta(idConta);
   if (!contaExiste(idConta)) {
      sprintf(message, "%lu: %s(%d): Erro.\n\n", pthread_self(), COMANDO_LER_SALDO, idConta);
      write(ficheiro, &message, strlen(message));
      unlock_conta(idConta);
      return -1;
   }
   saldo = contasSaldos[idConta - 1];
   sprintf(message, "%lu: %s(%d): O saldo da conta é %d.\n\n", pthread_self(), COMANDO_LER_SALDO, idConta, saldo);
   write(ficheiro, &message, strlen(message));
   unlock_conta(idConta);
   return saldo;
}

void simular(int numAnos) {
   int anos, conta, saldo_novo;

   for (anos = 0; anos <= numAnos; anos++) {
      printf("SIMULACAO: Ano %d\n", anos);
      printf("=================\n");
      for (conta = 1; conta <= NUM_CONTAS; conta++) {
         saldo_novo = 0;
         if (anos > 0) {
            saldo_novo = (lerSaldo(conta) * (1 + TAXAJURO) - CUSTOMANUTENCAO);
            if (saldo_novo <= 0)
               debitar(conta, lerSaldo(conta));
            else
               creditar(conta, saldo_novo - lerSaldo(conta));
         }
         printf("Conta %d, Saldo %d\n", conta, lerSaldo(conta));
      }
      printf("\n");
      if (flagSair == 1)
         return;
   }
}

void tratarSignal(int signal) {
   flagSair = 1;
}

int transferir(int idConta1, int idConta2, int valor) {
   int idConta_menor, idConta_maior;
   atrasar();

   if(idConta1 < idConta2){
      idConta_menor = idConta1;
      idConta_maior = idConta2;
    }
   else{
     idConta_menor = idConta2;
     idConta_maior = idConta1;
   }

   lock_conta(idConta_menor);
   lock_conta(idConta_maior);

   if (!contaExiste(idConta_menor) || !contaExiste(idConta_maior)) {
      printf(message, "%lu: Erro ao transferir %d da conta %d para a conta %d\n", pthread_self(),valor, idConta1, idConta2);
      write(ficheiro, &message, strlen(message));
      return -1;
   }

   /* Debitar */
   if (contasSaldos[idConta1 - 1] < valor){
      printf(message, "%lu: Erro ao transferir %d da conta %d para a conta %d\n", pthread_self(),valor, idConta1, idConta2);
      write(ficheiro, &message, strlen(message));
      unlock_conta(idConta_maior);
      unlock_conta(idConta_menor);
      return -1;
   }
   contasSaldos[idConta1 - 1] -= valor;

   /* Creditar */
   contasSaldos[idConta2 - 1] += valor;
   sprintf(message, "%lu: %s(%d, %d, %d): OK\n", pthread_self(), COMANDO_TRANSFERIR, 
      idConta1, idConta2, valor);
   write(ficheiro, &message, strlen(message));
   unlock_conta(idConta_maior);
   unlock_conta(idConta_menor);
   return 0;
}
