/*
*  myfs.c - Implementacao do sistema de arquivos MyFS
*
*  Autores: Arthur de Freitas Dornelas
            João Cotta Badaró
            Vinicius da Cruz Soranço
*  Projeto: Trabalho Pratico II - Sistemas Operacionais
*  Organizacao: Universidade Federal de Juiz de Fora
*  Departamento: Dep. Ciencia da Computacao
*
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "myfs.h"
#include "vfs.h"
#include "inode.h"
#include "util.h"

#define SUPER_NUM_BLOCKS (3 * sizeof(unsigned int) + sizeof(char))
#define SUPER_BLOCKSIZE 0
#define SUPER_FREE_SPACE_SECTOR (sizeof(unsigned int) + sizeof(char))
#define SUPER_FIRST_BLOCK_SECTOR (2 * sizeof(unsigned int) + sizeof(char))


//Declaracoes globais
FSInfo *fsInfo;
File *files[MAX_FDS] = {NULL};

///////FUNCOES AUXILIARES/////////
int firstZeroBit(unsigned char byte) {
    unsigned char mask = 1;

    for (int i = 0; i < sizeof(unsigned char); i++) {
        if( (mask & byte) == 0 )
            return i;

        mask <<= (unsigned char) 1;
    }

    return -1;
}

unsigned char setBitToOne(unsigned char byte, unsigned int bit) {
    unsigned char mask = (unsigned char) 1 << bit;
    return byte | mask;
}

unsigned char setBitToZero(unsigned char byte, unsigned int bit) {
    unsigned char mask = ((unsigned char) 1 << bit);
    mask = ~mask;
    return byte & mask;
}

unsigned int findFreeBlock(Disk *disk) {
    unsigned char buffer[DISK_SECTORDATASIZE];
    if (diskReadSector(disk, 0, buffer) == -1)
        return -1;

    unsigned int sectorsPerBlock;
    char2ul(&buffer[SUPER_BLOCKSIZE], &sectorsPerBlock);
    sectorsPerBlock /= DISK_SECTORDATASIZE;

    unsigned int numBlocks;
    char2ul(&buffer[SUPER_NUM_BLOCKS], &numBlocks);

    unsigned int firstBlock;
    char2ul(&buffer[SUPER_FIRST_BLOCK_SECTOR], &firstBlock);

    unsigned int freeSpaceSector;
    char2ul(&buffer[SUPER_FREE_SPACE_SECTOR], &freeSpaceSector);

    unsigned int freeSpaceSize = firstBlock - freeSpaceSector;

    for (int i = freeSpaceSector; i < freeSpaceSector + freeSpaceSize; i++) {
        if (diskReadSector(disk, i, buffer) == -1)
            return -1;

        for (int j = 0; j < DISK_SECTORDATASIZE; j++) {
            int freeBit = firstZeroBit(buffer[j]);

            if(freeBit != -1) {
                unsigned int freeBlock = firstBlock +
                        (i - freeSpaceSector) * DISK_SECTORDATASIZE * 8 * sectorsPerBlock +
                        j * 8 * sectorsPerBlock +
                        freeBit * sectorsPerBlock;

                if ((freeBlock - firstBlock) / sectorsPerBlock >= numBlocks)
                    return -1;

                buffer[j] = setBitToOne(buffer[j], freeBit);
                if (diskWriteSector(disk, i, buffer) == -1)
                    return -1;

                return freeBlock;
            }
        }
    }

    return -1;
}

bool setBlockFree(Disk *d, unsigned int block) {
    unsigned char buffer[DISK_SECTORDATASIZE];
    if (diskReadSector(d, 0, buffer) == -1)
        return false;

    unsigned int sectorsPerBlock;
    char2ul(&buffer[SUPER_BLOCKSIZE], &sectorsPerBlock);
    sectorsPerBlock /= DISK_SECTORDATASIZE;

    unsigned int numBlocks;
    char2ul(&buffer[SUPER_NUM_BLOCKS], &numBlocks);

    unsigned int firstBlock;
    char2ul(&buffer[SUPER_FIRST_BLOCK_SECTOR], &firstBlock);

    unsigned int freeSpaceStartSector;
    char2ul(&buffer[SUPER_FREE_SPACE_SECTOR], &freeSpaceStartSector);

    if ((block - firstBlock) / sectorsPerBlock >= numBlocks)
        return false;

    unsigned int blockFreeSpaceSector = ((block - firstBlock) / sectorsPerBlock) / (DISK_SECTORDATASIZE * 8);
    if (diskReadSector(d, blockFreeSpaceSector, buffer) == -1)
        return false;

    unsigned int blockFreeSpaceBit = ((block - firstBlock) / sectorsPerBlock) % (DISK_SECTORDATASIZE * 8);
    buffer[blockFreeSpaceBit / 8] = setBitToZero(buffer[blockFreeSpaceBit / 8], blockFreeSpaceBit % 8);

    if (diskWriteSector(d, blockFreeSpaceSector, buffer) == -1)
        return false;

    return true;
}

File* getFile(Disk* d,const char* path) 
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
/////FIM - FUNCOES AUXILIARES/////

//Funcao para verificacao se o sistema de arquivos está ocioso, ou seja,
//se nao ha quisquer descritores de arquivos em uso atualmente. Retorna
//um positivo se ocioso ou, caso contrario, 0.
int myFSIsIdle (Disk *d) 
{ 
	for (int i = 0; i < MAX_FDS; i++)
	{
		if (files[i] != NULL && diskGetId(d) == diskGetId(files[i]->disk))
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
    unsigned char superblock[DISK_SECTORDATASIZE] = {0};

    ul2char(blockSize,&superblock[SUPER_BLOCKSIZE]);//funçao para conversao de um unsigned int para um array de bytes

    unsigned int numInodes = (diskGetSize(d) / blockSize ) / 8;

    unsigned int freeSpaceSector = inodeAreaBeginSector() + numInodes / inodeNumInodesPerSector();
    unsigned int freeSpaceSize = (diskGetSize(d) / blockSize) / (sizeof(unsigned char) * 8 * DISK_SECTORDATASIZE);

    ul2char(freeSpaceSector, &superblock[SUPER_FREE_SPACE_SECTOR]);

    unsigned int firstBlockSector = freeSpaceSector + freeSpaceSize;
    unsigned int numBlocks = (diskGetNumSectors(d) - firstBlockSector) / (blockSize / DISK_SECTORDATASIZE);

    ul2char(firstBlockSector, &superblock[SUPER_FIRST_BLOCK_SECTOR]);
    ul2char(numBlocks, &superblock[SUPER_NUM_BLOCKS]);

    if(diskWriteSector(d, 0, superblock) == -1)
      return -1;

    unsigned char freeSpace[DISK_SECTORDATASIZE] = {0};
      for(int i=0; i<freeSpaceSize ; i++)
      {
        if(diskWriteSector(d, freeSpaceSector + i, freeSpace) == -1)
        {
          return -1;
        }
      }

      return numBlocks > 0 ? numBlocks : -1; 
}


//Funcao para abertura de um arquivo, a partir do caminho especificado
//em path, no disco montado especificado em d, no modo Read/Write,
//criando o arquivo se nao existir. Retorna um descritor de arquivo,
//em caso de sucesso. Retorna -1, caso contrario.
int myFSOpen (Disk *d, const char *path) 
{
	File *file = getFile(d,path);
  int numInode;
  
  // arquivo não existe ,entao vamos criá-lo
  if(file == NULL)
  {
    Inode *inode = NULL;
    for (int i = 0; i < MAX_FDS; i++)
    {
      if (files[i] == NULL)
      {
        numInode = inodeFindFreeInode(1, d);
        inode = inodeCreate(numInode, d);
        break;
      }
    }

    if(inode == NULL)
      return -1;

    file = malloc(sizeof(File));
    file->disk = d;
    file->path = path; 
    file->inode = inode;
    file->fd = inodeGetNumber(inode);
    files[file->fd - 1] = file;
  }

  return file->fd;
}
	
//Funcao para a leitura de um arquivo, a partir de um descritor de
//arquivo existente. Os dados lidos sao copiados para buf e terao
//tamanho maximo de nbytes. Retorna o numero de bytes efetivamente
//lidos em caso de sucesso ou -1, caso contrario.
int myFSRead (int fd, char *buf, unsigned int nbytes) 
{
  /*if(fd < 0 || fd >= MAX_FDS) return -1;

  File* file = files[fd];
  if(file == NULL)
      return -1;

  unsigned int fileSize = inodeGetFileSize(file->inode);
  unsigned int bytesRead = 0;
  unsigned int currentInodeBlockNum = file->lastByteRead / file->blocksize;
  unsigned int offset = file->lastByteRead % file->blocksize;
  unsigned int currentBlock = inodeGetBlockAddr(file->inode, currentInodeBlockNum);
  unsigned char diskBuffer[DISK_SECTORDATASIZE];

    while(bytesRead < nbytes &&
          bytesRead + file->lastByteRead < fileSize &&
          currentBlock > 0) {
        unsigned int sectorsPerBlock = file->blocksize / DISK_SECTORDATASIZE;
        unsigned int firstSector = offset / DISK_SECTORDATASIZE;
        unsigned int firstByteInSector = offset % DISK_SECTORDATASIZE;

        for(int i = firstSector; i < sectorsPerBlock && bytesRead < nbytes; i++) {
            if(diskReadSector(file->disk, currentBlock + i, diskBuffer) == -1)
                return -1;

            for(int j = firstByteInSector;  j < DISK_SECTORDATASIZE &&
                bytesRead < nbytes &&
                bytesRead + file->lastByteRead < fileSize;  j++) {
                buf[bytesRead] = diskBuffer[j];
                bytesRead++;
            }

            firstByteInSector = 0;
        }

        offset = 0;
        currentInodeBlockNum++;
        currentBlock = inodeGetBlockAddr(file->inode, currentInodeBlockNum);
    }

    file->lastByteRead += bytesRead;

    return bytesRead;*/

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
	if(fd <= 0 || fd > MAX_FDS)
    return -1;

  File *file = files[fd];
  if(!file)
    return -1;

  unsigned int fileSize = inodeGetFileSize(file->inode);
  unsigned int bytesWritten = 0;
  unsigned int currentInodeBlockNum = file->lastByteRead / file->blocksize;
  unsigned int offset = file->lastByteRead % file->blocksize;
  unsigned int currentBlock = inodeGetBlockAddr(file->inode, currentInodeBlockNum);
  unsigned char diskBuffer[DISK_SECTORDATASIZE];

  while(bytesWritten < nbytes)
  {
    unsigned int sectorsPreBlock = file->blocksize / DISK_SECTORDATASIZE;
    unsigned int firstSector = offset / DISK_SECTORDATASIZE;
    unsigned int firstByteInSector = offset % DISK_SECTORDATASIZE;

    if(currentBlock == 0)
    {
      currentBlock = findFreeBlock(file->disk);

      if(currentBlock == -1)
        break;
      
      if(inodeAddBlock(file->inode, currentBlock) == -1)
      {
        setBlockFree(file->disk, currentBlock);
        break;
      }
    }

    for(int i = firstSector; i<sectorsPreBlock && bytesWritten < nbytes; i++)
    {
      if(diskReadSector(file->disk, currentBlock + i, diskBuffer) == -1) 
        return -1;

      for(int j = firstByteInSector; j < DISK_SECTORDATASIZE && bytesWritten < nbytes; j++)
      {
        diskBuffer[j]=buf[bytesWritten];
        bytesWritten++;
      }

      if(diskWriteSector(file->disk, currentBlock + i, diskBuffer) == -1)
        return -1;

      firstByteInSector = 0;
    }

    offset = 0;
    currentInodeBlockNum++;
    currentBlock = inodeGetBlockAddr(file->inode, currentInodeBlockNum);
  }

  file->lastByteRead += bytesWritten;
  if(file->lastByteRead >= fileSize)
  {
    inodeSetFileSize(file->inode, currentInodeBlockNum);
    inodeSave(file->inode);
  }
  return bytesWritten;
}

//Funcao para fechar um arquivo, a partir de um descritor de arquivo
//existente. Retorna 0 caso bem sucedido, ou -1 caso contrario
int myFSClose (int fd) {
	if(fd <= 0 || fd > MAX_FDS)
    return -1;

  File *file = files[fd];
  if(!file)
    return -1;

  files[fd - 1] = NULL;
  free(file->inode);
  free(file);

  return 0;
}

//Funcao para adicionar uma entrada a um diretorio, identificado por um
//descritor de arquivo existente. A nova entrada tera' o nome indicado
//por filename e apontara' para o numero de i-node indicado por inumber.
//Retorna 0 caso bem sucedido, ou -1 caso contrario.
// int myFSLink (int fd, const char *filename, unsigned int inumber) //feito(vinicius)
// {
// 	if(fd < 0 || fd >= MAX_FDS)
//     return -1;
  
//   File* dir = files[fd];
//   if (dir == NULL)
//     return -1;

//   Inode *inode = inodeLoad(inumber,dir->disk);
//   if (inode == NULL)
//     return -1;

//   LinkDir link;
//   strcpy(link.filename,filename);
//   link.inumber = inumber;
//   dir->link = link;

//   return 0;
// }

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
  //fsInfo->linkFn = myFSLink;
}
