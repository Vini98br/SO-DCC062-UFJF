/*
*  myfs.c - Implementacao do sistema de arquivos MyFS
*
*  Autores: SUPER_PROGRAMADORES_C
*  Projeto: Trabalho Pratico II - Sistemas Operacionais
*  Organizacao: Universidade Federal de Juiz de Fora
*  Departamento: Dep. Ciencia da Computacao
*
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "myfs.h"
#include "vfs.h"
#include "inode.h"
#include "util.h"

//Declaracoes globais
FSInfo *fsInfo;
File *files[MAX_FDS] = {NULL};

//Funcao para verificacao se o sistema de arquivos está ocioso, ou seja,
//se nao ha quisquer descritores de arquivos em uso atualmente. Retorna
//um positivo se ocioso ou, caso contrario, 0.
int myFSIsIdle (Disk *d) //feito(vinicius)
{ 
	for (int i = 0; i < MAX_FDS; i++)
	{
		if (files[i] != NULL)
		{
			return 0;
		}
	}
	return 1;
}

//Funcao para formatacao de um disco com o novo sistema de arquivos
//com tamanho de blocos igual a blockSize. Retorna o numero total de
//blocos disponiveis no disco, se formatado com sucesso. Caso contrario,
//retorna -1.
int myFSFormat (Disk *d, unsigned int blockSize) {
	  return -1;
}

File* getFile(Disk* d,const char* path) //feito(vinicius) - func aux
{
    for (int i = 0; i < MAX_FDS; i++)
    {
      if (files[i] != NULL && files[i]->disk == d && strcmp(files[i]->path,path) == 0)
      {
        return files[i];
      }
    }
    return NULL;
}

//Funcao para abertura de um arquivo, a partir do caminho especificado
//em path, no disco montado especificado em d, no modo Read/Write,
//criando o arquivo se nao existir. Retorna um descritor de arquivo,
//em caso de sucesso. Retorna -1, caso contrario.
int myFSOpen (Disk *d, const char *path) //feito(vinicius)
{
	File *file = getFile(d,path);

  // arquivo não existe ,entao vamos criá-lo
  if(file == NULL)
  {
    Inode *inode = NULL;
    for (int i = 0; i < MAX_FDS; i++)
    {
      if (files[i] == NULL)
      {
        inode = inodeCreate(i+1,d);
        break;
      }
    }

    if(inode == NULL)
      return -1;

    file = malloc(sizeof(file));
    file->disk = d;
    file->path = path; 
    file->fd = inodeGetNumber(inode);
    files[file->fd - 1] = file;
    //file->link = NULL;
  }

  return file->fd;
}
	
//Funcao para a leitura de um arquivo, a partir de um descritor de
//arquivo existente. Os dados lidos sao copiados para buf e terao
//tamanho maximo de nbytes. Retorna o numero de bytes efetivamente
//lidos em caso de sucesso ou -1, caso contrario.
int myFSRead (int fd, char *buf, unsigned int nbytes) //feito(vinicius)
{
	Inode *inode = NULL;

  for (int i = 0; i < MAX_FDS; i++)
  {
    if (files[i] != NULL && files[i]->fd == fd)
    {
      inode = inodeLoad(fd,files[i]->disk);
    }
  }

  if (inode == NULL)
  {
    return -1;
  }
}

//Funcao para a escrita de um arquivo, a partir de um descritor de
//arquivo existente. Os dados de buf serao copiados para o disco e
//terao tamanho maximo de nbytes. Retorna o numero de bytes
//efetivamente escritos em caso de sucesso ou -1, caso contrario
int myFSWrite (int fd, const char *buf, unsigned int nbytes) {
	return -1;
}

//Funcao para fechar um arquivo, a partir de um descritor de arquivo
//existente. Retorna 0 caso bem sucedido, ou -1 caso contrario
int myFSClose (int fd) {
	return -1;
}

//Funcao para abertura de um diretorio, a partir do caminho
//especificado em path, no disco indicado por d, no modo Read/Write,
//criando o diretorio se nao existir. Retorna um descritor de arquivo,
//em caso de sucesso. Retorna -1, caso contrario.
int myFSOpenDir (Disk *d, const char *path) {
	return -1;
}

//Funcao para a leitura de um diretorio, identificado por um descritor
//de arquivo existente. Os dados lidos correspondem a uma entrada de
//diretorio na posicao atual do cursor no diretorio. O nome da entrada
//e' copiado para filename, como uma string terminada em \0 (max 255+1).
//O numero do inode correspondente 'a entrada e' copiado para inumber.
//Retorna 1 se uma entrada foi lida, 0 se fim de diretorio ou -1 caso
//mal sucedido
int myFSReadDir (int fd, char *filename, unsigned int *inumber) {
	return -1;
}

//Funcao para adicionar uma entrada a um diretorio, identificado por um
//descritor de arquivo existente. A nova entrada tera' o nome indicado
//por filename e apontara' para o numero de i-node indicado por inumber.
//Retorna 0 caso bem sucedido, ou -1 caso contrario.
int myFSLink (int fd, const char *filename, unsigned int inumber) //feito(vinicius)
{
	if(fd < 0 || fd >= MAX_FDS)
    return -1;
  
  File* dir = files[fd];
  if (dir == NULL)
    return -1;

  Inode *inode = inodeLoad(inumber,dir->disk);
  if (inode == NULL)
    return -1;

  LinkDir link;
  strcpy(link.filename,filename);
  link.inumber = inumber;
  dir->link = link;

  return 0;
}

//Funcao para remover uma entrada existente em um diretorio, 
//identificado por um descritor de arquivo existente. A entrada e'
//identificada pelo nome indicado em filename. Retorna 0 caso bem
//sucedido, ou -1 caso contrario.
int myFSUnlink (int fd, const char *filename) {
	return -1;
}

//Funcao para fechar um diretorio, identificado por um descritor de
//arquivo existente. Retorna 0 caso bem sucedido, ou -1 caso contrario.	
int myFSCloseDir (int fd) {
	return -1;
}

//Funcao para instalar seu sistema de arquivos no S.O., registrando-o junto
//ao virtual FS (vfs). Retorna um identificador unico (slot), caso
//o sistema de arquivos tenha sido registrado com sucesso.
//Caso contrario, retorna -1
int installMyFS (void) //feito(vinicius)
{
	fsInfo = malloc(sizeof(FSInfo));

  //fsInfo->fsname = "DTFS";
  fsInfo->fsid = (char) vfsRegisterFS(fsInfo);
  fsInfo->isidleFn = myFSIsIdle;
  fsInfo->formatFn = myFSFormat;
  fsInfo->openFn = myFSOpen;
  fsInfo->readFn = myFSRead;
  fsInfo->writeFn = myFSWrite;
  fsInfo->closeFn = myFSClose;
  fsInfo->opendirFn = myFSOpenDir;
  fsInfo->readdirFn = myFSReadDir;
  fsInfo->linkFn = myFSLink;
  fsInfo->unlinkFn = myFSUnlink;
  fsInfo->closedirFn = myFSCloseDir;

}