/*
*  rrprio.c - Implementacao do algoritmo Round Robin com Prioridades e sua API
*
*  Autores: SUPER_PROGRAMADORES_C
*  Projeto: Trabalho Pratico I - Sistemas Operacionais
*  Organizacao: Universidade Federal de Juiz de Fora
*  Departamento: Dep. Ciencia da Computacao
*
*
*/

#include "rrprio.h"
#include <stdio.h>
#include <string.h>

//Nome unico do algoritmo. Deve ter 6 caracteres no máximo.
const char rrpName[]="RRPrio";

//=====Funcoes Auxiliares=====
//...




//=====Funcoes da API=====

//Inicializa os parametros de escalonamento de um processo p. Funcao chamada 
//normalmente quando o processo deve ser associado ao algoritmo RRPrio
void rrpInitSchedParams(Process *p, void *rrparams) {
	//...
}

//Retorna o proximo processo a obter a CPU, conforme o algortimo RRPrio 
Process* rrpSchedule(Process *plist) {
	//...
	return NULL;
}

//Libera os parametros de escalonamento de um processo p. Funcao chamada 
//normalmente quando o processo e' desassociado do slot de RRPrio
//Retorna o numero do slot ao qual o processo estava associado
int rrpReleaseParams(Process *p) {
	//...
	return -1;
}

//Modifica a prioridade atual de um processo
//E' a funcao de setSomeFeatureFn() de RRPrio
void rrpSetPrio(Process *p, void *rrparams) {
	//...
}

//Notifica a mudanca de status de um processo para possivel manutencao de dados
//internos ao algoritmo RRPrio, responsavel pelo processo
void rrpNotifyProcessStatus(Process *p, int oldstatus) {
	//...
}

//Funcao chamada pela inicializacao do S.O. para a incializacao do escalonador
//conforme o algoritmo Round Robin com Prioridades
//Deve envolver a inicializacao de possiveis parametros gerais
//Deve envolver o registro do algoritmo junto ao escalonador
//Retorna o numero do slot obtido no registro do algoritmo junto ao escalonador
int rrpInitSchedInfo() {
	//...
	return -1;
}
