/*
 ============================================================================
 Name        : audio_test.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include "soft_synth.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

static struct SYNTH_Device_t dev;
static int fh;
static void aout(int a) {
	write(fh, &a, 1);
}

int main(int argc, char** args) {
	char *fileout = "out.raw";
	if (argc > 1) {
		fileout = args[1];
	}
	printf("writing to:%s\n", fileout);
	int i = 0;
	fh = open (fileout, O_CREAT | O_RDWR | O_TRUNC, S_IREAD | S_IWRITE);
	if (fh < 0) {
		printf("file error:%s\n", strerror(errno));
		return EXIT_FAILURE;
	}
	synth_init(&dev, aout, 2, 50);
	tune_init();
	dev.gain = 256/7;
//					dev.channels[0].sequencer.map_index = 15;
//					dev.channels[1].sequencer.map_index = 15;
//					dev.channels[2].sequencer.map_index = 15;
//					dev.channels[3].sequencer.map_index = 15;

	double calc = 0;
	for (i = 0; i<150*65536;i++) {
		synth_tick(&dev);
		calc++;
		if (calc > (1<<FREQ_LOG_2)/2) {
			calc = 0;
			printf("t:%f ix:%i chA:%i chB:%i chC:%i chD:%i\n",
					(double)(i)/(double)(1<<FREQ_LOG_2),
					dev.channels[0].sequencer.map_index,
					dev.channels[0].sequencer.map[dev.channels[0].sequencer.map_index],
					dev.channels[1].sequencer.map[dev.channels[1].sequencer.map_index],
					dev.channels[2].sequencer.map[dev.channels[2].sequencer.map_index],
					dev.channels[3].sequencer.map[dev.channels[3].sequencer.map_index]
					                              );
		}
	}
	close(fh);
	printf("all ok\n");
	//sox -V -t raw -u -1 -c 1 -r 65536 sound.raw -d
	return EXIT_SUCCESS;
}
