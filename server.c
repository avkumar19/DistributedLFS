#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

#include "mfs.h"	
#include "udp.h"
#include "lfs.h"



void printserverError(int line)
{
    printf("Server Error at line %d \n",((line)));
}

int init(int port, char* image_path)
{
    int res = fsInit(image_path);
    if(res==-1)
    {
        printserverError(__LINE__);
        return res;
    }

  int sd =-1;
  struct sockaddr_in socket;
  UDP_Packet BufferPacket,  RespPacket;
  int flag = 1;


  if((sd =   UDP_Open(port))< 0){
    perror("server_init: port open fail");
    return -1;
  }

  while (flag) {
    if( UDP_Read(sd, &socket, (char *)&BufferPacket, sizeof(UDP_Packet)) < 1)
      continue;


    if(BufferPacket.request == REQ_LOOKUP){
      RespPacket.inum = fsLookup(BufferPacket.inum, BufferPacket.name);
      RespPacket.request = REQ_RESPONSE;
      UDP_Write(sd, &socket, (char*)&RespPacket, sizeof(UDP_Packet));

    }
    else if(BufferPacket.request == REQ_UNLINK){
      RespPacket.inum = fsUnlink(BufferPacket.inum, BufferPacket.name);
      RespPacket.request = REQ_RESPONSE;
      UDP_Write(sd, &socket, (char*)&RespPacket, sizeof(UDP_Packet));

    }
    else if(BufferPacket.request == REQ_READ){
      RespPacket.inum = fsRead(BufferPacket.inum, RespPacket.buffer, BufferPacket.block);
      RespPacket.request = REQ_RESPONSE;
      UDP_Write(sd, &socket, (char*)&RespPacket, sizeof(UDP_Packet));

    }
    else if(BufferPacket.request == REQ_SHUTDOWN) {
      RespPacket.request = REQ_RESPONSE;
      UDP_Write(sd, &socket, (char*)&RespPacket, sizeof(UDP_Packet));
      fsShutDown();
    }
    else if(BufferPacket.request == REQ_STAT){
      RespPacket.inum = fsStat(BufferPacket.inum, &(RespPacket.stat));
      RespPacket.request = REQ_RESPONSE;
      UDP_Write(sd, &socket, (char*)&RespPacket, sizeof(UDP_Packet));

    }
    else if(BufferPacket.request == REQ_RESPONSE) {
      RespPacket.request = REQ_RESPONSE;
      UDP_Write(sd, &socket, (char*)&RespPacket, sizeof(UDP_Packet));
    }
    else if(BufferPacket.request == REQ_CREAT){
      RespPacket.inum = fsCreate(BufferPacket.inum, BufferPacket.type, BufferPacket.name);
      RespPacket.request = REQ_RESPONSE;
      UDP_Write(sd, &socket, (char*)&RespPacket, sizeof(UDP_Packet));

    }
    else if(BufferPacket.request == REQ_WRITE){
      RespPacket.inum = fsWrite(BufferPacket.inum, BufferPacket.buffer, BufferPacket.block);
      RespPacket.request = REQ_RESPONSE;
      UDP_Write(sd, &socket, (char*)&RespPacket, sizeof(UDP_Packet));

    }
    else {
      perror("server_init: unknown request");
      flag = 0;
      return -1;
    }


  }

  return 0;
}

int main(int argc, char *argv[])
{
	if(argc != 3) {
		perror("Usage: server [portnum] [file-system-image]\n");
		exit(1);
	}

	init(atoi(argv[1]),argv[2]);

	return 0;
}
