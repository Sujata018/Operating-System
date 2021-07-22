#include<stdio.h>
#include<errno.h>
#include<sys/types.h>
#include<dirent.h>
#include<string.h>
#include<stdlib.h>
#include<ctype.h>
#include<signal.h>

#define handle_error_en(en,msg) \
	do { errno=en; perror(msg); exit(EXIT_FAILURE); } while(0)

#define handle_error(msg) \
	do { perror(msg); exit(EXIT_FAILURE); } while(0)

#define think() \
	sleep(3)

#define eat() \
	sleep(10)

#define LEFT \
	tinfo->philosopher_num % num_threads

#define RIGHT \
	(tinfo->philosopher_num + 1) % num_threads
	
long PID[999][2]; /* global array to store process ids and parent  */

int PID_count=0;  /* stores count of proceeses populated in P */

/* This function reads parent process id (ppid) of a process from proc directory 
   Input : process id
   Output: global array P is populated with ppid for input pid                */
	
void get_procdetails(long pid){
	
	/* Get parent PID for a PID, from proc directory */ 
 	
	char path[40],*line=NULL,p[10];
	FILE* fptr;
	size_t len=0;
	int i;
	
	snprintf(path,40,"/proc/%ld/status",pid); /* open path /proc/pid/status */
	if(NULL==(fptr=fopen(path,"r")))
		handle_error_en(errno,"fopen");

	while(-1!=(getline(&line,&len,fptr))){    /* scan all lines of status file unless parent process id line is found */
		if(0==strncmp(line,"PPid:",5)){
			i=5;
			while(!isdigit(line[i])){
				i++;
			}
			while(isdigit(line[i])){  /* get the integer portion of the line as parent process id */
				strncat(p,&line[i],1);
				i++;	
			}
			PID[PID_count][0]=atoi(p); /* store pid and ppid in PID global array */
			PID[PID_count++][1]=pid;			
			break;
		}
	}
	free(line);
	fclose(fptr);
}

/* This function kills all descendant processes of a process (but not the process itself).
   Input: a process id
   Output : Kills the descendants and prints list of processes killed, in stdout
            On error, prints appropriate error message                                   */
             
int kill_descendants(long pid){

	long des[999],parent;           /* des stores all the descendants to be killed */
	int des_count=0,toFind=1,i,des_read=0,isActive=0; 
					 /* toFind=0 when all descendants are found from PID array 
					    isActive=0, if the input process is not in proc directory */
	parent=pid;
	
	while(toFind){  /* Loop until all descendants are found */
		toFind=0;
		for(i=0;i<PID_count;i++){
			if (PID[i][0]==parent){ /* add child pids to des array */
				des[des_count++]=PID[i][1];
				toFind=1;
			}
			if (PID[i][1]==pid)
				isActive=1;	/* set isActive, if process is found in proc structure */
		}
		if (des_read<des_count){
			parent=des[des_read++]; /* find children for the children too */
			toFind=1;
		}
	}	
	if (!isActive)   /* Error : the input process id is not active */ 
		printf("Process %ld is not active.\n",pid);
	else{
		if (des_count==0)
			printf("Process %ld does not have child processes.\n",pid); 
		else{    /*  if process have children, kill all descendants */
			printf("Killed: ");
			for (i=0;i<des_count;i++){
				kill (des[i],SIGTERM);
				printf("%ld ",des[i]);
			}
			printf("\n");
		}
	}
	
}

/* This is the main process.
   Usage ./killDescendants <pid>
   Kils all descendants of pid, does not kill pid itself. */ 
   
int main(int argc,char *argv[])
{
	DIR * proc;
	struct dirent* ent;
	long cur_pid;		
	
	if ((argc!=2) || (0==(atoi(argv[1])))){ /* Error if <> 1 process id is passed, or if process id not numeric */
		fprintf(stderr,"Usage %s <PID>\n" , argv[0]);
		exit(EXIT_FAILURE);
	}
	
	/*  Open proc directory */
 	
 	if (NULL==(proc = opendir("/proc")))
		handle_error_en(errno,"opendir");
	
	/* Read all PIDs from proc directory */
	
	errno=0;  /* Set errno to 0 to distinguish end of dirertory from error condition */
	while (ent = readdir(proc)){
		if (isdigit(*ent->d_name)){      /* note PID if starting with a digit */
			cur_pid=strtol(ent->d_name,NULL,10);
			get_procdetails(cur_pid);/* get details of the process in PID array */
		}
	}
	if (errno != 0)
		handle_error_en(errno,"readdir");
	
	closedir(proc);

	kill_descendants(atoi(argv[1])); /* kill descendants */
	exit(EXIT_SUCCESS);
}
