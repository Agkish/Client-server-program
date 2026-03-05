#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netdb.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<errno.h>

#define PORT "3490"
#define MAX_BUF_SIZE 200

void *get_in_addr(struct sockaddr *sa)
{
	if(sa->sa_family==AF_INET)
		return &(((struct sockaddr_in *)sa)->sin_addr);
	else
		return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
	struct addrinfo hints,*p,*servinfo;
	int status,sockfd,num_bytes;
	char s[INET6_ADDRSTRLEN];
	char buf[MAX_BUF_SIZE];

	if(argc!=2)
	{
		fprintf(stderr,"usage:client hostname\n");
		exit(1);
	}	
	memset(&hints,0,sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if((status = getaddrinfo(argv[1],PORT,&hints,&servinfo))!=0)
	{
		fprintf(stderr,"getaddrinfo: %s\n",gai_strerror(status));
		return 1;
	}

	for(p = servinfo;p!=NULL;p=p->ai_next)
	{
		if((sockfd = socket(p->ai_family,p->ai_socktype,p->ai_protocol))==-1)
		{
			perror("client:socket\n");
			continue;
		}
		if(connect(sockfd,p->ai_addr,p->ai_addrlen)==-1)
		{
			close(sockfd);
			fprintf(stderr,"client:failed to connect\n");
			exit(1);
		}
		break;
	}
	if(p==NULL)
	{
		fprintf(stderr,"client:failed to connect\n");
		return 2;
	}

	if((inet_ntop(p->ai_family,get_in_addr(p->ai_addr),s,sizeof(s)))==NULL)
	{
		fprintf(stderr,"client:inet with error number %d\n",errno);
		exit(1);
	}
	printf("client:Connecting to %s\n",s);

	freeaddrinfo(servinfo);
 	if((num_bytes = recv(sockfd,buf,sizeof(buf)-1,0)) == -1)
	{
		perror("client:recv");
		exit(1);
	}
	buf[num_bytes] = '\0';
	printf("client:Message of size %d received from %s:\n",num_bytes,s);
	printf("%s:",buf);
	scanf("%[^\n]",buf);
	if((num_bytes = send(sockfd,buf,strlen(buf),0))==-1)
	{
		perror("client:send\n");
		exit(1);
	}
	printf("client:Sent %d bytes to %s successfully\n",num_bytes,s);
 	if((num_bytes = recv(sockfd,buf,sizeof(buf)-1,0)) == -1)
	{
		perror("client:recv");
		exit(1);
	}
	buf[num_bytes] = '\0';
	printf("client:Message of %d bytes received from %s:\n%s\n",num_bytes,s,buf);
	close(sockfd);
	return 0;
}

	


