extern "C"
{

#include "avcodec.h"
#include "internal.h"
#include "libavutil/internal.h"
#include "libavutil/opt.h"

} // extern "C"

#include <lpvc/lpvc.h>
#include <stdexcept>


struct LpvcEncoderContext
{
    AVClass* lpvcClass;
    lpvc::Encoder* encoder;
    std::size_t frameCountSinceLastKeyFrame;
    int usePalette;
};

static_assert(std::is_trivial_v<LpvcEncoderContext>);


static lpvc::EncoderSettings makeEncoderSettings(AVCodecContext* avctx)
{
    auto ctx = static_cast<LpvcEncoderContext*>(avctx->priv_data);
    lpvc::EncoderSettings settings;

    settings.usePalette = (ctx->usePalette ? true : false);
    settings.zstdWorkerCount = std::max(1, avctx->thread_count);

    if(avctx->compression_level != FF_COMPRESSION_DEFAULT)
    {
        if(avctx->compression_level < 1 || avctx->compression_level > ZSTD_maxCLevel())
            throw std::invalid_argument("Compression level must be in range 1-" + std::to_string(ZSTD_maxCLevel()) + ".");

        settings.zstdCompressionLevel = avctx->compression_level;
    }

    return settings;
}


static int encodeInit(AVCodecContext* avctx)
{
    auto ctx = static_cast<LpvcEncoderContext*>(avctx->priv_data);

    try
    {
        auto bitmapInfo = lpvc::BitmapInfo {
            static_cast<std::size_t>(avctx->width),
            static_cast<std::size_t>(avctx->height)
        };

        ctx->encoder = new lpvc::Encoder(bitmapInfo, makeEncoderSettings(avctx));
        ctx->frameCountSinceLastKeyFrame = 0;

        return 0;
    }
    catch(std::exception& e)
    {
        av_log(avctx, AV_LOG_ERROR, "%s\n", e.what());
    }
    catch(...)
    {
    }

    return AVERROR_UNKNOWN;
}


static int encodeEnd(AVCodecContext* avctx)
{
    auto ctx = static_cast<LpvcEncoderContext*>(avctx->priv_data);

    delete ctx->encoder;

    return 0;
}


static int encodeFrame(AVCodecContext* avctx, AVPacket* pkt, const AVFrame* pict, int* got_packet)
{
    try
    {
        auto ctx = static_cast<LpvcEncoderContext*>(avctx->priv_data);
        bool keyFrame = false;

        if(avctx->gop_size > 0 &&
           ctx->frameCountSinceLastKeyFrame == static_cast<std::size_t>(avctx->gop_size - 1))
        {
            ctx->frameCountSinceLastKeyFrame = 0;
            keyFrame = true;
        }

        if(auto ret = ff_alloc_packet2(avctx, pkt, ctx->encoder->safeOutputBufferSize(), 0); ret < 0)
            return ret;

        auto result = ctx->encoder->encode(
            reinterpret_cast<const lpvc::Color*>(pict->data[0]),
            reinterpret_cast<std::byte*>(pkt->data),
            keyFrame
        );

        pkt->size = result.bytesWritten;

        if(result.keyFrame)
            pkt->flags |= AV_PKT_FLAG_KEY;
        else
            pkt->flags &= ~AV_PKT_FLAG_KEY;

        ++ctx->frameCountSinceLastKeyFrame;

        *got_packet = 1;

        return 0;
    }
    catch(std::exception& e)
    {
        av_log(avctx, AV_LOG_ERROR, "%s\n", e.what());
    }
    catch(...)
    {
    }

    return AVERROR_UNKNOWN;
}


const AVOption lpvcOptions[] = {
    { "use-palette", "Use palette compression", offsetof(LpvcEncoderContext, usePalette), AV_OPT_TYPE_BOOL, { .i64 = 1 }, 0, 1, AV_OPT_FLAG_VIDEO_PARAM | AV_OPT_FLAG_ENCODING_PARAM },
    { NULL }
};


const AVClass lpvcClass = {
    .class_name = "lpvc encoder",
    .item_name  = av_default_item_name,
    .option     = lpvcOptions,
    .version    = static_cast<int>(lpvc::version())
};


extern "C"
{

AVCodec ff_lpvc_encoder = {
    .name           = "lpvc",
    .long_name      = NULL_IF_CONFIG_SMALL("Longplay Video Codec"),
    .type           = AVMEDIA_TYPE_VIDEO,
    .id             = AV_CODEC_ID_LPVC,
    .capabilities   = AV_CODEC_CAP_SLICE_THREADS,
    .pix_fmts       = (const enum AVPixelFormat[]) { AV_PIX_FMT_RGB24, AV_PIX_FMT_NONE },
    .priv_class     = &lpvcClass,
    .priv_data_size = sizeof(LpvcEncoderContext),
    .init           = encodeInit,
    .encode2        = encodeFrame,
    .close          = encodeEnd,
};

} // extern "C"
