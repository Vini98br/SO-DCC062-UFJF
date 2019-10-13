/*
*  rrprio.c - Implementacao do algoritmo Round Robin com Prioridades e sua API
*
*  Autores: Vinicius da Cruz Soranço
            Joao Cotta Badaro
            Arthur de Freitas Dornelas
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
int RRPrioSchedSlot = -1;

//=====Funcoes Auxiliares=====

typedef struct process_node
{
    Process *process;
    struct process_node *next;
} ProcessNode;

typedef struct process_queue
{
    ProcessNode *first;
    ProcessNode *last;
} ProcessQueue;

// Cada elemento do vetor representa uma fila de processos daquela prioridade
ProcessQueue priorityQueues[5];
int iterationsOnCPU = 0;
Process *currentProcess = NULL;

void enqueueProcessNode(ProcessQueue *processQueue, ProcessNode *processNode) {
    if (processQueue->first==NULL) {
        processQueue->first = processNode;
        processQueue->last = processNode;
    }
    else {
        processQueue->last->next = processNode;
        processQueue->last = processNode;
    }
}

void enqueueProcess(ProcessQueue *processQueue, Process *process) {
    ProcessNode *processNode = (ProcessNode*)malloc(sizeof(ProcessNode));
    processNode->process = process;
    processNode->next = NULL;

    enqueueProcessNode(processQueue, processNode);
}

ProcessNode* dequeueProcessNode(ProcessQueue *processQueue)//desenfileira processo da fila de prioridade
{
    ProcessNode *processNode = processQueue->first;

    if (processNode!=NULL) {
        processQueue->first = processNode->next;
        processNode->next = NULL;

        if (processNode==processQueue->last) {
            processQueue->last = NULL;
        }
    }

    return processNode;
}

ProcessNode* requeueProcessNode(ProcessQueue *processQueue) {
    ProcessNode *processNode = dequeueProcessNode(processQueue);
    enqueueProcessNode(processQueue, processNode);

    return processNode;
}

// Esta operação quebra as regras de pilha
int removeProcess(ProcessQueue *processQueue, Process *process) {
    ProcessNode *processNodeIt = processQueue->first;
    ProcessNode *prevProcessNodeIt = NULL;
	int cont = 0;
    while(processNodeIt!=NULL) {
        if (processNodeIt->process==process)//percorre toda a fila de prioridade ate encontrar o processo em questao
            break;//caso o processo seja encontrado, sair do loop
		cont++;//calcula a posiçao deste processo
        prevProcessNodeIt = processNodeIt;//atribui o atual como sendo o anterior
        processNodeIt = processNodeIt->next;//pega o proximo processo da lista
    }

    if (processNodeIt!=NULL) {
        if (processNodeIt==processQueue->first)//se o processo a ser removido eh o primeiro da fila  
		{                                      //de prioridade , pega-se o proximo processo da fila.   
            processQueue->first = processNodeIt->next;
        }
        else//senao aloca-se o proximo processo no lugar do que vai ser removido
        {
            prevProcessNodeIt->next = processNodeIt->next;
        }

        if (processNodeIt->next == NULL)//se o proximo a ser removido esriver no final da fila 
            processQueue->last = prevProcessNodeIt;//last recebe entao a posicao anterior do processo a ser removido

        if (processNodeIt->process == currentProcess)//se o processo a ser removido eh o processo atual
            currentProcess = NULL;//atribui-se null ao processo atual

        free(processNodeIt);
    }
	return cont;//retorna a posicao deste processo
}




//=====Funcoes da API=====

//Inicializa os parametros de escalonamento de um processo p. Funcao chamada 
//normalmente quando o processo deve ser associado ao algoritmo RRPrio
void rrpInitSchedParams(Process *p, void *rrparams) { // ( feito - Vinicius)
	RRPSchedParams *rrpSchedParams = (RRPSchedParams*)rrparams;
	processSetSchedParams(p,rrparams);//Redireciona ponteiro de parametros de escalonamento para uma estrutura especifica.
	processSetSchedSlot(p,RRPrioSchedSlot);//Modifica o algoritmo de escalonamento associado, por meio do numero do slot registrado para o algoritmo.
	enqueueProcess(priorityQueues + rrpSchedParams->priority,p);//enfileira esse processa na fila de prioridade
}

//Retorna o proximo processo a obter a CPU, conforme o algortimo RRPrio 
Process* rrpSchedule(Process *plist) { // (feito(falta comentar tudo) - Vinicius) - acredito que tenha algum erro nessa funcao porem se comentar a funcao da erro tambem , mas demora pra dar erro
	if(currentProcess != NULL && processGetStatus(currentProcess)==PROC_READY)//se o processo atual eh diferente de NULL e o status do processo atual eh pronto.
    {
        iterationsOnCPU++;
        return currentProcess;//retorna o processo
    }
    
    iterationsOnCPU=1;

    int i;
    for(i=4;i>=0;--i)
    {
        if(priorityQueues[i].first == NULL) continue;

        ProcessNode *last = priorityQueues[i].last;
        ProcessNode *processNodeIt = requeueProcessNode(priorityQueues + i);

        int foundProcessReady = 1;
        while(processGetStatus(processNodeIt->process) != PROC_READY)
        {
            if(processNodeIt == last)
            {
                foundProcessReady = 0;
                break;
            }
            processNodeIt = requeueProcessNode(priorityQueues + i);
        }

        if(foundProcessReady)
        {
            currentProcess = processNodeIt->process;
            return currentProcess;
        }

    }
    return NULL;
}

//Libera os parametros de escalonamento de um processo p. Funcao chamada 
//normalmente quando o processo e' desassociado do slot de RRPrio
//Retorna o numero do slot ao qual o processo estava associado
int rrpReleaseParams(Process *p) { //( feito - Vinicius )
	RRPSchedParams *rrpSchedParams = (RRPSchedParams*)processGetSchedParams(p);
	//instancia da estrutura da rrprio.h recebendo um ponteiro de parametros de escalonamento 
	int index = removeProcess(priorityQueues + rrpSchedParams->priority,p);//busca o processo na fila de prioridade, remove-o e retorna sua posicao 
	free(rrpSchedParams);//libera o bloco rrpSchedParams instanciado acima
	return index;
}

//Modifica a prioridade atual de um processo
//E' a funcao de setSomeFeatureFn() de RRPrio
void rrpSetPrio(Process *p, void *rrparams) { //( feito - Vinicius )
	RRPSchedParams *rrpSchedParams = (RRPSchedParams*)processGetSchedParams(p);//instancia com antigos parametros de escalamento do processo p. 
    RRPSchedParams *rrpSched = (RRPSchedParams*)rrparams;//intancias com novos parametros de escalonamento.
    int oldprio = rrpSchedParams->priority; //parametros de escalonamento do processo p colocado como prioridade antiga.

    if(rrpSched->priority != oldprio)//se as novas prioridades forem diferentes das antigas.
    {
        rrpReleaseParams(p);//Libera os parametros de escalonamento do um processo p.
        rrpSchedParams = (RRPSchedParams*)malloc(sizeof(RRPSchedParams));//aloca dinamicamente um espaço para a intancia de parametros de escalonamento.
        rrpSchedParams->priority = rrpSched->priority;//atribui-se os novos parametros de escalonamento à instancia rrpSchedParams. 
        rrpInitSchedParams(p,rrpSchedParams);//inicia o processo p com os novos paramentros de escalonamento.
    }
    
}

//Notifica a mudanca de status de um processo para possivel manutencao de dados
//internos ao algoritmo RRPrio, responsavel pelo processo
void rrpNotifyProcessStatus(Process *p, int oldstatus) {
	 int status = processGetStatus(p);
     printf("Status: %d",status);
}

//Funcao chamada pela inicializacao do S.O. para a incializacao do escalonador
//conforme o algoritmo Round Robin com Prioridades
//Deve envolver a inicializacao de possiveis parametros gerais
//Deve envolver o registro do algoritmo junto ao escalonador
//Retorna o numero do slot obtido no registro do algoritmo junto ao escalonador
int rrpInitSchedInfo() { //(feito - Vinicius)
	SchedInfo *sched=(SchedInfo*)malloc(sizeof(SchedInfo));
	strcpy(sched->name,rrpName);                    //copia o nome do schedule para o vetor de caracteres.
	sched->initParamsFn = rrpInitSchedParams;		//a funcao para inicializar os parametros de escalonamento de um processo.
	sched->scheduleFn = rrpSchedule;                //a funcao para decidir qual o proximo processo a obter a CPU.
	sched->releaseParamsFn = rrpReleaseParams;      //a funcao para liberar os parametros de escalonemnto de um processo.
	RRPrioSchedSlot = schedRegisterScheduler(sched);//funcao que atribui ao slot a estrutura com informacoes do novo escalonador (sched) e retorna o indice do slot ocupado.
	return RRPrioSchedSlot;
}
