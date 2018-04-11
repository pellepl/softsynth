/*
 * soft_synth_wfseq.h
 *
 *  Created on: Feb 11, 2012
 *      Author: petera
 */

#ifndef SOFT_SYNTH_WFSEQ_H_
#define SOFT_SYNTH_WFSEQ_H_

/**
 * Waveform Sequence Command Structure
 * [bits 0..1] 		WFS COMMAND
 * [bit  2]		   	WFS END COMMAND - this part of sequence commands has ended
 * [bit  3]		   	WFS STOP COMMAND - this whole sequence has stopped
 * [bits 4..15]		WFS COMMAND DATA
 *
 * Organised as command clusters separated by WFS_COMMAND_SEQ_END bit.
 * A command cluster is executed instantly. Next cluster will be executed
 * next waveform tick, or when pause has finished as set by WFS_COMMAND_PAUSE.
 * A full sequence of command clusters is ended by WFS_COMMAND_SEQ_STOP bit.
 */

#define WFS_COMMAND_PAUSE		0x0
#define WFS_COMMAND_LOOP		0x1
#define WFS_COMMAND_SET			0x2
#define WFS_COMMAND_RELATIVE	0x3
#define WFS_COMMAND_SEQ_END		0x4	// put on bit 2 meaning we don't need an exclusive end command
#define WFS_COMMAND_SEQ_STOP	0x8 // put on bit 3 meaning we don't need an exclusive stop command

#define WFS_PAUSE(len)			(WFS_COMMAND_PAUSE | ((len)<<4))
#define WFS_LOOP(jmp,times)		(WFS_COMMAND_LOOP | (((jmp) & 0xf) | (((times-1)&0xff)<<4))<<4)
#define WFS_SET(cmd)			(WFS_COMMAND_SET | ((cmd)<<4))
#define WFS_RELATIVE(cmd)		(WFS_COMMAND_RELATIVE | ((cmd)<<4))
#define WFS_NOP					(WFS_COMMAND_RELATIVE | 0)
#define WFS_END(cmd)			(WFS_COMMAND_SEQ_END | (cmd))
#define WFS_STOP(cmd)			(WFS_COMMAND_SEQ_STOP | WFS_END(cmd))

#endif /* SOFT_SYNTH_WFSEQ_H_ */
