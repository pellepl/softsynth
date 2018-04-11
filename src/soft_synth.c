/*
 * soft_synth.c
 *
 *  Created on: Feb 11, 2012
 *      Author: petera
 */

#include "soft_synth.h"

#define BASE_FREQ (1<<FREQ_LOG_2)

#define SEED 0x12345678
#define FREQ_SEC (BASE_FREQ << ACC_F)
#define LINEAR_INTERPOLATION(factor, x, y, acc) (x) + ((factor) * ((y) - (x)) >> acc)
#define MAX_OUT (1<<ACC_O)

#define WFS__LOOP_NEVEREND		-2
#define WFS__LOOP_FIRST			-1

static const int TONE_TABLE[] = {
	0x10, 0x11, 0x12, 0x13, 0x15, 0x16, 0x17, 0x18, 0x1a, 0x1b, 0x1d, 0x1f,
	0x21, 0x23, 0x25, 0x27, 0x29, 0x2c, 0x2e, 0x31, 0x34, 0x37, 0x3a, 0x3e,
	0x41, 0x45, 0x49, 0x4e, 0x52, 0x57, 0x5c, 0x62, 0x68, 0x6e, 0x75, 0x7b,
	0x83, 0x8b, 0x93, 0x9c, 0xa5, 0xaf, 0xb9, 0xc4, 0xd0, 0xdc, 0xe9, 0xf7,
	0x106, 0x115, 0x126, 0x137, 0x14a, 0x15d, 0x172, 0x188, 0x19f, 0x1b8, 0x1d2, 0x1ee,
	0x20b, 0x22a, 0x24b, 0x26e, 0x293, 0x2ba, 0x2e4, 0x310, 0x33f, 0x370, 0x3a4, 0x3dc,
	0x416, 0x455, 0x497, 0x4dc, 0x526, 0x575, 0x5c8, 0x620, 0x67d, 0x6e0, 0x748, 0x7b7,
	0x82d, 0x8a9, 0x92d, 0x9b9, 0xa4d, 0xaea, 0xb90, 0xc40, 0xcfa, 0xdc0, 0xe91, 0xf6f,
	0x105a, 0x1152, 0x125a, 0x1372, 0x149a, 0x15d3, 0x171f, 0x187f, 0x19f4, 0x1b7f, 0x1d22, 0x1edd,
	0x20b3, 0x22a5, 0x24b4, 0x26e3, 0x2933, 0x2ba6, 0x2e3f, 0x30ff, 0x33e8, 0x36ff, 0x3a44, 0x3dbb,
	0x4166, 0x454a, 0x4969, 0x4dc6, 0x5266, 0x574c, 0x5c7d, 0x61fd, 0x67d1, 0x6dfd, 0x7488, 0x7b75,
	0x82cd, 0x8a94, 0x92d1, 0x9b8c, 0xa4cc, 0xae99, 0xb8fb, 0xc3fb, 0xcfa2, 0xdbfa, 0xe90f, 0xf6eb,
};

static void synth_setWaveformSeq(struct SYNTH_Device_t* pDev,
		int channel, enum synth_waveform_property_e wf_prop, short* sequence);


static synth_prop_t* synth_waveformChannelProperty(struct SYNTH_Channel_t* pChannel,
		enum synth_waveform_property_e wf_prop) {
	switch (wf_prop) {
	case TONE:
		return &pChannel->tone;
	case FREQUENCY:
		return &pChannel->f;
	case VOLUME:
		return &pChannel->vol;
	case MODULATION_FACTOR:
		return &pChannel->mod_fact;
	case MODULATION_DELTA:
		return &pChannel->mod_f_delta;
	case NOISE_FACTOR:
		return &pChannel->noise_fact;
	case NOISE_FREQUENCY:
		return &pChannel->noise_freq;
	case HIPASS_FACTOR:
		return &pChannel->hipass_fact;
	case LOPASS_FACTOR:
		return &pChannel->lopass_fact;
	case WAVE_SEL:
		return &pChannel->lopass_fact;
	default:
		return 0;
	}
}

static void synth_sequenceChannel(struct SYNTH_Device_t* pDev, int channel) {
	struct SYNTH_Channel_t* pChannel = &pDev->channels[channel];
	struct SYNTH_Sequencer_Channel_t* pSeq = &pChannel->sequencer;
	if (pSeq->timer > 1) {
		pSeq->timer--;
		return;
	}
	pChannel->waveform_sequencer.mask = 0;
	short track_selection = pSeq->map[pSeq->map_index];
	short* pTrack = pSeq->tracks[track_selection];
	if (!pTrack) {
		return;
	}
	char keepGoing = 1;
	do {
		short raw = pTrack[pSeq->track_index++];
		char cmd = (raw >> 12) & 0xf;
		short data = (raw & 0xfff);

		switch (cmd) {
		case SEQ_CMD_SET_TONE: {
			int tone = data & 0xff;
			pSeq->envelope = (data >> 8) & 0xf;
			pChannel->tone = tone;
			pChannel->f = TONE_TABLE[tone];
			synth_setWaveformSeq(pDev, channel, VOLUME,
					pDev->sequencer.envelope_waveform_sequences[pSeq->envelope]);
			synth_setWaveformSeq(pDev, channel, FREQUENCY,
					pDev->sequencer.tremolo_waveform_sequences[pSeq->tremolo]);
			synth_setWaveformSeq(pDev, channel, TONE,
					pDev->sequencer.chord_waveform_sequences[pSeq->chord]);
			pChannel->wavesel = pSeq->waveform;
			if (pSeq->flags & SEQ_FLAG_RESET_FREQ_AT_TONE) {
				pChannel->f_count = 0;
			}
			if (pSeq->flags & SEQ_FLAG_RESET_MOD_AT_TONE) {
				pChannel->f_mod_count = 0;
			}
			break;
		}
		case SEQ_CMD_SET_SOUND: {
			pSeq->waveform = data & 0x3;
			pSeq->chord = (data >> 2) & 0xf;
			pSeq->tremolo = (data >> 6) & 0xf;
			break;
		}
		case SEQ_CMD_SET_PROPERTY: {
			synth_prop_t* wf_prop =
					synth_waveformChannelProperty(pChannel, data & 0xf);
			*wf_prop = (data >> 4) & 0xfff;
			break;
		}
		case SEQ_CMD_WAVEFORM: {
			enum synth_waveform_property_e wf_prop = data & 0xf;
			int commonWaveformSequence = (data >> 4) & 0x1f;
			synth_setWaveformSeq(pDev, channel, wf_prop,
					pDev->sequencer.common_waveform_sequences[commonWaveformSequence]);
			break;
		}
		case SEQ_CMD_SET_FLAGS: {
			pSeq->flags = data & 0xff;
			break;
		}
		case SEQ_CMD_ENABLE_FLAGS: {
			pSeq->flags |= data & 0xff;
			break;
		}
		case SEQ_CMD_DISABLE_FLAGS: {
			pSeq->flags &= ~(data & 0xff);
			break;
		}
		case SEQ_CMD_END: {
			pSeq->timer = data & 0xff;
			keepGoing = 0;
			break;
		}
		case SEQ_CMD_NEXT: {
			pSeq->timer = 0;
			pSeq->track_index = 0;
			pSeq->map_index++;
			if (pSeq->map_index >= pSeq->map_length) {
				pSeq->map_index = 0;
			}
			track_selection = pSeq->map[pSeq->map_index];
			pTrack = pSeq->tracks[track_selection];

			break;
		}
		}
	} while (keepGoing);

}

void synth_sequence(struct SYNTH_Device_t* pDev) {
	int i;
	for (i = 0; i < CHANNELS; i++) {
		synth_sequenceChannel(pDev, i);
	}
}

static void synth_setWaveformSeq(struct SYNTH_Device_t* pDev,
		int channel, enum synth_waveform_property_e wf_prop, short* sequence) {
	struct SYNTH_Waveform_Property_Sequence_t* pSeq =
			&(pDev->channels[channel].waveform_sequencer.prop_seq[wf_prop]);
	pSeq->index = 0;
	pSeq->loop = WFS__LOOP_FIRST;
	pSeq->timer = 0;
	pSeq->sequence = sequence;
	pDev->channels[channel].waveform_sequencer.mask |= (1<<wf_prop);
}

static void synth_waveformChannel(struct SYNTH_Channel_t* pChannel) {
	enum synth_waveform_property_e wf_prop;
	int mask = pChannel->waveform_sequencer.mask;
	// go through each waveform property to see if it has an active sequence
	for (wf_prop = TONE; wf_prop < WAVEFORM_PROPERTIES; wf_prop++) {
		if (mask & 1) {
			// mask bit set for this waveform property, execute sequence
			struct SYNTH_Waveform_Property_Sequence_t* pSequence =
					&(pChannel->waveform_sequencer.prop_seq[wf_prop]);
			if (pSequence->timer > 0) {
				// pause
				pSequence->timer--;
			} else {
				// perform sequence until end command found
				synth_prop_t* pPropertyValue = synth_waveformChannelProperty(pChannel, wf_prop);
				short wfseq_cmd;
				do {
					wfseq_cmd = pSequence->sequence[pSequence->index++];
					pSequence->timer = 0;
					short wfseq_data = (wfseq_cmd & 0xfff0)>>4; // get data
					// dont check end and stop bits now plz
					switch (wfseq_cmd & 3) {
					case WFS_COMMAND_PAUSE:
						pSequence->timer = wfseq_data;
						break;
					case WFS_COMMAND_LOOP: {
						int jump = 1 + (wfseq_data & 0xf);
						if (pSequence->loop == WFS__LOOP_NEVEREND) {
							// neverending loop
							pSequence->index -= jump;
						} else if (pSequence->loop == WFS__LOOP_FIRST) {
							// first loop, initialize
							int times = ((wfseq_data>>4) & 0xff);
							pSequence->loop = times == 0 ? WFS__LOOP_NEVEREND : times-1;
							pSequence->index -= jump;
						} else if (pSequence->loop != 0) {
							// looping around
							pSequence->loop--;
							pSequence->index -= jump;
						} else {
							// loop finished, reset
							pSequence->loop = WFS__LOOP_FIRST;
						}
						break;
					}
					case WFS_COMMAND_SET:
						*pPropertyValue = wfseq_data;
						break;
					case WFS_COMMAND_RELATIVE:
						if (wfseq_data & 0x800) {
							wfseq_data |= 0xfffff000; // sign extend
						}
						*pPropertyValue += wfseq_data;
						if (*pPropertyValue < 0) {
							*pPropertyValue = 0;
						}
						break;
					}
					if (wf_prop == TONE) {
						// special handling for tone setting
						pChannel->f = TONE_TABLE[*pPropertyValue];
					}
					if (wfseq_cmd & WFS_COMMAND_SEQ_STOP) {
						// stop bit set, mask away this wf sequence
						pChannel->waveform_sequencer.mask &= ~(1<<wf_prop);
					}
					if (wfseq_cmd & WFS_COMMAND_SEQ_END) {
						// end bit set, break out of while loop
						break;
					}
				} while (1);
			}
		}
		mask >>= 1;
		if (mask == 0) {
			// no more properties to proces, break out of for loop
			break;
		}
	} // for waveform property
}


void synth_waveform(struct SYNTH_Device_t *pDev) {
	int i;
	for (i = 0; i < CHANNELS; i++) {
		synth_waveformChannel(&pDev->channels[i]);
	}
}

static int synth_synthetisize(struct SYNTH_Channel_t* pChannel) {
	int o_sq, o_sawup, o_sawdown, o_tri;
	int tmp_sawup;
	int out, noise_out;

	// base counter
	pChannel->f_count += (pChannel->f<<ACC_F);
	if (pChannel->f_count > FREQ_SEC) {
		pChannel->f_count -= FREQ_SEC;
	}
	// modulator
	pChannel->f_mod_count += ((pChannel->f<<ACC_F) + pChannel->mod_f_delta);
	if (pChannel->f_mod_count > FREQ_SEC) {
		pChannel->f_mod_count -= FREQ_SEC;
	}

	// output waveform generation
	// square
	o_sq = pChannel->f_count < FREQ_SEC >> 1 ? 0 : MAX_OUT;
	// saw intermediate (o * 2)
	tmp_sawup = pChannel->f_count>>(FREQ_LOG_2-1-(ACC_F>ACC_O?ACC_F-ACC_O:ACC_O-ACC_F));
	// saw uphill
	o_sawup = tmp_sawup >> 1;
	// saw downhill
	o_sawdown = MAX_OUT - o_sawup;
	// triangle
	o_tri = pChannel->f_count < FREQ_SEC >> 1 ? tmp_sawup : (MAX_OUT - 1
			- tmp_sawup);

	// output mux
	switch (pChannel->wavesel) {
	case 0:
		out = o_sq;
		break;
	case 1:
		out = o_sawup;
		break;
	case 2:
		out = o_sawdown;
		break;
	case 3:
		out = o_tri & (MAX_OUT-1);
		break;
	default:
		out = 0;
	}

	// modulation
	if (pChannel->mod_fact) {
		int mod_out = pChannel->f_mod_count < FREQ_SEC >> 1 ? 0
				: out;
		out = LINEAR_INTERPOLATION(pChannel->mod_fact, out, mod_out, ACC_O);
	}

	// noise gen
	if (pChannel->noise_fact) {
		noise_out = pChannel->noise_out;
		if (pChannel->noise_count++ >= pChannel->noise_freq) {
			pChannel->noise_count = 0;
			noise_out = (noise_out >> 1) ^ (-(noise_out & 1) & 0xd0000001u);
			pChannel->noise_out = noise_out;
		}
		out = LINEAR_INTERPOLATION(pChannel->noise_fact, out, noise_out & (MAX_OUT-1), ACC_O);
	}

	// filters
	int zout = pChannel->zout;
	if (pChannel->lopass_fact) {
		out = LINEAR_INTERPOLATION(pChannel->lopass_fact, out, zout, ACC_O);
	}
	if (pChannel->hipass_fact) {
		out = LINEAR_INTERPOLATION(pChannel->hipass_fact, out, out-zout < 0 ? zout-out : out-zout, ACC_O);
	}

	// volume
	out = LINEAR_INTERPOLATION((pChannel->vol * pChannel->gain)>>ACC_O, 0, out, ACC_O);

	pChannel->zout = out;
	return out & (MAX_OUT-1);
}

static int synth_mix(struct SYNTH_Device_t *pDev) {
	int output = 0;
	int i;
	for (i = 0; i < CHANNELS; i++) {
		output += synth_synthetisize(&pDev->channels[i]);
	}
	output = (output * (pDev->gain + (MAX_OUT / CHANNELS))) >> ACC_O;
	if (output < 0) output = 0;
	if (output >= MAX_OUT) output = MAX_OUT-1;
	return output;
}

void synth_output(struct SYNTH_Device_t* pDev) {
	int output = synth_mix(pDev);
	pDev->dac_f(output);
}

void synth_tick(struct SYNTH_Device_t* pDev) {
	if (pDev->waveformTimer >= BASE_FREQ) {
		if (pDev->sequenceTimer++ >= pDev->sequenceDiv) {
			pDev->sequenceTimer = 0;
			synth_sequence(pDev);
		}


		pDev->waveformTimer -= BASE_FREQ;
		synth_waveform(pDev);
	}
	pDev->waveformTimer += pDev->waveformHz;


	synth_output(pDev);
}

void synth_sequencer_init(struct SYNTH_Device_t* pDev,
	short** envelope_waveform_sequences,
	short** tremolo_waveform_sequences,
	short** chord_waveform_sequences,
	short** common_waveform_sequences) {
	pDev->sequencer.envelope_waveform_sequences = envelope_waveform_sequences;
	pDev->sequencer.tremolo_waveform_sequences = tremolo_waveform_sequences;
	pDev->sequencer.chord_waveform_sequences = chord_waveform_sequences;
	pDev->sequencer.common_waveform_sequences = common_waveform_sequences;
}

void synth_channel_sequencer_init(struct SYNTH_Device_t *pDev,
		int channel, short trackmapLen, short* trackmap, short** tracks) {
	struct SYNTH_Sequencer_Channel_t *pSeq = &pDev->channels[channel].sequencer;
	pSeq->envelope = 0;
	pSeq->tremolo = 0;
	pSeq->chord = 0;
	pSeq->waveform = 0;
	pSeq->flags = 0;
	pSeq->timer = 0;
	pSeq->track_index = 0;
	pSeq->map_index = 0;
	pSeq->map_length = trackmapLen;
	pSeq->map = trackmap;
	pSeq->tracks = tracks;
}

void synth_init(struct SYNTH_Device_t* pDev, synth_set_dac_f f, int sequenceDiv, int waveformHz) {
	int chan,prop;
	for (chan = 0; chan < CHANNELS; chan++) {
		pDev->channels[chan].tone = 0;
		pDev->channels[chan].f = 0;
		pDev->channels[chan].wavesel = 0;
		pDev->channels[chan].vol = MAX_OUT-1;
		pDev->channels[chan].noise_fact = 0;
		pDev->channels[chan].noise_freq = 0;
		pDev->channels[chan].mod_fact = 0;
		pDev->channels[chan].hipass_fact = 0;
		pDev->channels[chan].lopass_fact = 0;

		pDev->channels[chan].zout = 0;
		pDev->channels[chan].noise_out = SEED + chan;
		pDev->channels[chan].f_count = 0;
		pDev->channels[chan].f_mod_count = 0;

		pDev->channels[chan].gain = MAX_OUT;
		pDev->channels[chan].waveform_sequencer.mask = 0;
		for (prop = 0; prop < WAVEFORM_PROPERTIES; prop++) {
			pDev->channels[chan].waveform_sequencer.prop_seq[prop].index = 0;
			pDev->channels[chan].waveform_sequencer.prop_seq[prop].timer = 0;
			pDev->channels[chan].waveform_sequencer.prop_seq[prop].loop = WFS__LOOP_FIRST;
		}
	}
	pDev->dac_f = f;
	pDev->waveformTimer = BASE_FREQ;
	pDev->waveformHz = waveformHz;
	pDev->sequenceTimer = sequenceDiv;
	pDev->sequenceDiv = sequenceDiv;
	pDev->gain = 0;
}
