#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <sys/stat.h>
#include <sys/sendfile.h>
#include <netinet/in.h>
#include <fcntl.h>

#include <pthread.h>

#define MAX_LINE 511

typedef struct {
	char* file_name;
	int serv_sock;
	unsigned short port;
	struct sockaddr_in* serv_addr;

}val;

void* recieve_file(void* data);

int main(int argc, char *argv[]){
	
	int serv_sock;
	struct sockaddr_in serv_addr;
	char buf[100];
	int str_len;
	
	if(argc!=3){
		printf("Usage : %s <IP> <port>\n",argv[0]);
		exit(1);
	}
	
	serv_sock=socket(PF_INET,SOCK_STREAM,0);
	memset(&serv_addr,0,sizeof(serv_addr));
	serv_addr.sin_family=AF_INET;
	serv_addr.sin_addr.s_addr=inet_addr(argv[1]);
	serv_addr.sin_port=htons(atoi(argv[2]));
	
	connect(serv_sock,(struct sockaddr *)&serv_addr,sizeof(serv_addr));

	char message[20]="Hello_Server!";
	send(serv_sock,message,20,0);
	

	int ls_size;

	recv(serv_sock,&ls_size,sizeof(int),0);
	char* ls_file=malloc(ls_size);


	recv(serv_sock,ls_file,ls_size,0);
	
	int ls_FD;
	char ls_name[20]="temp.txt";
	while(1){
		ls_FD=open(ls_name,O_CREAT|O_EXCL|O_WRONLY,0666);
		if(ls_FD==-1){
			sprintf(ls_name+strlen(ls_name),"_1");			
		}else break;
	}
	write(ls_FD,ls_file,ls_size);
	char command[20]="cat ";
	strcat(command,ls_name);
	printf("\n------list of server file------\n");
	system(command);
	printf("-------------------------------\n");

	printf("\ntype a file name and press 'Enter' \n Type 'x' when you want to start transfering files.\n");
	int file_num=0;	
	char* file_list[10];
	while(1){
		char* menu=(char*)malloc(sizeof(char)*30);
		printf("input : ");
		scanf("%s",menu);
		if(menu[0]=='x') {
			send(serv_sock,menu,sizeof(char)*30,0);		
			break;
		}else{
			send(serv_sock,menu,sizeof(char)*30,0);
			file_list[file_num++]=menu;
		}
	}
	pthread_t p_thread[10];
	for(int t=0;t<file_num;t++){
		val* data=(val*)malloc(sizeof(data));
		data->file_name=file_list[t];
		data->serv_sock=serv_sock;
		data->serv_addr=&serv_addr;
		recv(serv_sock,&(data->port),sizeof(unsigned short),0);
		pthread_create(&p_thread[t],NULL,recieve_file,(void*)data);
	}
	for(int t=0;t<file_num;t++){
		pthread_join(p_thread[t],NULL);
	}
	close(serv_sock);
	return 0;
}


void* recieve_file(void* data){
	val* file_data=(val*)data;
	int sv_sock;
	struct sockaddr_in* serv_addr=file_data->serv_addr;
	struct sockaddr_in sv_addr;
	char buf[100];
	int str_len;

	unsigned short port=file_data->port;
	sv_sock=socket(PF_INET,SOCK_STREAM,0);
	memset(&sv_addr,0,sizeof(sv_addr));
	sv_addr.sin_family=AF_INET;
	sv_addr.sin_addr.s_addr=serv_addr->sin_addr.s_addr;
	sv_addr.sin_port=port;
	connect(sv_sock,(struct sockaddr *)&sv_addr,sizeof(sv_addr));



	char filename[30];
	int size;
	recv(sv_sock,&size,sizeof(int),0);
	recv(sv_sock,filename,sizeof(char)*30,0);
	printf("filename : %s / file size : %d byte\n",filename,size);

	char* f=malloc(size);
 	char* ptr=f;

	int remain=size;
	int download=0;
	int percent=0;
	while(0<remain){
		int recvsize=0;
		if(remain<1460){
			recvsize=recv(sv_sock,ptr,remain,0);
		}else{
			recvsize=recv(sv_sock,ptr,1460,0);
		}
		if(percent<download/(size/100)){
			printf("%s : download : %d%%\n",filename,++percent);
		};
		
		ptr+=recvsize;
		download+=recvsize;
		remain-=recvsize;
	}



	int fd;
	while(1){
		fd=open(filename,O_CREAT|O_EXCL|O_WRONLY,0666);
		if(fd==-1){
			sprintf(filename+strlen(filename),"_1");			
		}else break;
	}
		

	printf("fd : %d / size = %d\n",fd,size);
	write(fd,f,size);
	close(fd);

	struct stat downloadfile;
	stat(filename,&downloadfile);	
	int download_size=downloadfile.st_size;
	if(download_size==size){
		printf("download_size : %dbyte \nfile size : %dbyte \n--perfect--\n",download_size,size);
	}else{
		printf("download_size : %dbyte \nfile size : %dbyte \n--%dbyte is not downloaded--\n",download_size,size,size-download_size);
	}
	close(sv_sock);
}