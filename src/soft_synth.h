/*
 * soft_synth.h
 *
 *  Created on: Feb 11, 2012
 *      Author: petera
 */

#ifndef SOFT_SYNTH_H_
#define SOFT_SYNTH_H_

#include "soft_synth_wfseq.h"
#include "soft_synth_seq.h"

#define FREQ_LOG_2 	15
#define ACC_F		8
#define ACC_O 		8
#define CHANNELS 	4

typedef void (*synth_set_dac_f)(int out);

struct SYNTH_Sequencer_t {
	short** envelope_waveform_sequences;	//16
	short** tremolo_waveform_sequences;		//16
	short** chord_waveform_sequences;  		//16
	short** common_waveform_sequences; 		//32
};

struct SYNTH_Sequencer_Channel_t {
	short envelope;
	short tremolo;
	short chord;
	char waveform;
	char flags;
	short timer;
	short track_index;
	short map_index;
	short map_length;
	short* map;
	short** tracks;
};

enum synth_waveform_property_e {
	TONE = 0,
	FREQUENCY,
	VOLUME,
	MODULATION_FACTOR,
	MODULATION_DELTA,
	NOISE_FACTOR,
	NOISE_FREQUENCY,
	HIPASS_FACTOR,
	LOPASS_FACTOR,
	WAVE_SEL,
	WAVEFORM_PROPERTIES // end indicator index
};

struct SYNTH_Waveform_Property_Sequence_t {
	short* sequence;
	short timer;
	short index;
	char loop;
};

struct SYNTH_Waveform_Sequencer_t {
	int mask;
	struct SYNTH_Waveform_Property_Sequence_t prop_seq[WAVEFORM_PROPERTIES];
};

typedef short synth_prop_t;

struct SYNTH_Channel_t {
	synth_prop_t tone;
	synth_prop_t f;
	synth_prop_t vol;

	synth_prop_t mod_fact;
	synth_prop_t mod_f_delta;

	synth_prop_t noise_fact;
	synth_prop_t noise_freq;

	synth_prop_t hipass_fact;
	synth_prop_t lopass_fact;

	synth_prop_t wavesel;

    int f_count;
    int f_mod_count;
    int zout;
    int noise_out;
    int noise_count;
    int gain;
    struct SYNTH_Waveform_Sequencer_t waveform_sequencer;
    struct SYNTH_Sequencer_Channel_t sequencer;
};

struct SYNTH_Device_t {
	int gain;
	int sequenceTimer;
	int sequenceDiv;
	int waveformTimer;
	int waveformHz;
	struct SYNTH_Channel_t channels[CHANNELS];
	struct SYNTH_Sequencer_t sequencer;
	synth_set_dac_f dac_f;
};


void synth_init(struct SYNTH_Device_t* pDev, synth_set_dac_f f, int sequenceHz, int waveformHz);
void synth_sequencer_init(struct SYNTH_Device_t* pDev, short** envelopes, short** tremolos, short** chords, short** commons);
void synth_channel_sequencer_init(struct SYNTH_Device_t *pDev, int channel, short trackmapLen, short* trackmap, short** tracks);
void synth_tick(struct SYNTH_Device_t* pDev);

#endif /* SOFT_SYNTH_H_ */
