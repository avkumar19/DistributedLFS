#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

#include "lfs.h"

int fsInit(char * fsImage){
  int temp, i = 0;

  disk = open(fsImage,O_RDWR);
    
  if((int)disk < 0){
    disk = open(fsImage, O_RDWR| O_CREAT| O_TRUNC, S_IRWXU);
    cr.endLog = sizeof(cr)+ sizeof(Imap_t) +sizeof(Inode_t)+sizeof(Dir_t);
    for(i = 0; i < MAXIMAPS; i++ ){
      cr.iMap[i] = -1;
    }
    cr.iMap[0] = sizeof(cr);  
    
    lseek(disk, 0, 0);
    write(disk, &cr, sizeof(cr));
       
    Imap_t imapP1;
    for(i = 0; i < MAXIMAPSIZE; i++)
      imapP1.iLoc[i] = -1; 
       
       
    imapP1.iLoc[0] = sizeof(cr)+sizeof(imapP1); 
       
    write(disk, &imapP1, sizeof(imapP1));
       
    Inode_t root_inode;
    root_inode.stats.type = 0;
    for(i = 0; i < MAXDP; i++)
      root_inode.dp[i] = -1;
       
    root_inode.dp[0] = sizeof(cr)+sizeof(imapP1)+sizeof(root_inode) ; 
    root_inode.stats.size = 2*MFS_BLOCK_SIZE; 
       
       
    write(disk, &root_inode, sizeof(root_inode));
    Dir_t rootDirBlock;      
    temp = sizeof(rootDirBlock)/sizeof(rootDirBlock.dTable[0]);
    for(i = 0; i < temp; i ++){
      rootDirBlock.dTable[i].inum= -1;
      memcpy(rootDirBlock.dTable[i].name, "\0", sizeof("\0"));
    }
    memcpy(rootDirBlock.dTable[0].name, ".\0", sizeof(".\0"));
    rootDirBlock.dTable[0].inum= 0;
    
    memcpy(rootDirBlock.dTable[1].name, "..\0", sizeof("..\0"));
    rootDirBlock.dTable[1].inum=0;

    
    write(disk, &rootDirBlock, sizeof(rootDirBlock));   
  }
  else{
    (void) read(disk,&cr, sizeof(cr));  
  }

  for(i = 0; i < MAXINODE; i++){
    inodeMap.inodes[i] = -1;
  }
  i =0;
  temp = 0;
  int j = 0; 
  Imap_t imapBlock;
  while(cr.iMap[i] >= 0){
    lseek(disk,cr.iMap[i], 0);
    read(disk,&imapBlock, sizeof(imapBlock));
    while(imapBlock.iLoc[temp] >= 0){
      inodeMap.inodes[j] = imapBlock.iLoc[temp];
      j++;temp++;
    }
    i++;
  }

  memLoad();
  return 0;
}


int fsLookup(int pinum, char * name){
  if(pinum >MAXINODE-1  || pinum < 0)
    return -1;
  if(inodeMap.inodes[pinum] == -1)
    return -1;
 
  if(strlen(name) < 1 || strlen(name) > MAXNAMELEN)
    return -1;
  
  int bytesRead;
  int i,dirBlockLocation, j;
 
 int pinumLocation = inodeMap.inodes[pinum];
  lseek(disk, pinumLocation, 0);
   
  Inode_t pInode;
  bytesRead = read(disk,&pInode, sizeof(pInode));

  if(pInode.stats.type != MFS_DIRECTORY){
    return -1;
  }

  for(j = 0; j < MAXDP; j++){
    dirBlockLocation = pInode.dp[j];
    lseek(disk, dirBlockLocation, 0);
    Dir_t dirBlock;
    bytesRead += read(disk,&dirBlock, sizeof(dirBlock));
    for(i = 0; i < MAXDIRSIZE; i++){
      if(strncmp(dirBlock.dTable[i].name, name, MAXNAMELEN) == 0){
	return dirBlock.dTable[i].inum;
      }
    }
  }
  return -1;
}

int fsCreate(int pinum, int type, char* name){
  if((type != MFS_DIRECTORY && type != MFS_REGULAR_FILE) || pinum > MAXINODE-1 || pinum < 0)
    return -1;
 
  if(strlen(name) < 1 || strlen(name) > MAXNAMELEN)
    return -1;
  
  if(inodeMap.inodes[pinum] == -1)
    return -1;

  if(fsLookup(pinum, name) >= 0)
    return 0;

  int pinumLocation = inodeMap.inodes[pinum];

  lseek(disk, pinumLocation, 0);
  
  Inode_t pInode;
  int bytesRead = read(disk,&pInode, sizeof(pInode));
   
  if(pInode.stats.type != MFS_DIRECTORY){
    return -1;
  }
  
  if(pInode.stats.size >= (MFS_BLOCK_SIZE * MAXDP*MAXDIRSIZE)){
     return -1;
  }
  
  int newInum = createInode(pinum,type);
  
  int inodeDirBlockIndex = (int) pInode.stats.size/(MFS_BLOCK_SIZE*MAXDIRSIZE);
  
 
  if(inodeDirBlockIndex < 0 ||inodeDirBlockIndex > MAXDP){
    return -1;
  }
 
  int dirBlockIndex = (int) (pInode.stats.size/(MFS_BLOCK_SIZE) %MAXDIRSIZE);
  pInode.stats.size += MFS_BLOCK_SIZE;
 
 
  if(dirBlockIndex < 0){
    return -1;
  }

  if(dirBlockIndex == 0){
    pInode.dp[inodeDirBlockIndex] = cr.endLog;
    Dir_t freshDirBlock;
    int k;
    for(k = 0; k < MAXDIRSIZE; k++){
      memcpy(freshDirBlock.dTable[k].name, "\0", sizeof("\0"));
      freshDirBlock.dTable[k].inum = -1;
    }
    lseek(disk, cr.endLog, 0);
    write(disk, &freshDirBlock, sizeof(freshDirBlock));
    
    cr.endLog += MFS_BLOCK_SIZE;
    lseek(disk, 0, 0);
    write(disk, &cr, sizeof(cr));   
  }
  
  lseek(disk, pinumLocation, 0);
  write(disk, &pInode, sizeof(pInode));
 
  memLoad();

  lseek(disk, pInode.dp[inodeDirBlockIndex], 0);
  Dir_t dirBlock;
  bytesRead = read(disk,&dirBlock, sizeof(dirBlock));
  
  int newIndex = dirBlockIndex;
  memcpy(dirBlock.dTable[newIndex].name, "\0", sizeof("\0"));
  memcpy(dirBlock.dTable[newIndex].name, name, MAXNAMELEN);
  dirBlock.dTable[newIndex].inum = newInum;
 
  lseek(disk, pInode.dp[inodeDirBlockIndex], 0);
  write(disk, &dirBlock, sizeof(dirBlock));
  memLoad();
  return 0;
}


int createImapBlock(){
  
  memLoad();
  int i, newImapBlockIndex;
  for(i = 0; i < MAXIMAPS; i++){
    if(cr.iMap[i] == -1){
      newImapBlockIndex = i;
      i = 5000;
    }
  }

  cr.iMap[newImapBlockIndex] = cr.endLog;
  Imap_t newPiece;
  for(i = 0; i < MAXIMAPSIZE; i++){
    newPiece.iLoc[i] = -1;
  } 
  cr.endLog += sizeof(newPiece);
  lseek(disk,0, 0);
  write(disk, &cr, sizeof(cr));
  
  lseek(disk,cr.iMap[newImapBlockIndex], 0);
  write(disk, &newPiece, sizeof(newPiece));
  memLoad();
  return newImapBlockIndex;
}


int deleteInode(int inum){
  
  int imapBlockIndex =inum/MAXIMAPSIZE;
  int imapInodeIndex = inum%MAXIMAPSIZE;
  int i;
  if(imapInodeIndex < 0){
    return -1;
  }

  Imap_t imapBlock;
  lseek(disk, cr.iMap[imapBlockIndex], 0);
  read(disk, &imapBlock, sizeof(imapBlock));

  imapBlock.iLoc[imapInodeIndex] = -1;
  i = 0;
  while(imapBlock.iLoc[i] > 0 && i < MAXIMAPSIZE)
    i++;

  if(i == 0){
    int  test =deleteImapBlock(imapBlockIndex);
  }
  else{
     lseek(disk, cr.iMap[imapBlockIndex], 0);
     write (disk, &imapBlock, sizeof(imapBlock));
  }
  memLoad();
  return 0;

}


int deleteImapBlock(int imapBlockIndex){
  memLoad();
  cr.iMap[imapBlockIndex] = -1;
  lseek(disk,0, 0);
  write(disk, &cr, sizeof(cr));
  memLoad();
  return 0;
}

int createInode(int pinum, int type){
  memLoad();
  int i, newInodeNum, newImapInodeIndex, test;
  for(i = 0; i < MAXINODE; i++ ){
    if(inodeMap.inodes[i] == -1){
      newInodeNum = i;
      i = 5000;
    }
  }
 
  int imapBlockIndex = newInodeNum/MAXIMAPSIZE;
  newImapInodeIndex = newInodeNum%MAXIMAPSIZE;
 
  if(newImapInodeIndex < 0){
    return -1;
  }
  if(newImapInodeIndex == 0){
    test =createImapBlock();
    newImapInodeIndex = 0;
  }
  
  Imap_t imapBlock;
  lseek(disk, cr.iMap[imapBlockIndex], 0);
  read(disk, &imapBlock, sizeof(imapBlock));
  
  imapBlock.iLoc[newImapInodeIndex] = cr.endLog;
  lseek(disk, cr.iMap[imapBlockIndex], 0);
  write(disk, &imapBlock, sizeof(imapBlock));
  
  Inode_t newInode;
  newInode.stats.type = type;
  for(i = 0; i < MAXDP; i++)
    newInode.dp[i] = -1;
       
  newInode.dp[0] = cr.endLog+sizeof(newInode); 
  if(type == MFS_DIRECTORY)
    newInode.stats.size = 2*MFS_BLOCK_SIZE; 
  else
    newInode.stats.size = 0; 
    
  lseek(disk, cr.endLog, 0);
  write(disk, &newInode, sizeof(newInode));
  
  cr.endLog += sizeof(newInode);
  
  if(type == MFS_DIRECTORY){
    Dir_t dirBlock;      
   
    int temp = sizeof(dirBlock)/sizeof(dirBlock.dTable[0]);
    for(i = 0; i < temp; i ++){
      dirBlock.dTable[i].inum= -1;
      memcpy(dirBlock.dTable[i].name, "\0", sizeof("\0"));
    }

    memcpy(dirBlock.dTable[0].name, ".\0", sizeof(".\0"));
    dirBlock.dTable[0].inum= newInodeNum;
    
    memcpy(dirBlock.dTable[1].name, "..\0", sizeof("..\0"));
    dirBlock.dTable[1].inum=pinum;
  
    write(disk, &dirBlock, sizeof(dirBlock));
    cr.endLog += sizeof(dirBlock);
  }
  else{
    char * newblock = malloc(MFS_BLOCK_SIZE);
    write(disk, newblock, MFS_BLOCK_SIZE);
    free(newblock);
    cr.endLog += MFS_BLOCK_SIZE;
  }

  lseek(disk, 0, 0);
  write(disk, &cr, sizeof(cr));

  memLoad();
  
  return newInodeNum;
}

int memLoad(){
  lseek(disk, 0, 0);
  read(disk, &cr, sizeof(cr));
  
  int i =0,temp = 0,  j = 0;
    
  for(i = 0; i < MAXINODE; i++){
    inodeMap.inodes[i] = -1;
  }
  
  i=0;temp = 0;
  Imap_t imapBlock; 
  for(i = 0; i < MAXIMAPS; i++){
    if(cr.iMap[i] >= 0){
      lseek(disk, cr.iMap[i], 0);
      read(disk,&imapBlock, sizeof(imapBlock));
      for(j = 0 ; j < MAXIMAPSIZE ; j++){
	if(imapBlock.iLoc[j] >= 0){
	  inodeMap.inodes[temp] = imapBlock.iLoc[j];
	  temp++;
	}
      }
    }
  }
  //fsync(disk);
  return 0;
}

int fsRead(const int inum, char *buffer, int block){
  if(inum > MAXINODE-1 || inum < 0)
    return -1;

  if(block > 13 || block < 0 )
    return -1;

  memLoad();
  
  if(inodeMap.inodes[inum] == -1){
    return -1;
  }

  Inode_t inode;
  lseek(disk, inodeMap.inodes[inum], 0);
  read(disk, &inode, sizeof(inode));

  if(inode.dp[block] == -1){
    return -1;
  }
  
  lseek(disk,inode.dp[block], 0);
  read(disk, buffer, MFS_BLOCK_SIZE);
  return 0;
}

int fsWrite(int inum, char *buffer, int block){
  if(inum > MAXINODE-1 || inum < 0)
    return -1;
  
  if(block > 13 || block < 0 )
    return -1;

  memLoad();

  if(inodeMap.inodes[inum] == -1){
    return -1;  
  }
  
  Inode_t inode;
  lseek(disk, inodeMap.inodes[inum], 0);
  read(disk, &inode, sizeof(inode));
  
  if(inode.stats.type != MFS_REGULAR_FILE){
    return -1;
  }
  
  if(inode.dp[block] == -1){
    int oldEndLog = cr.endLog;
    
    Dir_t freshDirBlock;
    int k;
    for(k = 0; k < MAXDIRSIZE; k++){
      memcpy(freshDirBlock.dTable[k].name, "\0", sizeof("\0"));
      freshDirBlock.dTable[k].inum = -1;
    }
    lseek(disk, cr.endLog, 0);
    write(disk, &freshDirBlock, sizeof(freshDirBlock));
    
    cr.endLog += MFS_BLOCK_SIZE;
    lseek(disk, 0, 0);
    write(disk, &cr, sizeof(cr));
    
    inode.dp[block] = oldEndLog;
    inode.stats.size = (block+1)*(MFS_BLOCK_SIZE);
    lseek(disk, inodeMap.inodes[inum], 0);
    write(disk, &inode, sizeof(inode));
    
    lseek(disk, oldEndLog, 0);
  }
  else{
  
    lseek(disk,inode.dp[block], 0);
    inode.stats.size = (block + 1)*(MFS_BLOCK_SIZE);
  }

  write(disk, buffer, MFS_BLOCK_SIZE);
 
  lseek(disk, inodeMap.inodes[inum], 0);
  write(disk, &inode, sizeof(inode));
  memLoad();
  return 0;
}

int fsUnlink(int pinum, char * name){
  int i, k;
  memLoad();
  if(pinum < 0 || pinum > MAXINODE-1)
    return -1;
  
  if(strlen(name) > MAXNAMELEN || strlen(name) < 0)
    return -1;


  if(inodeMap.inodes[pinum] == -1 )
    return -1;  
  
  if(strcmp(name, "..\0") == 0 || strcmp(name, ".\0") == 0 )
    return -1;
  
  int pinumLocation = inodeMap.inodes[pinum];

  lseek(disk, pinumLocation, 0);
  
  Inode_t pInode;
  int bytesRead =  read(disk,&pInode, sizeof(pInode));

   if(pInode.stats.type != MFS_DIRECTORY)
     return -1;
  
  int found = -1, deleteDirBlockLoc, deleteIndex, deleteInodeLocation, deleteINum;
  Dir_t dirBlock;
  for(i = 0; i < MAXDP; i++){
    if(pInode.dp[i] >= 0 )
    {
      lseek(disk,pInode.dp[i],0);
      read(disk, &dirBlock, sizeof(dirBlock));
      for(k = 0; k < MAXDIRSIZE; k++)
      {
	      if((dirBlock.dTable[k].inum>0) && strcmp(dirBlock.dTable[k].name, name) ==0)
        {
	        found = 1;
	        deleteInodeLocation = inodeMap.inodes[dirBlock.dTable[k].inum%MFS_BLOCK_SIZE];
          deleteINum = dirBlock.dTable[k].inum;
	        deleteIndex = k;
	        deleteDirBlockLoc = pInode.dp[i];
	        i = 5000;
          break;
	 
	      }
      }
    }
    if(found==1)
        break;

  } 
 
  if(found < 0)
    return 0;
  Inode_t inodeToDelete;
  lseek(disk, deleteInodeLocation,0);
  read(disk, &inodeToDelete, sizeof(inodeToDelete));
  if(inodeToDelete.stats.type == MFS_DIRECTORY){  
    if(inodeToDelete.stats.size > 2*(MFS_BLOCK_SIZE)){
      return -1;
    }
   }
  
  deleteInode(deleteINum);

  
  dirBlock.dTable[deleteIndex].inum = -1;
  memcpy(dirBlock.dTable[deleteIndex].name, "\0", sizeof("\0"));
  lseek(disk, deleteDirBlockLoc,0);
  write(disk, &dirBlock, sizeof(dirBlock));
      
  int sizeIndex = 0;
  for(i =0; i < MAXDP; i++)
    if(pInode.dp[i] != -1)
      sizeIndex =i;
	
  pInode.stats.size = (sizeIndex+1)*MFS_BLOCK_SIZE;
  lseek(disk, pinumLocation,0);
  write(disk, &pInode, sizeof(pInode));
  memLoad();
  return 0;
}

int fsStat(const int inum, MFS_Stat_t * m){
  if (inum < 0|| inum > MAXINODE-1)
    return -1;
 
  if(disk < 0)
    return -1;

  memLoad();  

  if(inodeMap.inodes[inum] == -1){
    return -1;  
  }
  
  Inode_t inode;
  lseek(disk, inodeMap.inodes[inum], 0);
  read(disk, &inode, sizeof(inode));
  
  m->type = inode.stats.type;
  m->size = inode.stats.size;
  return 0;
}

int fsShutDown(){
  close(disk);
  exit(0);
  return 0;
}

