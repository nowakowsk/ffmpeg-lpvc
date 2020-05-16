extern "C"
{

#include "avcodec.h"
#include "internal.h"
#include "libavutil/internal.h"

} // extern "C"

#include "lpvciter.h"
#include <lpvc/lpvc.h>


struct LpvcDecoderContext
{
    lpvc::Decoder* decoder;
};


static av_cold int decodeInit(AVCodecContext* avctx)
{
    try
    {
        auto ctx = static_cast<LpvcDecoderContext*>(avctx->priv_data);
        auto bitmapInfo = lpvc::BitmapInfo {
            static_cast<std::size_t>(avctx->width),
            static_cast<std::size_t>(avctx->height)
        };

        avctx->pix_fmt = AV_PIX_FMT_RGB24;
        ctx->decoder = new lpvc::Decoder(bitmapInfo);

        return 0;
    }
    catch(std::exception& e)
    {
        av_log(avctx, AV_LOG_ERROR, "%s\n", e.what());
    }
    catch(...)
    {
        av_log(avctx, AV_LOG_ERROR, "Unknown error\n");
    }

    return AVERROR_UNKNOWN;
}


static av_cold int decodeEnd(AVCodecContext* avctx)
{
    auto ctx = static_cast<LpvcDecoderContext*>(avctx->priv_data);

    delete ctx->decoder;

    return 0;
}


static int decodeFrame(AVCodecContext* avctx, void* data, int* got_frame, AVPacket* avpkt)
{
    try
    {
        auto ctx = static_cast<LpvcDecoderContext*>(avctx->priv_data);
        auto frame = static_cast<AVFrame*>(data);
        auto inputBuffer = reinterpret_cast<const std::byte*>(avpkt->data);
        auto inputBufferSize = avpkt->size;

        if(auto ret = ff_get_buffer(avctx, frame, 0); ret < 0)
            return ret;

        auto result = ctx->decoder->decode(
            inputBuffer,
            inputBufferSize,
            LpvcIterator(avctx->width, frame->linesize[0], reinterpret_cast<std::byte*>(frame->data[0]))
        );

        frame->key_frame = result.keyFrame;
        frame->pict_type = AV_PICTURE_TYPE_I;
        *got_frame = 1;

        return inputBufferSize;
    }
    catch(std::exception& e)
    {
        av_log(avctx, AV_LOG_ERROR, "%s\n", e.what());
    }
    catch(...)
    {
        av_log(avctx, AV_LOG_ERROR, "Unknown error\n");
    }

    return AVERROR_UNKNOWN;
}


extern "C"
{

AVCodec ff_lpvc_decoder = {
    .name           = "lpvc",
    .long_name      = NULL_IF_CONFIG_SMALL("Longplay Video Codec"),
    .type           = AVMEDIA_TYPE_VIDEO,
    .id             = AV_CODEC_ID_LPVC,
    .capabilities   = AV_CODEC_CAP_DR1,
    .priv_data_size = sizeof(LpvcDecoderContext),
    .init           = decodeInit,
    .decode         = decodeFrame,
    .close          = decodeEnd,
};

} // extern "C"
