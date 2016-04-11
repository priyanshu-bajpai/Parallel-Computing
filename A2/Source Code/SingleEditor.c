#include "mpi.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#define NO_OF_TELECAST 2
#define NO_OF_REPORTS 5
#define NO_OF_UPDATES 7
#define NO_OF_TOPICS 3

//int editor_to_comm = (rank-NO_OF_EDITORS)/REPORTERS_PER_EDITOR;
//+rank*7

typedef unsigned long long ull;
//global variables for editor.
int NumProcessesComplete=0;
int update_to_tele[NO_OF_TOPICS];	// only updated in mukhiya
int topicLeader[NO_OF_TOPICS];

typedef struct{
	char* t_name;
	char* u_name[NO_OF_UPDATES];
	int valid;
}t_record;

t_record** file;

void print_file(t_record* file){
	int i,j;
	for(i=0;i<NO_OF_TOPICS;i++){
		printf("\nValid bit: %d t_name: %s\n",file[i].valid,file[i].t_name);
		for(j=0;j<NO_OF_UPDATES;j++){
			printf("\nupdate: %s\n",file[i].u_name[j]);
		}
	}
	return;
}	

void populate(int tele_no,t_record** file){
	char* str= malloc(200);
	char str2[50];
	sprintf(str, "%d", tele_no);
	strcpy(str2, "news_");
	strcat(str2,str);
	strcat(str2,".txt");
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    //printf("\nfile name is %s\n", str2);
	FILE* fp=fopen(str2,"r");
	if (fp == NULL)
       exit(EXIT_FAILURE);
   	int i,j,x;
  	for(i=0;i<NO_OF_TOPICS;i++)
  	{
  		file[i]->t_name=malloc(200);
  		getline(&str, &len, fp);
  		x = strlen(str);
  		if(str[x-1]=='\n') str[x-1] = 0;
  		strcpy(file[i]->t_name,str);
  		getline(&str,&len,fp);
  		x = strlen(str);
  		if(str[x-1]=='\n') str[x-1] = 0;
  		file[i]->valid=atoi(str);
  		for(j=0;j<NO_OF_UPDATES;j++){
  			getline(&str, &len, fp);
  			x = strlen(str);
  			if(str[x-1]=='\n') str[x-1] = 0;
			file[i]->u_name[j]=strdup(str);
  		}
  	} 
  	//print the file
  	//print_file(file);

  	fclose(fp);
}

int num_reporters(ull n){
	unsigned int count = 0;
  	while(n){
    	count += n & 1;
    	n >>= 1;
  	}
  	return count;
}

char* get_msg(int t_id, int u,t_record** file){
	printf("\ninput to get_msg function, t_id: %d, u_id: %d\n", t_id, u);
	char* ret=malloc(200);
	char temp[15];
	sprintf(ret, "%d#", file[t_id]->valid);
	sprintf(temp,"%d#",t_id);
	strcat(ret,temp);
	strcat(ret,file[t_id]->t_name);
	strcat(ret,"#");
	strcat(ret,file[t_id]->u_name[u]);
	strcat(ret,"#");

	
	//printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@ %s",ret);
	printf("\noutput of getmsg: %s\n", ret);
	return ret;

}

char* get_msg2(int topic_no, int update_no){
	char* ret=malloc(100);
	char temp[20];
	sprintf(ret,"%d#",topic_no);
	sprintf(temp,"%d",update_no);
	strcat(ret,temp);
	strcat(ret,"#");
	//printf("getmessage mesg = %s\n",ret);
	return ret;
}

bool check_valid(int topic_id,int valid_bit,int** tele){
	int i=0;
	while(tele[topic_id][i]!=-1)
	{
		
		if(tele[topic_id][i]== valid_bit){
			printf("CHECK VALID returns false******#$$$$$$$$$$$\n");
			return false;
		}
		i++;
	}
	tele[topic_id][i]=valid_bit;
	//printf("CHECK VALID returns true******#$$$$$$$$$$$\n");
	return true;
}


/////////////

void* lead(void* x){
	int rank=*((int*)x);
	int l,q,p;
	MPI_Status Stat;
	MPI_Request request;
	//printf("%d became leader for topic %d\n",rank,topic_no);
	//leader=true;
	// spawn a thread here // only one thread to listen to queries for all topics
	//pthread_t= pth;
	//pthread_create(&pth,f,NULL);
	char* u= malloc(100);
	int temp,z,n_lead=0;//count=0;
	int t_id,u_id,flag;
	bool r_working[NO_OF_TOPICS][NO_OF_UPDATES];
	for(p=0;p<NO_OF_TOPICS;p++)
		for(q=0;q<NO_OF_UPDATES;q++)
			r_working[p][q] = false;

	//int for_breakin = 0;
	while(1)
	{
		// tag no 4 repersents comm between reporter to leader.
		
		//printf("\nprocess %d waiting to recieve message\n",rank);						
		//if(for_breakin==1) for_breakin++;
		MPI_Recv(u, 100, MPI_CHAR, MPI_ANY_SOURCE,4, MPI_COMM_WORLD, &Stat);
		// tag 4 used when reporter asks to continue on a update.
		//Enter this loop when editor sends a message to leader
		if(Stat.MPI_SOURCE==0)
		{	
			for(z=0;z<NO_OF_TOPICS;z++){
				if(topicLeader[z]==rank){
					n_lead++;
				}
			}

			t_id = atoi(u);
			MPI_Send(get_msg(t_id,update_to_tele[t_id],file), 100, MPI_CHAR,0,3, MPI_COMM_WORLD);
			//printf("recieved finally from editor to : %d ************************\n",rank);

			for(l=0;l<n_lead-1;l++)
			{
				printf("###################LEAD FOR PROCESS %d is %d and value of l = %d##############\n",rank,n_lead,l);
				MPI_Recv(u, 100, MPI_CHAR, MPI_ANY_SOURCE,4, MPI_COMM_WORLD, &Stat);
				t_id = atoi(u);
		// check this  get_msg = fseek and get the record from the file searated by #(valid_bit#topic_id#topic_String#,update_String);
				MPI_Send(get_msg(t_id,update_to_tele[t_id],file), 100, MPI_CHAR,0,3, MPI_COMM_WORLD);
			//	printf("###################BACKCHODDDDDDDDDDDDDD process %d##############\n",rank);
			//	printf("sent final update of topic %d to editor\n",t_id);

			}

			//printf("process %d breaking yayyyyyyyyyyyyyyyyyyyyyyy\n",rank);
			//for_breakin++;
			//if(for_breakin > 1)
			break;
		}	

		else
		{
			printf("\n******LEADER %d Recieved string %s from process %d**********\n",rank,u,Stat.MPI_SOURCE);
			t_id=atoi(strtok(u,"#"));
			u_id=atoi(strtok(NULL,"#"));
			//printf("TID %d UID%d\n",t_id,u_id);
			// tag 5 is used to indicate to the reporter to work or not.
			if(r_working[t_id][u_id]){
				temp=0;
				MPI_Isend(&temp, 1, MPI_INT,Stat.MPI_SOURCE, 5, MPI_COMM_WORLD,&request);
			}
			else
			{
				temp=1;
				MPI_Isend(&temp, 1, MPI_INT,Stat.MPI_SOURCE, 5, MPI_COMM_WORLD,&request);
				r_working[t_id][u_id]=true;
				if(u_id > update_to_tele[t_id]){
					update_to_tele[t_id]=u_id;
				}
			}
		}
		
	}
	printf("\n@@@@@@@@@@@@@@Thread of rank : %d exits@@@@@@@@@@@@@@@@@@\n",rank);
	fflush(stdout);
	return;
}



int main(int argc, char *argv[]){
	int size, rank, dest, source, rc, count, tag=0;  
	char* inmsg;
	int i,j,pr;
	ull outmsg;
	MPI_Status Stat;
	int leader_list[NO_OF_TOPICS];
	for(i=0;i<NO_OF_TOPICS;i++){
		leader_list[i]=-1;
	}
	//
	pthread_t pt;
	
	int valid_id[NO_OF_TOPICS];
	file = (t_record**)malloc(sizeof(t_record*)*NO_OF_TOPICS);

	for(i=0;i<NO_OF_TOPICS;i++) 
		file[i]=(t_record*)malloc(sizeof(t_record));
	
	MPI_Init_thread(&argc,&argv,MPI_THREAD_MULTIPLE,&pr);
/*	if(pr==MPI_THREAD_MULTIPLE){
		printf("\ngiven what is asked\n");
	}*/

	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	int tele_no=0,in;
	int val = size-1;
	MPI_Request request;
	


	if (rank == 0) {
		int** tele = malloc(NO_OF_TOPICS* sizeof(int*));
		for(i=0;i<NO_OF_TOPICS;i++){
			tele[i]=malloc(sizeof(int)*NO_OF_TELECAST);
		}	
		inmsg = malloc(1000);

		while(tele_no < NO_OF_TELECAST){	
			

			//initialsations
			for(i=0;i<NO_OF_TOPICS;i++){
				topicLeader[i]=-1;
			}

			for(i=0;i<NO_OF_TOPICS;i++)
				for(j=0; j<NO_OF_TELECAST;j++)
					tele[i][j] = -1;

			//
			while(NumProcessesComplete <val)
			{	printf("\ncheck here\n");
			  	rc = MPI_Recv(&in, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &Stat);
								
				if(in!=-1){
					printf("\n Editor recieved signal from  reproter %d for topic %d\n", Stat.MPI_SOURCE,in);
					int topic = in;


					//

					if(topicLeader[topic]==-1) 
					{
						topicLeader[topic] = Stat.MPI_SOURCE;
						/*int head =0,flag2=1;
						while(leader_list[head]!=-1)
						{
							if(leader_list[head]==Stat.MPI_SOURCE)
								flag2=0;
							head++;
						}
						if(flag2)
						{	
							leader_list[head]=Stat.MPI_SOURCE;
							val--;
						}*/

						printf("$$$$$$$$$MAking process %d leader for topic %d Value of val is %d\n",Stat.MPI_SOURCE,topic,val);
						
					}

					//


						/*
					if(topicLeader[topic]==-1) 
					{
						topicLeader[topic] = Stat.MPI_SOURCE;
						val--;
					}*/
					//printf("\ntopic updated to be %d\n", Stat.MPI_SOURCE);
					//info about leader sent by editor back to the reporter tag 6 
					MPI_Isend(&topicLeader[topic], 1, MPI_INT, Stat.MPI_SOURCE, 6, MPI_COMM_WORLD, &request);
					//printf("\ninfo about leader sent by editor back to the %d reporter tag 6\n",Stat.MPI_SOURCE);
				}

				//termination request 
				else if(in == -1){
					printf("\ntermination request\n");
					NumProcessesComplete++;
				}
				
			 // printf("\nmessage Received: %s from %d\n", inmsg, Stat.MPI_SOURCE);

			}



			int count_leaders=0;
			for(i=0;i<NO_OF_TOPICS;i++)
			{
				if(topicLeader[i]!=-1)
				{
					char *string = malloc(10);
					sprintf(string,"%d",i);
					MPI_Isend(string, 10, MPI_CHAR, topicLeader[i], 4, MPI_COMM_WORLD,&request);
					count_leaders++;
				}
			}

			//printf("NO OF LEADERS  = %d\n",count_leaders);

			i=0;

			while(i<count_leaders)
			{
				int x;
				// TAG 3 represents msg sent by leaders for telecast
				rc = MPI_Recv(inmsg, 1000, MPI_CHAR, MPI_ANY_SOURCE, 3, MPI_COMM_WORLD, &Stat);
				// This can taken to new thread
				
				//printf("\nThis is imsg : %s \n",inmsg);
				int valid_bit = atoi(strtok(inmsg , "#"));

				//tokenize by #
				
				int topic_id = atoi(strtok(NULL , "#"));
				char* a,*b;
				a=strtok(NULL, "#");
				b=strtok(NULL, "#");
				//printf("Recieved from reporter %d****************************************************\n",Stat.MPI_SOURCE);
				
				if(check_valid(topic_id,valid_bit,tele))
					printf("Topic:%s  Update:%s",a,b);			
				
				i++;
			}
			printf("\ntele_no is updated, editor\n");
			tele_no++;
			MPI_Barrier(MPI_COMM_WORLD);
		}
	} 







	//Reporters will enter this loop
	else if (rank != 0) {
		int topic_no;
		int update_no;
		int p,q,k,l;
		bool leader=false;
		int recent_update[NO_OF_TOPICS]; //	with every reporter used in random update selection
		 
		
		srand(time(NULL) + rank);
		//
		int tf=0;
		//
		while(tele_no < NO_OF_TELECAST)
		{
			populate(tele_no,file);

			//Initializing Variables;
			for(p=0;p<NO_OF_TOPICS;p++) 
			{
				recent_update[p] = -1;
				update_to_tele[p] = -1; // reason for segfault.
				topicLeader[p]=-1;
			}

			int r_timestamp=0;
			
			
			for(k=0;k<NO_OF_REPORTS;k++)
			{	
				printf("\n++++++++++++++++++++++++++++++++++++++++new report\n");
				topic_no=rand()%(NO_OF_TOPICS);
				if(recent_update[topic_no]==-1)
					update_no=0;
					//update_no=rand()%(NO_OF_UPDATES);  // check this
				
				else
				{
					if(recent_update[topic_no]<NO_OF_UPDATES-1)
					{
						//printf("\nrt :%d ru:%d\n",r_timestamp,recent_update[topic_no]);
						if(r_timestamp>=NO_OF_UPDATES)
							update_no=(rand())%(NO_OF_UPDATES-recent_update[topic_no]-1) + recent_update[topic_no]+1;	
						else
							update_no=(rand())%(r_timestamp-recent_update[topic_no]) + recent_update[topic_no] +1;	
						
							
					}	
					
					
				}
				printf(" topic no is :%d update no. is %d\n",topic_no, update_no);
				recent_update[topic_no]=update_no;

				if(topicLeader[topic_no]==-1)
				{
					MPI_Send(&topic_no, 1, MPI_INT,0,0, MPI_COMM_WORLD);
					printf("\nReporter %d asking editor for leader of topic %d\n",rank, topic_no);
					MPI_Recv(&topicLeader[topic_no], 1, MPI_INT, 0, 6, MPI_COMM_WORLD, &Stat);
					printf("\nMy rank = %d, leader for topic %d is  %d\n",rank, topic_no,topicLeader[topic_no]);
				}



				/////////////////////Leader enters this loop///////////////////////			
					// ASSUMPTION WITHOUT THREADS : MUkhiya can be of one topic only.
				

				if(topicLeader[topic_no]==rank )
				{	
					leader=true;
					///
					update_to_tele[topic_no]=update_no;

					if(tf==0){
						printf("\n*************** New thread is created here for mukhiya %d ***************************\n",rank);
						pthread_create(&pt,NULL,lead,&rank);
						//thread_used=pt[thread_no++];
						tf=1;
					}
				}	

				//////////ordinary reporter will enter this loop///////////////////////////			
				else if(topicLeader[topic_no]!=rank)
				{	//getmsg2() = topic_id#update_id
					int t;
					printf("\nelse part+++++++++++++++++++++++++++++++++++++++++++++++\n");
					//printf("Reporter id %d is ordinary reporter, \n",rank);
					char *mystr = get_msg2(topic_no,update_no);
					printf("\nprocess %d sending message %s to leader %d for topic %d and update %d\n",rank,mystr,topicLeader[topic_no],topic_no,update_no);
					rc = MPI_Send(mystr, 100, MPI_CHAR, topicLeader[topic_no],4, MPI_COMM_WORLD);
					if (rc != MPI_SUCCESS) {
     					printf ("Error starting MPI program. Terminating.\n");
     					MPI_Abort(MPI_COMM_WORLD, rc);
     				}
					//printf("\n** THIS LINE SHOULDNT PRINT Sent request for topic %d\n",topic_no);
					MPI_Recv(&t, 1, MPI_INT,topicLeader[topic_no], 5, MPI_COMM_WORLD, &Stat);
					printf("\n timestamp = %d for reporter = %d\n", r_timestamp, rank);
				}
				printf("\nstarting new report +++++++++++++++++++++++++++++++++++++++++==\n");
				r_timestamp++;		// required for taking records randomly from the file;

			}	
			
			int termination=-1;
			/////

			/*if(!leader  && r_timestamp==NO_OF_REPORTS)
			{
				printf("--------------------------PROCESS %d sending termination message to editor\n",rank);
				MPI_Send(&termination, 1, MPI_INT, 0,0, MPI_COMM_WORLD);
			}
			else if(!leader) 
				printf("\nKUCH BAKCHODI HAI\n");*/
			
			/////
			printf("--------------------------PROCESS %d sending termination message to editor\n",rank);
			MPI_Send(&termination, 1, MPI_INT, 0,0, MPI_COMM_WORLD);

			tele_no++;
			//
			if(leader){
				printf("\n@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ came here\n");
				pthread_join(pt,NULL);
			}


			printf("\nhhhhhhhhhhhhhhhhhhhhhtele_no is updated, reporter %dhhhhhhhhhhhhhhhhhh\n",rank);
			fflush(stdout);
			MPI_Barrier(MPI_COMM_WORLD);
			printf("Reporter %d going for next telecast\n",rank);
		}
	}

	MPI_Finalize();

	return 0;
}
