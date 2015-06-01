#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <netinet/tcp.h> 
#include <sys/ioctl.h>
#define EDGEONLY

#include "fifo.h"	/* for taboo list */


#define MAXSIZE (541)

#define TABOOSIZE (500)
#define BIGCOUNT (9999999)
#ifdef __APPLE__
#  define error printf
#endif
/***
 *** example of very simple search for R(7,7) counter examples
 ***
 *** starts with a small randomized graph and works its way up to successively
 *** larger graphs one at a time
 ***
 *** uses a taboo list of size #TABOOSIZE# to hold and encoding of and edge
 *** (i,j)+clique_count
 ***/

/*
 * PrintGraph
 *
 * prints in the right format for the read routine
 */
void PrintGraph(int *g, int gsize)
{
	int i;
	int j;

	fprintf(stdout,"%d\n",gsize);

	for(i=0; i < gsize; i++)
	{
		for(j=0; j < gsize; j++)
		{
			fprintf(stdout,"%d ",g[i*gsize+j]);
		}
		fprintf(stdout,"\n");
	}

	return;
}

/*
 * CopyGraph 
 *
 * copys the contents of old_g to corresponding locations in new_g
 * leaving other locations in new_g alone
 * that is
 * 	new_g[i,j] = old_g[i,j]
 */
void CopyGraph(int *old_g, int o_gsize, int *new_g, int n_gsize)
{
	int i;
	int j;

	/*
	 * new g must be bigger
	 */
	if(n_gsize < o_gsize)
		return;

	for(i=0; i < o_gsize; i++)
	{
		for(j=0; j < o_gsize; j++)
		{
			new_g[i*n_gsize+j] = old_g[i*o_gsize+j];
		}
	}

	return;
}

/*
 ***
 *** returns the number of monochromatic cliques in the graph presented to
 *** it
 ***
 *** graph is stored in row-major order
 *** only checks values above diagonal
 */



void SendBuffer(char *buffer, int buffer_size){
	int sockfd, portno, n;
	struct sockaddr_in serv_addr;
	struct hostent *server;

	portno = 9999;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	
	if(sockfd < 0)
		error("ERROR opening socket");
	
	char *host_address = "localhost";

	server = gethostbyname(host_address);

	if(server == NULL){
		fprintf(stderr, "ERROR, no such host\n");
		exit(0);
	}

	
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;

	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	
	serv_addr.sin_port = htons(portno);
	
	if(connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		error("ERROR connecting");
	
	n = write(sockfd, buffer, buffer_size);
	
	if(n < 0)
		error("ERROR writing to socket");
	
	close(sockfd);
		
}

void SendGraph(int *ramsey_g, int g_size){
	char str[4];
	sprintf(str, "%d", g_size);
	SendBuffer(str, 4);
	int buffer_size = 1024;	
	char *buffer = malloc(buffer_size*sizeof(char));
	int i, j;
	int elem_count = 0;

	for(i = 0; i < g_size; i++){
		
		for(j = 0; j < g_size; j++){
			buffer[elem_count] = ramsey_g[g_size*i + j] + '0';
			elem_count++;
			if(elem_count == 1024){
				SendBuffer(buffer, buffer_size);
				elem_count = 0;
				bzero(buffer, buffer_size);
			}	
		}
	}
	
	if(elem_count > 0)
		SendBuffer(buffer, buffer_size);
	free(buffer);

}


int* RequestGraph(int *ramsey_g, int *size_holder){
//send request to server to get an update
//get size from server
//realloc pointer to approriate size
//get graph in blocks
	int sockfd, portno, n;
	struct sockaddr_in serv_addr;
	struct hostent *server;

	portno = 9999;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if(sockfd < 0)
		error("ERROR opening socket");
	
														
	char *host_address = "localhost";

	server = gethostbyname(host_address);

	if(server == NULL){
		fprintf(stderr, "ERROR, no such host\n");
		exit(0);
	}

	
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;

	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	
	serv_addr.sin_port = htons(portno);
	
	if(connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		error("ERROR connecting");
	
	char message_buffer[4];
	sprintf(message_buffer, "%d", 309);	
	n = write(sockfd, message_buffer, 4);
	
	if(n < 0)
		error("ERROR writing to socket");
	
	char size_buffer[4];
	n = read(sockfd, size_buffer, 2);

	if(n < 0)
		error("ERROR opening socket");
	int graph_size = atoi(size_buffer);
	*size_holder = graph_size;
	fprintf(stdout, "%d\n", graph_size);
	int tries = (graph_size*graph_size / 1024) + 1;
	int *new_ramsey_g;
	new_ramsey_g = malloc(graph_size*graph_size*sizeof(int));
	fprintf(stdout, "%d\n", graph_size*graph_size*sizeof(char));		
	char *block_buffer = malloc(1024*sizeof(char));
	
	//in = read(sockfd, block_buffer, 1024);
	//if (n < 0){
	//	error("ERROR reading from host");	
	//}
	char * string_buffer = malloc(graph_size*graph_size*sizeof(char));
	int len = 0;
	ioctl(sockfd, FIONREAD, &len);
	if (len > 0) {
  		len = read(sockfd, string_buffer, len);
	}
	fprintf(stdout, "%s\n\n", string_buffer, graph_size*graph_size);	
	
	int i, j;

	for(i = 0; i < graph_size; i++){
		for(j = 0; j < graph_size; j++){
			new_ramsey_g[i*graph_size + j] = string_buffer[i*graph_size + j] - '0';
			fprintf(stdout, "%d", new_ramsey_g[i*graph_size + j]); 
		}
	
	}
	free(string_buffer);			
	close(sockfd);
	return new_ramsey_g;

}


int CliqueCount(int *g,
	     int gsize)
{
    int i;
    int j;
    int k;
    int l;
    int m;
    int n;
    int o;
    int count=0;
    int sgsize = 7;
    
    for(i=0;i < gsize-sgsize+1; i++)
    {
	for(j=i+1;j < gsize-sgsize+2; j++)
        {
	    for(k=j+1;k < gsize-sgsize+3; k++) 
            { 
		if((g[i*gsize+j] == g[i*gsize+k]) && 
		   (g[i*gsize+j] == g[j*gsize+k]))
		{
		    for(l=k+1;l < gsize-sgsize+4; l++) 
		    { 
			if((g[i*gsize+j] == g[i*gsize+l]) && 
			   (g[i*gsize+j] == g[j*gsize+l]) && 
			   (g[i*gsize+j] == g[k*gsize+l]))
			{
			    for(m=l+1;m < gsize-sgsize+5; m++) 
			    {
				if((g[i*gsize+j] == g[i*gsize+m]) && 
				   (g[i*gsize+j] == g[j*gsize+m]) &&
				   (g[i*gsize+j] == g[k*gsize+m]) && 
				   (g[i*gsize+j] == g[l*gsize+m])) {
					for(n=m+1; n < gsize-sgsize+6; n++)
					{
						if((g[i*gsize+j]
							== g[i*gsize+n]) &&
						   (g[i*gsize+j] 
							== g[j*gsize+n]) &&
						   (g[i*gsize+j] 
							== g[k*gsize+n]) &&
						   (g[i*gsize+j] 
							== g[l*gsize+n]) &&
						   (g[i*gsize+j] 
							== g[m*gsize+n])) {
					for(o=n+1; o < gsize-sgsize+7; o++) {
						if((g[i*gsize+j]
							== g[i*gsize+o]) &&
						   (g[i*gsize+j] 
							== g[j*gsize+o]) &&
						   (g[i*gsize+j] 
							== g[k*gsize+o]) &&
						   (g[i*gsize+j] 
							== g[l*gsize+o]) &&
						   (g[i*gsize+j] 
							== g[m*gsize+o]) &&
						   (g[i*gsize+j] == 
							   g[n*gsize+o])) {
			      					count++;
						   }
					}
						}
					}
				}
			    }
			}
		    }
		}
	    }
         }
     }
    return(count);
}

void
TrySolve(int *oldG,int oldGSize,int* biggest)
{
	int* newG = (int *)malloc((oldGSize+1)*(oldGSize+1)*sizeof(int));
	CopyGraph(oldG,oldGSize,newG,oldGSize+1);
	int j;
	for(j=0; j < (oldGSize+1); j++)
        {
                if(rand() % 2 == 0)
		{
			newG[j*(oldGSize+1) + oldGSize] = 0; // last column
                	newG[oldGSize*(oldGSize+1) + j] = 0; // last row
		}
		else
		{
			newG[j*(oldGSize+1) + oldGSize] = 1; // last column
                	newG[oldGSize*(oldGSize+1) + j] = 1; // last row
		}
        }
	int i=0;
	void *taboo_list = FIFOInitEdge(TABOOSIZE);
	while(1)
	{
		int count = CliqueCount(newG,oldGSize+1);
        	if(count == 0)
        	{
			if(oldGSize+1>*biggest)
			{
				*biggest=oldGSize+1;
				printf("Eureka!  Counter-example found!\n");
				PrintGraph(newG,oldGSize+1);
				if(oldGSize+1 > 60)
					SendGraph(newG,oldGSize+1);
                	}
			TrySolve(newG,oldGSize+1,biggest);
		}
		int last_count = count+1;
		int best_count = last_count+1;
		int best_i;
		for(i=0; i < (oldGSize+1); i++)
		{
			newG[i*(oldGSize+1)+oldGSize] = 1 - newG[i*(oldGSize+1)+oldGSize];
                        count = CliqueCount(newG,oldGSize+1);
			if(count==0){
				FIFOInsertEdge(taboo_list,i,oldGSize);
				if(oldGSize+1>*biggest)
                        	{
                                	*biggest=oldGSize+1;
                                	printf("Eureka!  Counter-example found!\n");
                                	PrintGraph(newG,oldGSize+1);
					if(oldGSize+1 > 60)
						SendGraph(newG,oldGSize+1);
                        	}
				TrySolve(newG,oldGSize+1,biggest);
				continue;
			}
                        if((count < best_count) && !FIFOFindEdge(taboo_list,i,oldGSize))
                        {
                        	best_count = count;
                        	best_i = i;
                        }
                        newG[i*(oldGSize+1)+oldGSize] = 1 - newG[i*(oldGSize+1)+oldGSize];
		}
		if(best_count > last_count) {
			return;
		}
		newG[best_i*(oldGSize+1)+oldGSize] = 1 - newG[best_i*(oldGSize+1)+oldGSize];
		FIFOInsertEdge(taboo_list,best_i,oldGSize);
	}
	free(newG);
}

int
main(int argc,char *argv[])
{
	
	int *g;
	int *new_g;
	int gsize;
	int count;
	int i;
	int j;
	int best_count;
	int best_i;
	int best_j;
	void *taboo_list;
        srand(time(NULL));
	gsize = 1;
	g = (int *)malloc(gsize*gsize*sizeof(int));
	if(g == NULL) {
		exit(1);
	}
	memset(g,0,gsize*gsize*sizeof(int));
	int* biggest=(int *)malloc(sizeof(int));
	int *t_size = malloc(sizeof(int));
	int* t = RequestGraph(g, t_size); //hopefully get the newest graph from server somewhere
        fprintf(stdout, "%s\n", "HELLO");
	fprintf(stdout, "%d\n", *t_size);
	*biggest = *t_size+1;
	TrySolve(t,*t_size,biggest);
        g[0]=1;
	
	TrySolve(g,1,biggest);
	return(0);
}
