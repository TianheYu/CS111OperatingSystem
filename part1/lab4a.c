#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include "mraa/aio.h"
#include <math.h>
#include <time.h>
sig_atomic_t volatile run_flag = 1;

void do_interrupt(int sig) {
	if (sig = SIGINT)
		run_flag = 0;
}
int main()
{
	mraa_aio_context adc_a0;
	adc_a0 = mraa_aio_init(0);
	uint16_t adc_value = 0;
	double Temp = 0.0;
	time_t ct;
	FILE *fp;
	fp = fopen("log1.txt", "w");
	struct tm c_tm = *localtime(&ct);
	if (adc_a0 == NULL) {
		return 1;
	}
	while(run_flag) {
		adc_value = mraa_aio_read(adc_a0);

		Temp = log(((10240000 / adc_value) - 10000));
		Temp = 1 / (0.001129148 + (0.000234125 + (0.0000000876741 * Temp * Temp))* Temp);
		Temp = Temp - 273.15;              // Convert Kelvin to Celsius
		Temp = (Temp * 9.0) / 5.0 + 32.0;
		ct = time(NULL);
		struct tm c_tm = *localtime(&ct);
		
		fprintf(stdout, "%02d:%02d:%02d %.1f\n", c_tm.tm_hour, c_tm.tm_min, c_tm.tm_sec, Temp);
		fflush(stdout);
		fprintf(fp, "%02d:%02d:%02d %.1f\n", c_tm.tm_hour, c_tm.tm_min, c_tm.tm_sec, Temp);
		fflush(fp);
		sleep(1);
	}
	mraa_aio_close(adc_a0);
	fclose(fp);
	return 0;
}