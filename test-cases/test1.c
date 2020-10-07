#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <crypter.h>

int main()
{
  DEV_HANDLE cdev;
  // char *msg = "Hello CS730!";
  // ADDR_PTR op_text = map_card()
  KEY_COMP a=31, b=17;
  // uint64_t size = strlen(msg);
  uint64_t size = 0xffff;
  char op_text[2*size];
  // strcpy(op_text, msg);
  for(int i=0;i<2*size;i++){
    op_text[i]='a';
    i++;
    op_text[i]='b';
  }
  cdev = create_handle();
  // ADDR_PTR op_text = map_card(cdev,size);
  
  if(cdev == ERROR)
  {
    printf("Unable to create handle for device\n");
    exit(0);
  }
  printf("%d",cdev);
  if(set_key(cdev, a, b) == ERROR){
    printf("Unable to set key\n");
    exit(0);
  }
  if(set_config(cdev,DMA,SET)== ERROR)
    {
    printf("Unable to set config\n");
    exit(0);
  }
  if(set_config(cdev,INTERRUPT,SET)== ERROR)
    {
    printf("Unable to set config\n");
    exit(0);
  }
  // strncpy((char *)op_text,msg,size);
  printf("Original Text: %c %c %c %c\n", op_text[100],op_text[101],op_text[size+1],op_text[size+2]);

  encrypt(cdev, op_text, 2*size, 0);
  printf("Original Text: %c %c %c %c\n", op_text[100],op_text[101],op_text[size+1],op_text[size+2]);

  decrypt(cdev, op_text, 2*size, 0);
  printf("Original Text: %c %c %c %c\n", op_text[100],op_text[101],op_text[size+1],op_text[size+2]);

  close_handle(cdev);
  return 0;
}
