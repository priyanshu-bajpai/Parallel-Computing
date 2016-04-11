#include <sys/types.h>
#include <sys/dir.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/param.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
# include <pthread.h>
#include <openssl/md5.h>
#define FALSE 0
#define TRUE !FALSE

int level=0;
int space=0;
int MAX_DIRECTORIES = 200;
char Filename1[MAXPATHLEN]="abc",Filename2[MAXPATHLEN]="bcd";
char **filename;
int condition1=0, condition2=0;

extern  int alphasort();
void *traverseSystem(void *threadid);
int main(int argc, char*argv[])
{
  int i;
  filename = (char**)malloc(sizeof(char*)*(argc-1));
  for(i=0;i<argc-1;i++)
  {
    filename[i] = (char*)malloc(sizeof(char)*100);
    strcpy(filename[i],argv[i+1]);
  }
  int threadid=0;
  traverseSystem(&threadid);
  return 0;
}


void addDirectory(char *path,char **directories, int *index)
{
    (*index)++;
    directories[*index]=(char*)malloc(sizeof(char)*MAXPATHLEN);
    strcpy(directories[*index],path);
}


//args to store the thread number on the basis of which it will access filename1/2
void *traverseSystem(void *threadid)
{
  int count,i,index=-1;
	struct direct **files;
	int file_select();

  char pathname[MAXPATHLEN];

  if(getcwd(pathname,MAXPATHLEN) == NULL){printf("Error getting path\n"); exit(0);}
  strcat(pathname,"/");
  int threadId = *(int*)threadid;
  strcat(pathname,filename[threadId]);
  char **directories = (char**) malloc(sizeof(char*)*MAX_DIRECTORIES);
  //printf("\npathname: %s\n",pathname);
  addDirectory(pathname,directories,&index);
  index++;
  directories[index]=NULL;
  int front=0;

  while(front<index)
  {
    if(directories[front]!=NULL)
    {
      printf("pathname: %s\n",directories[front]);
      count = scandir(directories[front], &files, file_select, alphasort);
      if(count <= 0){	printf("No files in this directory\n");exit(0);}

      for (i=0;i<count;++i)
      {
        printf("%s\n",files[i]->d_name);
        strcpy(Filename1,files[i]->d_name);
            if(files[i]->d_type == DT_DIR)
            {
              //add it at the end of the directories to be traversed
              if(strcpy(pathname,directories[front]) == NULL){printf("Error getting path\n"); exit(0);}
              strcat(pathname,"/");
              strcat(pathname,files[i]->d_name);
              addDirectory(pathname,directories,&index);
            }
       }
    }
    else
    {
      index++;
      directories[index]=NULL;
      level++;
      printf("\nLEVEL: %d\n",level);
    }
    front++;
  }
  return NULL;
}


int file_select(struct direct *entry)
{
  if ((strcmp(entry->d_name, ".") == 0) ||(strcmp(entry->d_name, "..") == 0))
    return (FALSE);
  return (TRUE);
}
