#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include "mraa/aio.h"
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
sig_atomic_t volatile run_flag = 1;
int volatile T = 3;//time interval
mraa_aio_context adc_a0;

uint16_t adc_value = 0;
double Temp = 0.0;
time_t ct;
FILE *fp;//open local log

//client code
int sockfd, portno, n;
struct sockaddr_in serv_addr;
struct hostent *server;
pthread_t threadr, threadw;
char bufferw[256], bufferr[256];
char *buf_pt, period[128];
int status;
int stopflag=0, default_temp=1;
void *read_fun(void* arg);
void *write_fun(void* arg);
void do_interrupt(int sig) {
	if (sig = SIGINT)
		run_flag = 0;
}
int buf_len = 0;
int main(int argc, char *argv[])
{
	buf_pt = &bufferr[0];
	if (argc < 3) {
		fprintf(stderr, "usage %s hostname port\n", argv[0]);
		exit(0);
	}

	portno = atoi(argv[2]);

	/* Create a socket point */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("ERROR opening socket");
		exit(1);
	}

	server = gethostbyname(argv[1]);
	if (server == NULL) {
		fprintf(stderr, "ERROR, no such host\n");
		exit(0);
	}

	bzero((char *)&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(portno);

	/* Now connect to the server */
	if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
		perror("ERROR connecting");
		exit(1);
	}
	adc_a0 = mraa_aio_init(0);

	if (adc_a0 == NULL) {
		return 1;
	}
	// end client code
	//2 threads
	if(pthread_create(&threadw, NULL, write_fun, NULL)!=0){
		exit(2);
	}
	if(pthread_create(&threadr, NULL, read_fun, NULL)){
	exit(2);
	}
	if (pthread_join(threadr, NULL) != 0)
		exit(3);
	if (pthread_join(threadw, NULL) != 0)
		exit(3);
	while (1);
	return 0;
}


void *write_fun(void* arg) {
	fp = fopen("log2.txt", "w");
	strcpy(bufferw, "804778158\n");
	write(sockfd, bufferw, strlen(bufferw));
	while (run_flag) {
		if(!stopflag){
		adc_value = mraa_aio_read(adc_a0);
		Temp = log(((10240000 / adc_value) - 10000));
		Temp = 1 / (0.001129148 + (0.000234125 + (0.0000000876741 * Temp * Temp))* Temp);
		Temp = Temp - 273.15;  
		if(default_temp)
		Temp = (Temp * 9.0) / 5.0 + 32.0;
		ct = time(NULL);
		struct tm c_tm = *localtime(&ct);
		sprintf(bufferw, "804778158 TEMP=%.1f\n", Temp);
		write(sockfd, bufferw, strlen(bufferw));
		fprintf(fp, "%02d:%02d:%02d %.1f\n",c_tm.tm_hour, c_tm.tm_min, c_tm.tm_sec, Temp);
		fflush(fp);

		sleep(T);
		}
	}
}

void *read_fun(void* arg) {
	while (1) {
		//judge
		memset(bufferr, 0, sizeof(bufferr));
		read(sockfd, bufferr, 32);
			if (bufferr == NULL)
				continue;
		if (!strcmp(bufferr, "OFF")) {
			fprintf(fp, "%s\n", bufferr);
			fflush(fp);
			fclose(fp);
			mraa_aio_close(adc_a0);
			exit(0);
		}
		else if(!strcmp(bufferr, "STOP")){
			stopflag = 1;
		}
		else if (!strcmp(bufferr, "START")) {
			stopflag = 0;
		}
		else if (!strncmp(bufferr, "SCALE=",6)&&strlen(bufferr)==7) {
			if (bufferr[6] == 'F') {
				default_temp = 1;
			}
			else if (bufferr[6] == 'C') {
				default_temp = 0;
			}
			else {
				sprintf(bufferr, "%sI\n", bufferr);
			}
		}
		else if (!strncmp(bufferr, "PERIOD=", 7)) {
			memcpy(period, buf_pt + 7, strlen(bufferr) - 6);

			int k=atoi(period);
			if (k <= 3600 && k >= 1) {
				T = k;
			}
			else {
				sprintf(bufferr, "%sI\n", bufferr);
			}
		}
		else {
			sprintf(bufferr, "%sI\n", bufferr);
		}
		fprintf(fp, "%s\n", bufferr);
		fflush(fp);
		}


	}
	
//make && make run
