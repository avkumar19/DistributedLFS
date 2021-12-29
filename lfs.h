#include "mfs.h"

#define MAXINODE 4096
#define MAXIMAPSIZE 16
#define MAXIMAPS 256
#define MAXDIRSIZE 128
#define MAXDP 14
#define BLOCKSIZE 4096
#define MAXNAMELEN 28

enum TYPE {dir,regular};


typedef struct CR_t
{
    int endLog;
    int iMap[MAXIMAPS];
}CR_t;

typedef struct Inode_t {
    MFS_Stat_t stats;
    int dp[MAXDP];
}Inode_t;

typedef struct Dir_t {
    struct __MFS_DirEnt_t dTable[MAXDIRSIZE];
}Dir_t;

typedef struct Imap_t
{
    int iLoc[MAXIMAPSIZE];
}Imap_t;

typedef struct Inode_t_Map{
  int inodes[MFS_BLOCK_SIZE];
}Inode_t_Map;

int disk;
CR_t cr;

Inode_t_Map inodeMap;





int memLoad();
int createInode(int pinum, int type);

int createImapBlock();

int deleteInode(int inum);
void printDisk();
int deleteImapBlock(int imapPieceIndex);
int fsFindInodeAddr(int iParent);
int fsLookup(int iParent, char *name);
int fsRead(int inum, char *buffer, int block);
int dumpFileInodeDataImap(Inode_t* inode, int inum, char* data, int block);
int dumpDirInodeDataImap(Inode_t* inode, int inum, Dir_t* dirBlock, int block);
int fsWrite(int inum, char *buffer, int block) ;
int fsUnlink(int iParent, char *name);
int updateDirUnlink(Inode_t* inode, char* name, int* delInodeAddr);
int updateDir(Inode_t* inode,int newiNum,char* name);
int updateCRMap(int iNum, int iAddr);
int fsCreate(int iParent, int type, char *name);
int fsInit(char* fsImage);
int fsShutDown();
int fsStat(int iNum, MFS_Stat_t* stat);



