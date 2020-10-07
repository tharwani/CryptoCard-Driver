#include<crypter.h>

/*Function template to create handle for the CryptoCard device.
On success it returns the device handle as an integer*/
DEV_HANDLE create_handle()
{
  return open("/dev/hello_crypto",O_RDWR);
  // open call for char_device
}

/*Function template to close device handle.
Takes an already opened device handle as an arguments*/
void close_handle(DEV_HANDLE cdev)
{
  close(cdev);
  //close the char device
}

/*Function template to encrypt a message using MMIO/DMA/Memory-mapped.
Takes four arguments
  cdev: opened device handle
  addr: data address on which encryption has to be performed
  length: size of data to be encrypt
  isMapped: TRUE if addr is memory-mapped address otherwise FALSE
*/
int encrypt(DEV_HANDLE cdev, ADDR_PTR addr, uint64_t length, uint8_t isMapped)
{
  if(isMapped){
    char *buff = (char *)malloc(2*sizeof(char));
    *buff=(uint8_t)2;
    *(buff+1)=isMapped;
    // *(buff+2)=(uint64_t)addr;
    if (read(cdev,buff,length)<0){
      free(buff);
      return ERROR;
    }// strncpy(addr,buff,length);
    free(buff);
  }
  else{
    char *buff = (char *)malloc((length+2)*sizeof(char));
    *buff=(uint8_t)2;
    *(buff+1)=isMapped;
    // *(buff+2) = (uint64_t)addr;
    memcpy(buff+2,addr,length);
    // *(buff+10)='\0';
    // printf("addr: %p,%c,%c,%c,%c,%c,%x,%x,%x,%lx\n",addr,*(buff+2),*(buff+3),*(buff+4),*(buff+5),*(buff+6),*(buff+7),*(buff+8),*(buff+9),&addr);
    if (read(cdev,buff,length)<0){
      free(buff);
      return ERROR;
    }memcpy(addr,buff,length);
    free(buff);
  }
  return 0;
}

/*Function template to decrypt a message using MMIO/DMA/Memory-mapped.
Takes four arguments
  cdev: opened device handle
  addr: data address on which decryption has to be performed
  length: size of data to be decrypt
  isMapped: TRUE if addr is memory-mapped address otherwise FALSE
*/
int decrypt(DEV_HANDLE cdev, ADDR_PTR addr, uint64_t length, uint8_t isMapped)
{
  if(isMapped){
    char *buff = (char *)malloc(2*sizeof(char));
    *buff=(uint8_t)3;
    *(buff+1)=isMapped;
    // *(buff+2)=(uint64_t)addr;
    if (read(cdev,buff,length)<0){
      free(buff);
      return ERROR;
    }// strncpy(addr,buff,length);
    free(buff);
  }
  else{
    char *buff = (char *)malloc((length+2)*sizeof(char));
    *buff=(uint8_t)3;
    *(buff+1)=isMapped;
    // *(buff+2) = (uint64_t)addr;
    memcpy(buff+2,addr,length);
    // *(buff+10)='\0';
    // printf("addr: %p,%c,%c,%c,%c,%c,%x,%x,%x,%lx\n",addr,*(buff+2),*(buff+3),*(buff+4),*(buff+5),*(buff+6),*(buff+7),*(buff+8),*(buff+9),&addr);
    if (read(cdev,buff,length)<0){
      free(buff);
      return ERROR;
    }memcpy(addr,buff,length);
    free(buff);
  }
  return 0;
}


/*Function template to set the key pair.
Takes three arguments
  cdev: opened device handle
  a: value of key component a
  b: value of key component b
Return 0 in case of key is set successfully*/
int set_key(DEV_HANDLE cdev, KEY_COMP a, KEY_COMP b)
{
  char *buff = (char *)malloc(3*sizeof(char));
    *buff=(uint8_t)1;
    // *(buff+1)=length;
    *(buff+1)=(uint8_t)a;
    *(buff+2)=(uint8_t)b;
    if (read(cdev,buff,3)<0){
      free(buff);
      return ERROR;
    }
  free(buff);
  return 0;
}

/*Function template to set configuration of the device to operate.
Takes three arguments
  cdev: opened device handle
  type: type of configuration, i.e. set/unset DMA operation, interrupt
  value: SET/UNSET to enable or disable configuration as described in type
Return 0 in case of key is set successfully*/
int set_config(DEV_HANDLE cdev, config_t type, uint8_t value)
{
  uint8_t op_type;
  if(type==DMA && value==0) op_type=0;
  else if(type==DMA && value==1) op_type=1;
  else if(type==INTERRUPT && value==0) op_type=2;
  else if(type==INTERRUPT && value==1) op_type=3;
  char *buff = (char *)malloc(sizeof(char));
  *buff=0;
  *(buff+1)=op_type;
  if (read(cdev,buff,1)<0){
      free(buff);
      return ERROR;
    }
    free(buff);
    return 0;
}

/*Function template to device input/output memory into user space.
Takes three arguments
  cdev: opened device handle
  size: amount of memory-mapped into user-space (not more than 1MB strict check)
Return virtual address of the mapped memory*/
ADDR_PTR map_card(DEV_HANDLE cdev, uint64_t size)
{
  if(size>0xfffff-0xa8) return NULL;
  ADDR_PTR ptr = mmap(0,size,PROT_READ | PROT_WRITE,MAP_PRIVATE,cdev,0);
  if(ptr) return ptr;
  else return NULL;
}

/*Function template to device input/output memory into user space.
Takes three arguments
  cdev: opened device handle
  addr: memory-mapped address to unmap from user-space*/
void unmap_card(DEV_HANDLE cdev, ADDR_PTR addr)
{
  munmap(addr,0xfffff-0xa8);
}
