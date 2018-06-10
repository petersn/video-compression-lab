// LabCodec

#ifndef AVCODEC_LABCODEC_H
#define AVCODEC_LABCODEC_H

#include "get_bits.h"
#include "put_bits.h"

// This is the persistent state of your encoder/decoder.
typedef struct {
	// This context is used during decoding to read input packets bit by bit.
	GetBitContext gb;

	// This is used during encoding to write output packets bit by bit.
	PutBitContext pb;

	// If you need to store data structures between frames, add them here...

} LabCodecContext;

#endif /* AVCODEC_LABCODEC_H */

