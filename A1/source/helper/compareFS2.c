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


/*
1.Filename[]: array storing the files that the respective threads are currently accessing
2.Level[]: store the level at which every thread is.

*/

int space=0;
int MAX_DIRECTORIES = 200;
char **Filename;
int level[2]={0};
char **filesystem;
int condition1=0, condition2=0;

extern  int alphasort();
void compareFS(int k);
void *traverseSystem(void *threadid);

pthread_cond_t condition = PTHREAD_COND_INITIALIZER;
pthread_mutex_t condition_mutex = PTHREAD_MUTEX_INITIALIZER;

/******************************* MAIN ***********************************/
int main(int argc, char*argv[])
{
  int i;
  filesystem = (char**)malloc(sizeof(char*)*(argc-1));
  Filename = (char**)malloc(sizeof(char*)*(100));
  for(i=0;i<argc-1;i++)
  {
    Filename[i]= NULL;//(char*)malloc(sizeof(char)*100);
    filesystem[i] = (char*)malloc(sizeof(char)*100);
    strcpy(filesystem[i],argv[i+1]);
    //printf("filesystem: %s\n",filesystem[i]);
  }

  compareFS(argc-1);
  return 0;
}
/************************************************************************/

void compareFS(int k){
  pthread_t threads[2];
  int i=0,j=1;

      pthread_create(&threads[0], NULL,&traverseSystem,(void*)&i);
      pthread_create(&threads[1], NULL,&traverseSystem,(void*)&j);
      pthread_join(threads[0], NULL);
      pthread_join(threads[1], NULL);
}

void addDirectory(char *path,char **directories, int *index){
    (*index)++;
    directories[*index]=(char*)malloc(sizeof(char)*MAXPATHLEN);
    strcpy(directories[*index],path);
}

void makepath(char *pathname, char *pwd, char *append){
  if(strcpy(pathname,pwd) == NULL){printf("Error getting path\n"); exit(0);}
  strcat(pathname,"/");
  strcat(pathname,append);
}

int mod(int i){return i>=0?i:(-1*i);}


//args to store the thread number on the basis of which it will access filename1/2
void *traverseSystem(void *data){
  int count,i,index=-1;
	struct direct **files;
	int file_select();

  char pathname[MAXPATHLEN];

  if(getcwd(pathname,MAXPATHLEN) == NULL){printf("Error getting path\n"); exit(0);}
  strcat(pathname,"/");
  int threadId = *(int*)data;
  strcat(pathname,filesystem[threadId]);
  char **directories = (char**) malloc(sizeof(char*)*MAX_DIRECTORIES);

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
        //printf("THREADID: %d - ",threadId);
        Filename[threadId]=files[i]->d_name;
        //printf("%s\n",Filename[threadId]);
/***************************** critical section ****************************/
      pthread_mutex_lock(&condition_mutex);
      if((level[threadId]>level[mod(threadId-1)])||(Filename[mod(threadId-1)]!=NULL && strcmp(files[i]->d_name,Filename[mod(threadId-1)])>0)){
            printf("THREADID: %d waiting on file:",threadId);
            printf("%s AT LEVEL %d while other thread at level %d acessing file: %s\n",
            Filename[threadId],level[threadId],level[mod(threadId-1)],Filename[mod(threadId-1)]);
            pthread_cond_signal(&condition);
            while(pthread_cond_wait(&condition,&condition_mutex));
          }

          else if(Filename[mod(threadId-1)]!=NULL && (strcmp(files[i]->d_name,Filename[mod(threadId-1)])==0))
          {
            //printf("THREADID: %d - ",threadId);
            //printf("LEVEL: %d\n",level[threadId]);
            printf("common file found: %s\n",files[i]->d_name);
            if(files[i]->d_type == DT_DIR)
            {
              makepath(pathname,directories[front],files[i]->d_name);
              addDirectory(pathname,directories,&index);
            }
              // else
              // {//match the content of the files
              //   continue;
              // }
             pthread_cond_broadcast(&condition);
          }
      pthread_mutex_unlock(&condition_mutex);
/***************************** critical section end ****************************/
      }
    }
    else  {
      index++;
      directories[index]=NULL;
      level[threadId]++;
    }
    front++;
  }
  return NULL;
}

int file_select(struct direct *entry){
  if ((strcmp(entry->d_name, ".") == 0) ||(strcmp(entry->d_name, "..") == 0))
    return (FALSE);
  return (TRUE);
}
