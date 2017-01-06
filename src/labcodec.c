// LabCodec

#include <stdio.h>
#include "avcodec.h"
#include "internal.h"
#include "labcodec.h"

// This macro will be used to access pixels in encode_frame() and decode_frame().
// It assumes you have defined `frame` in the surrounding scope as an AVFrame*.
// The argument `plane` is the color plane to access, 0: red, 1: green, 2: blue.
#define PIXEL(plane, x, y) (frame->data[0][(x) * 3 + frame->linesize[0] * (y) + (plane)])

static av_cold int encode_init(AVCodecContext* avctx)
{
	// libavcodec allocates a buffer of size .priv_data_size and sticks it here.
	// We cast this to a LabCodecContext and use this space for our private storage.
	LabCodecContext* const ctx = avctx->priv_data;

	// Your codec initialization code here...
	// You will have to initialize any fields of your LabCodecContext here.
	// As a default example we save our parent AVCodecContext.
	ctx->avctx = avctx;

	return 0;
}

static av_cold int encode_end(AVCodecContext* avctx)
{
	// Your cleanup code here...

	return 0;
}

// This function is called once per frame.
static int encode_frame(AVCodecContext* avctx, AVPacket* avpkt,
                        const AVFrame* frame, int* got_packet)
{
	LabCodecContext* const ctx = avctx->priv_data;
	int ret;

	// Unfortunately, we must pre-allocate an output buffer for our encoded frame,
	// and thus guess as to the maximum possible number of bytes we could need.
	// We return the error code (probably out of memory) if the call fails.
	int maximum_possible_encoded_frame_size = avctx->width * avctx->height * 10;
	if ((ret = ff_alloc_packet2(avctx, avpkt, maximum_possible_encoded_frame_size + AV_INPUT_BUFFER_MIN_SIZE, 0)) < 0)
		return ret;

	// Our output buffer now lives at avpkt->data, and can fit at most avpkt->size bytes.

	// We now initialize our LabCodecContext.PutBitContext.
	// This will allow us to write to avpkt->data bit by bit.
	init_put_bits(&ctx->pb, avpkt->data, avpkt->size * 8);

	// You can now write bits into our output buffer as follows:
	//put_bits(&ctx->pb, num_bits, value);
	// For example, put_bits(&ctx->pb, 5, 13) will write 01101 into the output buffer.
	// The argument num_bits can be at most 31.

	// === Do encoding here ===
	// Hints: avctx->width and avctx->height give the input frame size.
	// You can read pixels with PIXEL(color, x, y) (See comments at top of file.)

	// Flush our bit writer to the buffer.
	flush_put_bits(&ctx->pb);	

	// We round up the number of bits to the nearest byte, and set that as our output size.
	// This adds an untracked number of padding zero bits 0-7, so you cannot rely on
	// distinguishing how many zero bits your input packet ends with!
	avpkt->size = (put_bits_count(&ctx->pb) + 7) / 8;

	// For now mark every single packet as containing an I-frame.
	avpkt->flags |= AV_PKT_FLAG_KEY;

	// We tell libavcodec that we successfully wrote output.
	*got_packet = 1;

	return 0;
}

static av_cold int decode_init(AVCodecContext* avctx)
{
	LabCodecContext* const ctx = avctx->priv_data;
	ctx->avctx = avctx;

	// Tell libavcodec that we would like our input as
	// RGB triples encoded in a single packed array.
	avctx->pix_fmt = AV_PIX_FMT_RGB24;
	return 0;
}

static av_cold int decode_end(AVCodecContext* avctx)
{
	return 0;
}

static int decode_frame(AVCodecContext *avctx,
                        void *data, int *got_frame,
                        AVPacket *avpkt)
{
	LabCodecContext* const ctx = avctx->priv_data;
	int ret;

	// We init a bit reader on our input buffer, and return on failure.
	if ((ret = init_get_bits(&ctx->gb, avpkt->data, avpkt->size * 8)) < 0)
		return ret;

	// We request a buffer to write our output to, and return on failure.
	if ((ret = ff_get_buffer(avctx, frame, 0)) < 0)
		return ret;

	// === Do decoding here ===
	// Hints: avctx->width and avctx->height are still the width and height.
	// You can write pixels like:
	//PIXEL(0, 10, 30) = 255;

	// Tell libavcodec that we successfully decoded a frame.
	*got_frame = 1;
	return 0;
}

// These two structs should be the only exported symbols of this file.

AVCodec ff_labcodec_encoder = {
	.name                  = "labcodec",
	.long_name             = NULL_IF_CONFIG_SMALL("Video Compression Lab Codec"),
	.type                  = AVMEDIA_TYPE_VIDEO,
	.id                    = AV_CODEC_ID_LABCODEC,
	.priv_data_size        = sizeof(LabCodecContext),
	.init                  = encode_init,
	.encode2               = encode_frame,
	.close                 = encode_end,
    .pix_fmts              = (const enum AVPixelFormat[]) {
        AV_PIX_FMT_RGB24,
        AV_PIX_FMT_NONE
    },
};

AVCodec ff_labcodec_decoder = {
    .name           = "labcodec",
    .long_name      = NULL_IF_CONFIG_SMALL("Video Compression Lab Codec"),
    .type           = AVMEDIA_TYPE_VIDEO,
    .id             = AV_CODEC_ID_LABCODEC,
    .priv_data_size = sizeof(LabCodecContext),
    .init           = decode_init,
    .decode         = decode_frame,
	.close          = decode_end,
    .capabilities   = AV_CODEC_CAP_DR1,
};

