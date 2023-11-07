#ifndef CONTAS_H
#define CONTAS_H

#define NUM_CONTAS 5
#define TAXAJURO 0.1
#define CUSTOMANUTENCAO 1

#define ATRASO 1

void inicializarContas();
int contaExiste(int idConta);
int debitar(int idConta, int valor);
int creditar(int idConta, int valor);
int lerSaldo(int idConta);
void simular(int numAnos);
void tratarSignal(int signal);
void mutex_contas_init();
int transferir(int idConta1, int idConta2, int valor);
void abrir_ficheiro();
void setFileDescriptor(int valor);

#endif
