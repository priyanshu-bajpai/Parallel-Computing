#include "lib.hpp"

typedef map < pair<string,off_t>, pair<int, bool[50]> > Mymap1;
typedef map < string,pair <string, bool[50]> > Mymap2;
//hashmap for storing the filenames, count and in whihc FS they are present.
 Mymap1 map1;
// hashmap to store the md5, filename and in which FS they are present
 Mymap2 map2;

int MAX_DIRECTORIES = 200;
int level[50]={0};
char **Filename, **filesystem;
unsigned char result[MD5_DIGEST_LENGTH];

pthread_mutex_t mutex1=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex2=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond2=PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex3=PTHREAD_MUTEX_INITIALIZER;

int NUM_THREADS, NUM_THREADS_COMPLETED, THREADS_REMAINING;
int LEVEL=0;

extern  int alphasort();
void masterThread(int num_threads);
void *SlaveThread(void *threadid);
off_t fsize(const char *filename);



/**************************************QUESTIONS*******************************************/
//how master will initialize THREADS_REMAINING=NUM_THREADS;
//how will it clear the hashmap ? and and how to initialize the bool array with all false
//whether more than 2 mutexes are needed? seperate for hashmap updation and waiting while all have updated
/**************************************QUESTIONS*******************************************/


int main(int argc, char *argv[]) {

  NUM_THREADS=argc-1;
  filesystem = (char**)malloc(sizeof(char*)*(argc-1));
  for(int i=0;i<argc-1;i++)
  {
    filesystem[i] = (char*)malloc(sizeof(char)*100);
    strcpy(filesystem[i],argv[i+1]);
  }
  masterThread(argc-1);
  return 0;
}


/************************************HELPER FUNCTIONS************************************/
int mod(int i){return i>=0?i:(-1*i);}


void addDirectory(char *path,char **directories, int *index){
    directories[*index]=(char*)malloc(sizeof(char)*MAXPATHLEN);
    strcpy(directories[*index],path);
    (*index)++;
}



off_t fsize(const char *filename) {
    struct stat st;
    if (stat(filename, &st) == 0)
        return st.st_size;
    fprintf(stderr, "Cannot determine size of %s: %s\n",
            filename, strerror(errno));
    return -1;
}



void TraverseMap2(){
  for(Mymap2::iterator it=map2.begin();it!=map2.end();++it)
  {
    int count=0;
    // map < unsigned char*,pair <string, bool[50]={false}> >
    string md5string = it->first;
    pair<string,bool[50]>p = it->second;
    vector<int>v;
    for(int i=0;i<50;i++)
      if(p.second[i])
	{
	v.push_back(i);
        count++;
	}

    if(count>ceil((float)NUM_THREADS/2))
	{
	 sort(v.begin(),v.end());
	 cout<<"FILE: "<<p.first<<" PRESENT IN "<<count<<" FILE SYSTEMS:";
	 for (std::vector<int>::iterator it1=v.begin(); it1!=v.end(); ++it1)
    		std::cout << ' ' << *it1;
	 printf("\n");
	}

  }
  return ;
}



unsigned long get_size_by_fd(int fd) {
    struct stat statbuf;
    if(fstat(fd, &statbuf) < 0) exit(-1);
    return statbuf.st_size;
}



string md5(string p){
    int file_descript;
    unsigned long file_size,filesize;
    char* file_buffer;

    file_descript = open((char*)p.c_str(), O_RDONLY);
    if(file_descript < 0) exit(-1);

    file_size = get_size_by_fd(file_descript);

    file_buffer = (char*)mmap(0, file_size, PROT_READ, MAP_SHARED,file_descript, 0);
    MD5((unsigned char*) file_buffer, file_size, result);
    munmap(file_buffer, file_size);

	char *mm=(char*)malloc(sizeof(char*)*(MD5_DIGEST_LENGTH*2+1));
	for(int i=0;i<MD5_DIGEST_LENGTH;i++)
    	sprintf(&mm[i*2], "%02X", result[i]);
  string str(mm);
    return str;
}
/************************************HELPER FUNCTIONS************************************/



/************************************MASTER************************************/

void masterThread(int num_threads)
{
//Initializing constants
    NUM_THREADS_COMPLETED=0;
    THREADS_REMAINING = NUM_THREADS;
  pthread_t threads[num_threads];
  int array[num_threads];

  for(int i=0;i<num_threads;i++)
  {
    array[i]=i;
    pthread_create(&threads[0], NULL,&SlaveThread,(void*)&array[i]);
  }

  for(int i=0;i<num_threads;i++)
    pthread_join(threads[0], NULL);
  return ;
}
/************************************************************************/



/*************************************SLAVE***********************************/
void *SlaveThread(void *threadid){

  int count,i,index=1;
	struct direct **files;
  int threadId = *(int*)threadid;
  char *pathname=filesystem[threadId];
  int  startIndex =strlen(pathname)+1;
  char rname[MAXPATHLEN];
  char **directories = (char**) malloc(sizeof(char*)*MAX_DIRECTORIES);
  int front=0;
  directories[front]=NULL;
  int level=0;

  while(front<=index){

    if(pathname!=NULL){
      count = scandir(pathname, &files,0, alphasort);
      if(count <= 0){	printf("No files in this directory\n");exit(0);}

      for (i=0;i<count;++i){
        if ((strcmp(files[i]->d_name, ".") == 0) || (strcmp(files[i]->d_name, "..") == 0))
          continue;
        string str(pathname+startIndex);

        if(level!=0)str+="/";

        str.append(files[i]->d_name);

        char rname[MAXPATHLEN];
        if(strcpy(rname,pathname) == NULL){printf("Error getting path\n"); exit(0);}
        strcat(rname,"/");
        strcat(rname,files[i]->d_name);

        pair<string,off_t>p(str,fsize((char*)rname));
//critical section to be accessed by only one thread
        pthread_mutex_lock(&mutex1);
            if(map1.find(p)==map1.end())
              map1[p].first=1;
            else
              map1[p].first++;
              map1[p].second[threadId]=true;
        //cout<<"Thread: "<<threadId<<" inserting file "<<str<<" of size: "<<fsize((char*)rname)<<" in map1\n";
        pthread_mutex_unlock(&mutex1);
      }

    }

// if the thread has listed all the files in a particular level it waits until all threads have
// updated the hashmap before it can process the map and update the next map.
    else
    {
      pthread_mutex_lock(&mutex2);

        NUM_THREADS_COMPLETED++;
        if(NUM_THREADS_COMPLETED<THREADS_REMAINING)
          while(pthread_cond_wait(&cond1,&mutex2)!=0);

        else
        {
            pthread_cond_broadcast(&cond1);
            NUM_THREADS_COMPLETED=0;
        }

      pthread_mutex_unlock(&mutex2);


      pthread_mutex_lock(&mutex1);
      for(Mymap1 :: iterator it1 = map1.begin();it1!=map1.end();++it1)
      {
//if the number of FS in which file is present is greater than k/2 and this file system has it than
//update the second hashmap with the filename and md5
//< pair<string,off_t>, pair<int, bool[50]> >
        pair<string,off_t> key = it1->first;
        pair<int, bool[50]> value = it1->second;

        if((value.first>ceil((float)NUM_THREADS/2))&&(value.second[threadId]==1))
         {
                 string  temp(filesystem[threadId]);
                 temp+="/";
                 temp+=key.first;
                 int status;
                 struct stat st_buf;
                 status = stat ((char*)temp.c_str(), &st_buf);
                 if (status != 0) {printf ("Error, errno = %d\n", errno);exit(0);}

//if(directory) than add it to the end of the directories for for processing, to map2
//add it at the end of the directories to be traversed
                 if (S_ISDIR (st_buf.st_mode))
                    addDirectory((char*)temp.c_str(),directories,&index);

                 if (S_ISREG (st_buf.st_mode))
                 {
 //generating the md5 value for the file    //map < string,pair <string, bool[50]={false}> >
                  string md5string = md5(temp);
                  //pthread_mutex_lock(&mutex1);

                    if(map2.find(md5string)==map2.end())
                      map2[md5string].first = key.first;
                    map2[md5string].second[threadId]=true;
                   //pthread_mutex_unlock(&mutex1);
                 }
          }
      }
      pthread_mutex_unlock(&mutex1);
      if(directories[index-1]!=NULL)
      {
        directories[index]=NULL;
        index++;
        level++;
      }


//Thread will wait for all other threads to complete
      pthread_mutex_lock(&mutex2);
        NUM_THREADS_COMPLETED++;
        if(NUM_THREADS_COMPLETED<THREADS_REMAINING)
          while(pthread_cond_wait(&cond2,&mutex2)!=0);

//last thread which completes will have to do some extra task before next level can be traversed
        else{
//traverse map2 to list all the files which are common:
        TraverseMap2();
//clear the hash table map1 and map2 for further use by next level
          map1.clear();
          map2.clear();
          NUM_THREADS_COMPLETED=0;
          pthread_cond_broadcast(&cond2);
        }
      pthread_mutex_unlock(&mutex2);
    }
    pathname=directories[front];
    front++;
  }


  pthread_mutex_lock(&mutex3);
  //cout<<"THREAD:"<<threadId<<"ENDING.\n";
  THREADS_REMAINING--;
  pthread_mutex_unlock(&mutex3);
  return NULL;
}
