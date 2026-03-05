#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netdb.h>
#include<arpa/inet.h>
#include<sys/mman.h>
#include<signal.h>
#include<sys/wait.h>

#define PORT "3490"
#define MAX_BUF_SIZE 200
#define BACKLOG 10
#define NUM_OF_FRUITS 5
#define NUM_OF_CUSTOMERS 10

typedef struct fruit_data
{
	char name[20];
	int quan_in_stock;
	int quan_last_sold;
}fruit;

typedef struct customer_data
{
	char ip[INET6_ADDRSTRLEN];
	unsigned short int port;
}customer;
void sigchld_handler(int s)
{
	while(waitpid(-1,NULL,WNOHANG)>0);
}
void *get_in_addr(struct sockaddr_storage *ss)
{
	if(ss->ss_family == AF_INET)
		return &(((struct sockaddr_in *)ss)->sin_addr);
	else
		return &(((struct sockaddr_in6 *)ss)->sin6_addr);
}
int main()
{
	fruit *f = mmap(NULL,sizeof(fruit)*NUM_OF_FRUITS,PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS,-1,0);
	if(f == MAP_FAILED)
	{
		perror("fruit mmap failed");
		exit(1);
	}
	fruit temp[]={
		{"MANGO",60,0},
		{"ORANGE",48,0},
		{"BANANA",100,0},
		{"LICHI",200,0},
		{"GUAVA",30,0}
	};
	memcpy(f,temp,sizeof(temp));
	customer *c = mmap(NULL,sizeof(customer)*NUM_OF_CUSTOMERS,PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1,0);
	if(c == MAP_FAILED)
	{
		perror("customer mmap failed");
		exit(1);
	}
	int *cnt_custm = mmap(NULL,sizeof(int),PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if(cnt_custm == MAP_FAILED)
	{
		perror("cnt_custm mmap failed");
		exit(1);
	}
	*cnt_custm = 0;
	int *cnt_uniq_custm = mmap(NULL,sizeof(int),PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if(cnt_uniq_custm == MAP_FAILED)
	{
		perror("cnt_uniq_custm mmap failed");
		exit(1);
	}
	*cnt_uniq_custm = 0;
	fruit order;

	struct addrinfo hints, *p, *servinfo;
	struct sockaddr_storage their_addr;
	char buf[MAX_BUF_SIZE];
	int sockfd,newfd,status,yes=1,num_bytes;
	char s[INET6_ADDRSTRLEN];
	socklen_t sock_size;
	char *char_p;
	struct sigaction sa;

	memset(&hints,0,sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_socktype = SOCK_STREAM;

	if((status = getaddrinfo(NULL,PORT,&hints,&servinfo))!=0)
	{
		fprintf(stderr,"getaddrinfo:%s\n",gai_strerror(status));
		return 1;
	}

	for(p = servinfo ; p != NULL; p = p->ai_next)
	{
		if((sockfd = socket(p->ai_family,p->ai_socktype,p->ai_protocol))==-1)
		{
			perror("server:socket");
			continue;
		}

		if(setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int))==-1)
		{
			perror("setsockopt");
			exit(1);
		}

		if(bind(sockfd,p->ai_addr,p->ai_addrlen)==-1)
		{
			close(sockfd);
			perror("bind");
			continue;
		}
		break;
	}

	if(p == NULL)
	{
		fprintf(stderr,"server:failed to bind\n");
		return 2;
	}
	freeaddrinfo(servinfo);

	if(listen(sockfd,BACKLOG)==-1)
	{
		perror("listen");
		exit(1);
	}
	
	sa.sa_handler=sigchld_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD,&sa, NULL) == -1)
	{
		perror("sigaction");
		exit(1);
	}

	printf("server:Waiting for customers...\n");
	while(1)
	{
		sock_size = sizeof(their_addr);
		if((newfd = accept(sockfd,(struct sockaddr *)&their_addr,&sock_size))==-1)
		{
			perror("accept");
			exit(1);
		}
		inet_ntop(their_addr.ss_family,get_in_addr(&their_addr),s,sizeof(s));
		printf("server:Received connection request from %s\n",s);
		if(!fork())
		{
			if(*cnt_custm >=1)
			{
				printf("Transaction History\n");
				for(int i = 0;i<*cnt_custm;i++)
				{
					printf("Transaction %d	IP %s	PORT %d\n",i+1,c[i].ip,c[i].port);
				}
			}
			printf("CURRENTLY AVAILABLE STOCK\n");
			for(int i = 0;i<NUM_OF_FRUITS;i++)
			{
				printf("FUIT_NAME   %s	QUAN_IN_STOCK	%d   QUAN_LAST_SOLD   %d\n",f[i].name,f[i].quan_in_stock,f[i].quan_last_sold);
			}	
			close(sockfd);
			snprintf(buf,sizeof(buf),"WELCOME TO FRESH FRUIT STALL!!\nWe have served %d happy customers till date\nWhat would you like to have today?",*cnt_uniq_custm);
			if((num_bytes = send(newfd,buf,strlen(buf),0))==-1)
			{
				perror("server:send");
				exit(1);
			}
			printf("server:Sent %d bytes to the customer %s successfully\n",num_bytes,s);
			if((num_bytes = recv(newfd,buf,sizeof(buf),0))==-1)
			{
				perror("server:recv");
				exit(1);
			}
			printf("server:Received %d bytes from customer %s successfully\n",num_bytes,s);
			buf[num_bytes] = '\0';
			char_p = buf;
			int i=0;
			while(*char_p != ' ' && *char_p != '\t' && *char_p != '\0')
			{
				(order.name)[i]=*char_p;
				char_p++;
				i++;
			}
			(order.name)[i] = '\0';
			while(*char_p==' ' || *char_p == '\t')
			{
				char_p++;
			}
			if(*char_p == '\0')
			{
				strcpy(buf,"Please Specify Quanity");
				if((num_bytes = send(newfd,buf,strlen(buf),0))==-1)
				{	
						perror("server:send");
						exit(1);
				}
				return 1;
			}
			order.quan_in_stock = atoi(char_p);
			printf("ORDER received for %d quantities of %s\n",order.quan_in_stock,order.name); 
			int j;
			for(j = 0;j<NUM_OF_FRUITS;j++)
			{
				if(strcmp(f[j].name,order.name)==0)
				{
					if(f[j].quan_in_stock>=order.quan_in_stock)
					{
						int k;
						f[j].quan_in_stock = f[j].quan_in_stock - order.quan_in_stock;
						f[j].quan_last_sold = order.quan_in_stock;
						strcpy(c[*cnt_custm].ip,s);
						c[*cnt_custm].port = ((struct sockaddr_in *)&their_addr)->sin_port;
						for(k=0;k<*cnt_custm;k++)
						{
							if((strcmp(c[k].ip,s)==0) || c[k].port == c[*cnt_custm].port)
								break;
						}
						if(*cnt_custm==0 || k>=*cnt_custm)
							(*cnt_uniq_custm)++;	
						(*cnt_custm)++;
						strcpy(buf,"HURRAH!Your order has been delivered\nThank You for coming");
						if((num_bytes = send(newfd,buf,strlen(buf),0))==-1)
						{
							perror("server:send");
							exit(1);
						}
					}
					else
					{
						strcpy(buf,"SORRY!\n Required quantity is not available");
						if((num_bytes = send(newfd,buf,strlen(buf),0))==-1)
						{
							perror("server:send");
							exit(1);
						}
					
					}
					break;
				}
			}
			if(j>=NUM_OF_FRUITS)
			{
					strcpy(buf,"SORRY!\n The fruit is unavailable\n");
					if((num_bytes = send(newfd,buf,strlen(buf),0))==-1)
					{
						perror("server:send");
						exit(1);
					}
			}
			exit(0);
		}
		close(newfd);
	}	
	return 0;
}
					


