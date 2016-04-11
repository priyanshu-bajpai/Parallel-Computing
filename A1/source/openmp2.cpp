# include "lib.hpp"

# include <omp.h>

typedef map < pair<string,off_t>, pair<int, bool[2]> > Mymap1;
typedef map < string,pair <string, bool[2]> > Mymap2;
//hashmap for storing the filenames, count and in whihc FS they are present.
 Mymap1 map1;
// hashmap to store the md5, filename and in which FS they are present
 Mymap2 map2;

int MAX_DIRECTORIES = 200;
int level[50]={0};
char **Filename, **filesystem;
unsigned char result[MD5_DIGEST_LENGTH];

int NUM_THREADS, NUM_THREADS_COMPLETED, THREADS_REMAINING;
int LEVEL=0, exactcount=0, filecount=0;
int exactcopy=1;
extern  int alphasort();
void masterThread();
void *SlaveThread();



int main(int argc, char *argv[]) {

  NUM_THREADS=argc-1;
  filesystem = (char**)malloc(sizeof(char*)*(NUM_THREADS));

  # pragma omp parallel for num_threads(NUM_THREADS)
   for(int i=0;i<NUM_THREADS;i++)
   {
     //int threadid = omp_get_thread_num();
     //cout<<"THREAD "<<threadid<<"allocating memory\n";
     filesystem[i] = (char*)malloc(sizeof(char)*100);
     strcpy(filesystem[i],argv[i+1]);
   }
  masterThread();
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

void TraverseMap2()
{
  for(Mymap2::iterator it=map2.begin();it!=map2.end();++it)
  {
    int count=0;
    // map < unsigned char*,pair <string, bool[2]={false}> >
    //string md5string = it->first;
    pair<string,bool[2]>p = it->second;
    vector<int>v;

    if(p.second[0]==1&&p.second[1]==1)
      cout<<"FILE: "<<p.first<<" PRESENT IN BOTH FILE SYSTEMS\n";

	  else if(p.second[0]==0&&p.second[1]==1)
    {
      cout<<"FILE: "<<p.first<<" PRESENT IN FIRST FILE SYSTEM\n";
      exactcopy=0;
    }

    else {
      cout<<"FILE: "<<p.first<<" PRESENT IN SECOND FILE SYSTEM\n";
      exactcopy=0;
    }

  }
  return ;
}



unsigned long get_size_by_fd(int fd)
{
    struct stat statbuf;
    if(fstat(fd, &statbuf) < 0) exit(-1);
    return statbuf.st_size;
}



string md5(string p)
{
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

void masterThread()
{
//Initializing constants
    NUM_THREADS_COMPLETED=0;
    THREADS_REMAINING = NUM_THREADS;

# pragma omp parallel num_threads(NUM_THREADS)
    SlaveThread();

if(exactcount==filecount)
    printf("FILE SYSTEMS ARE EXACT COPIES OF EACH OTHER\n");
  return ;
}
/************************************************************************/


/*************************************SLAVE***********************************/
void *SlaveThread(){
  int count,i,index=1;
	struct direct **files;
  int threadId = omp_get_thread_num();
  char *pathname=filesystem[threadId];
  int  startIndex =strlen(pathname)+1;
  char rname[MAXPATHLEN];
  char **directories = (char**) malloc(sizeof(char*)*MAX_DIRECTORIES);
  int front=0;
  directories[front]=NULL;
  int level=0;

  while(front<=index)
  {

    if(pathname!=NULL){
      count = scandir(pathname, &files,0, alphasort);
      if(count <= 0){	printf("No files in this directory\n");exit(0);}

//parallelism can be exploited here # pragma omp parallel for
    #pragma omp parallel for
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
        #pragma omp critical(section1)
        {
          if(map1.find(p)==map1.end())
            map1[p].first=1;
          else
            map1[p].first++;
          map1[p].second[threadId]=true;
        }
      }
}

// if the thread has listed all the files in a particular level it waits until all threads have
// updated the hashmap before it can process the map and update the next map.
    else
    {
	#pragma omp barrier

  	//if(threadId==0)
	  // cout<<"###############THREADS RELEASED TO PROCESS FURTHER###############\n";
      #pragma omp critical(section2)
      {
      for(Mymap1 :: iterator it1 = map1.begin();it1!=map1.end();++it1)
      {

        pair<string,off_t> key = it1->first;
        pair<int, bool[2]> value = it1->second;

        if((value.first>=ceil((float)NUM_THREADS/2))&&(value.second[threadId]==1))
         {
           if(value.first==1)
            exactcopy=0;
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

                      if(map2.find(md5string)==map2.end())
                        map2[md5string].first = key.first;
                      map2[md5string].second[threadId]=true;
                    }
          }
      }
      }
      if(directories[index-1]!=NULL)
      {
        directories[index]=NULL;
        index++;
        level++;
      }

      //Thread will wait for all other threads to complete
        NUM_THREADS_COMPLETED++;

        if(NUM_THREADS_COMPLETED==THREADS_REMAINING)
        {
          /*last thread which completes will have to do some extra task before next level can be traversed
          1.traverse map2 to list all the files which are common:
          2.clear the hash table map1 and map2 for further use by next level*/
          TraverseMap2();
            map1.clear();
            map2.clear();
            NUM_THREADS_COMPLETED=0;
        }
    # pragma omp barrier
    }


//for the time being
    if(front==4)
      break;
    pathname=directories[front];
    front++;
  }

  #pragma omp atmoic
    THREADS_REMAINING--;

  return NULL;
}
