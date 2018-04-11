/*
 * soft_synth_seq.h
 *
 *  Created on: Feb 12, 2012
 *      Author: petera
 */

#ifndef SOFT_SYNTH_SEQ_H_
#define SOFT_SYNTH_SEQ_H_

/**
 * Sequencer Command Structure
 * [bits 0..11]		Sequencer data
 * [bits 12..15]	Sequencer command
 */

/**
 * SET TONE
 * bits[0..7]		tone selection
 * bits[8..11]		envelope waveform sequence (waveform property volume)
 */
#define SEQ_CMD_SET_TONE		0x0
/**
 * SET SOUND
 * bits[0..1]		waveform selection (0 square, 1 sawup, 2 sawdown, 3 triangle)
 * bits[2..5]		chord waveform sequence (waveform property tone)
 * bits[6..9]		tremolo waveform sequence (waveform property frequency)
 */
#define SEQ_CMD_SET_SOUND		0x1
/**
 * SET PROPERTY
 * bits[0..3]		waveform property as defined by enum synth_waveform_property_e
 * bits[4..11]		waveform value
 */
#define SEQ_CMD_SET_PROPERTY	0x2
/**
 * WAVEFORM
 * bits[0..3]		waveform property as defined by enum synth_waveform_property_e
 * bits[4..8]		common waveform sequence
 */
#define SEQ_CMD_WAVEFORM		0x3
/**
 * SET FLAGS
 * bits[0..7]		define sequencer channel flags
 */
#define SEQ_CMD_SET_FLAGS		0x4
/**
 * ENABLE FLAGS
 * bits[0..7]		enable sequencer channel flags
 */
#define SEQ_CMD_ENABLE_FLAGS	0x5
/**
 * DISABLE FLAGS
 * bits[0..7]		disable sequencer channel flags
 */
#define SEQ_CMD_DISABLE_FLAGS	0x6
/**
 * SEQUENCE PART END
 * bits[0..7]		pause in sequencer ticks until next sequence command cluster
 */
#define SEQ_CMD_END				0xe
/**
 * SEQUENCE NEXT
 * 					ends this sequence and goes to next
 */
#define SEQ_CMD_NEXT			0xf

#define SEQ_FLAG_RESET_FREQ_AT_TONE		(1<<0)
#define SEQ_FLAG_RESET_MOD_AT_TONE		(1<<1)

#define SEQ_TONE(tone,env)			 ((SEQ_CMD_SET_TONE<<12) | ((tone)&0xff) |(((env)&0xf)<<8))
#define SEQ_SOUND(wave,chord,trem)	 ((SEQ_CMD_SET_SOUND<<12) | ((wave)&0x3) |(((chord)&0xf)<<2) | (((trem)&0xf)<<6))
#define SEQ_PROPERTY(prop, val)		 ((SEQ_CMD_SET_PROPERTY<<12) | ((prop)&0xf) | (((val)&0xff)<<4))
#define SEQ_WAVEFORM(prop, val)		 ((SEQ_CMD_WAVEFORM<<12) | ((prop)&0x1f) | (((val)&0xff)<<4))
#define SEQ_SET_FLAGS(flags)		 ((SEQ_CMD_SET_FLAGS<<12) | ((flags)&0xff))
#define SEQ_ENA_FLAGS(flags)		 ((SEQ_CMD_ENABLE_FLAGS<<12) | ((flags)&0xff))
#define SEQ_DIS_FLAGS(flags)		 ((SEQ_CMD_DISABLE_FLAGS<<12) | ((flags)&0xff))
#define SEQ_END(pause)				 ((SEQ_CMD_END<<12) | ((pause)&0xff))
#define SEQ_NEXT					 (SEQ_CMD_NEXT<<12)

#endif /* SOFT_SYNTH_SEQ_H_ */
