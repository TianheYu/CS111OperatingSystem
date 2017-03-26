#include <sys/socket.h>
#include <resolv.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include "mraa/aio.h"
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>

#include <pthread.h>
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

int T = 3;//time interval
mraa_aio_context adc_a0;

uint16_t adc_value = 0;
double Temp = 0.0;
time_t ct;
FILE *fp;//open local log

//client code


pthread_t threadr, threadw;
char bufferw[512], bufferr[512];
char *buf_pt, period[128];
int status;
int stopflag=0, default_temp=1;
void *read_fun(void* arg);
void *write_fun(void* arg);
int buf_len = 0;
SSL_CTX *ctx;
SSL *ssl;
int server = 0;
int sockfd;
char hostname[256] = "";
char      *tmp_ptr = NULL;
int           port;
struct hostent *host;
struct sockaddr_in dest_addr;

int main(int argc, char *argv[])
{
	//These function calls initialize openssl for correct work
	OpenSSL_add_all_algorithms();
	ERR_load_BIO_strings();
	ERR_load_crypto_strings();
	SSL_load_error_strings();

	//Create the Input/Output BIO's

	//initialize SSL library and register algorithms 
	SSL_library_init();
	//Try to create a new SSL context    
	ctx = SSL_CTX_new(TLSv1_client_method());
	SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2);
	//Create new SSL connection state object              
	ssl = SSL_new(ctx);
	//Make the underlying TCP socket connection 
	port = atoi(argv[2]);
	host = gethostbyname(argv[1]);

	//create the basic TCP socket 
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(port);
	dest_addr.sin_addr.s_addr = *(long*)(host->h_addr);
	memset(&(dest_addr.sin_zero), '\0', 8);//Zeroing the rest of the struct
	tmp_ptr = inet_ntoa(dest_addr.sin_addr);

	//Try to make the host connect here 
	connect(sockfd, (struct sockaddr *) &dest_addr, sizeof(struct sockaddr));
	server = sockfd;
	
	// Attach the SSL session to the socket descriptor 
	SSL_set_fd(ssl, server);

	//Try to SSL-connect here, returns 1 for success
	SSL_connect(ssl);
	//connection setup ends here.

	buf_pt = &bufferr[0];
	adc_a0 = mraa_aio_init(0);
	if (adc_a0 == NULL) {
		return 1;
	}
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
	fp = fopen("log3.txt", "w");
	strcpy(bufferw, "804778158\n");
	SSL_write(ssl, bufferw, strlen(bufferw));
	while (1) {
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
		SSL_write(ssl, bufferw, strlen(bufferw));
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
		SSL_read(ssl, bufferr, 256);
			if (bufferr == NULL)
				continue;
		if (!strcmp(bufferr, "OFF")) {
			fprintf(fp, "%s\n", bufferr);
			fflush(fp);
			fclose(fp);
			mraa_aio_close(adc_a0);
			//free
			SSL_free(ssl);
			close(server);
			SSL_CTX_free(ctx);

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
