/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2015 - ROLI Ltd.

   Permission is granted to use this software under the terms of either:
   a) the GPL v2 (or any later version)
   b) the Affero GPL v3

   Details of these licenses can be found at: www.gnu.org/licenses

   JUCE is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
   A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

   ------------------------------------------------------------------------------

   To release a closed-source product which uses JUCE, commercial licenses are
   available: visit www.juce.com for more information.

  ==============================================================================
*/

#if JUCE_USE_FFMPEG

#include <unistd.h>
#include <sys/types.h>

namespace FFmpegNamespace {

extern "C" {
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libswresample/swresample.h>
}

class customAVCOntext {
    InputStream *is;
    unsigned char *buffer;
    AVIOContext *ctx;

    customAVCOntext(customAVCOntext const &);
    customAVCOntext &operator=(customAVCOntext const &);

public:

    customAVCOntext(InputStream *_is)
            : is(_is),
              buffer(static_cast<unsigned char*>(av_malloc(4 * 1024))) {
        ctx = avio_alloc_context(buffer,
                                 4 * 1024, 0, this,
                                 &customAVCOntext::read, NULL,
                                 &customAVCOntext::seek);
    }

    ~customAVCOntext() {
        // av_free(ctx); // context is freed in FFmpegReader
        av_free(buffer);
    }

    static int read(void *opaque, unsigned char *buf, int buf_size) {
        customAVCOntext* h = static_cast<customAVCOntext*>(opaque);
        return h->is->read(buf, buf_size);
    }

    static int64_t seek(void *opaque, int64_t offset, int whence) {
        customAVCOntext* h = static_cast<customAVCOntext*>(opaque);

        if (whence == AVSEEK_SIZE)
            return h->is->getTotalLength();
        else if (whence == SEEK_CUR)
            return h->is->setPosition(h->is->getPosition() + offset);
        else if (whence == SEEK_END)
            return h->is->setPosition(h->is->getTotalLength() + offset);

        return h->is->setPosition(offset);
    }

    AVIOContext *getAVIO() {
        return ctx;
    }
};
}

//==============================================================================
static const char* const FFmpegFormatName = "FFmpeg file";

//==============================================================================
class FFmpegReader : public AudioFormatReader
{
public:
    FFmpegReader (InputStream* const inp)
        : AudioFormatReader (inp, FFmpegFormatName),
          reservoirStart (0),
          samplesInReservoir (0)//,
          //sampleBuffer (nullptr)
    {
        using namespace FFmpegNamespace;

        // register all ffmpeg formats (could be called many times)
        av_register_all();

        ffmpegContext = new customAVCOntext(inp);

        format = avformat_alloc_context();
        format->pb = ffmpegContext->getAVIO();

        // get format from file
        if (avformat_open_input(&format, "", NULL, NULL) == 0) {
            std::cout << "open input ok" << std::endl;
            if (avformat_find_stream_info(format, NULL) >= 0) {
                std::cout << "find stream ok" << std::endl;

                // find audio stream index
                audioIndex = -1;
                for (unsigned int i = 0; i < format->nb_streams; ++i) {
                    if (format->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
                        audioIndex = i;
                        // TODO: multiple streams?
                        std::cout << "found audio stream" << std::endl;
                        break;
                    }
                }

                if (audioIndex != -1) {
                    stream = format->streams[audioIndex];
                    codec = stream->codec;

                    // find suitable codec
                    if (avcodec_open2(codec, avcodec_find_decoder(codec->codec_id), NULL) >= 0) {
                        std::cout << "found suitable codec for stream" << std::endl;
                        swr = swr_alloc();
                        av_opt_set_int(swr, "in_channel_count", codec->channels, 0);
                        av_opt_set_int(swr, "out_channel_count", 2, 0);
                        av_opt_set_int(swr, "in_channel_layout", codec->channel_layout, 0);
                        av_opt_set_int(swr, "out_channel_layout", AV_CH_LAYOUT_STEREO, 0);
                        av_opt_set_int(swr, "in_sample_rate", codec->sample_rate, 0);
                        av_opt_set_int(swr, "out_sample_rate", codec->sample_rate, 0);
                        av_opt_set_sample_fmt(swr, "in_sample_fmt", codec->sample_fmt, 0);
                        av_opt_set_sample_fmt(swr, "out_sample_fmt", AV_SAMPLE_FMT_FLTP, 0);
                        swr_init(swr);

                        if (swr_is_initialized(swr)) {
                            std::cout << "swr is initialized" << std::endl;

                            av_init_packet(&packet);
                            frame = av_frame_alloc();

                            if (frame) {
                                lengthInSamples = (uint32)format->duration;// * codec->sample_rate / AV_TIME_BASE;
                                numChannels = (unsigned int)codec->channels;
                                bitsPerSample = 32;
                                sampleRate = codec->sample_rate;
                                usesFloatingPointData = true;

                                uint32 smp = (uint32)(av_q2d(format->streams[audioIndex]->time_base) *
                                                      format->streams[audioIndex]->duration *
                                                      codec->sample_rate);

                                uint32 smp2 = (uint32)(av_q2d(format->streams[audioIndex]->time_base) *
                                                       format->duration *
                                                       codec->sample_rate);

                                lengthInSamples = smp;

                                std::cout << "frame ok" << std::endl
                                          << "samples: " << lengthInSamples << " or " << smp << " or " << smp2 << " or " << stream->nb_frames
                                          << " channels: " << numChannels
                                          << " format duration: " << format->duration
                                          << " time base: " << AV_TIME_BASE
                                          << " sample rate: " << sampleRate << std::endl;

                                reservoir.setSize ((int) numChannels, (int) jmin (lengthInSamples, (int64) 4096));
                            }
                        }
                    }
                }
            }
        }

        // using namespace WavPackNamespace;
        // sampleRate = 0;
        // usesFloatingPointData = true;

        // wvReader.read_bytes = &wv_read_bytes;
        // wvReader.get_pos = &wv_get_pos;
        // wvReader.set_pos_abs = &wv_set_pos_abs;
        // wvReader.set_pos_rel = &wv_set_pos_rel;
        // wvReader.get_length = &wv_get_length;
        // wvReader.can_seek = &wv_can_seek;
        // wvReader.push_back_byte = &wv_push_back_byte;

        // wvContext = WavpackOpenFileInputEx (&wvReader, input, nullptr, wvErrorBuffer, OPEN_NORMALIZE, 0);

        // if (wvContext != nullptr)
        // {
        //     lengthInSamples = (uint32) WavpackGetNumSamples(wvContext);
        //     numChannels = (unsigned int) WavpackGetNumChannels(wvContext);
        //     bitsPerSample = WavpackGetBitsPerSample(wvContext);
        //     sampleRate = WavpackGetSampleRate(wvContext);

        //     reservoir.setSize ((int) numChannels, (int) jmin (lengthInSamples, (int64) 4096));
        // }
    }

    ~FFmpegReader()
    {
        std::cout << "kill reader" << std::endl;
        using namespace FFmpegNamespace;
        av_frame_free(&frame);
        swr_free(&swr);
        avcodec_close(codec);
        avformat_free_context(format);
        delete ffmpegContext;
        // using namespace WavPackNamespace;
        // WavpackCloseFile (wvContext);
    }

    int bufferPosition = 0;
    int samplesInBuffer = 0;

    //==============================================================================
    bool readSamples (int** destSamples, int numDestChannels, int startOffsetInDestBuffer,
                      int64 startSampleInFile, int numSamples) override
    {
        using namespace FFmpegNamespace;

        // destSamples - here goes samples, 2d array destSamples[0] - left, destSamples[1] - right etc.
        // numDestChannels - how many channels (array first index there are)
        // startOffsetInDestBuffer - where to write in output buffer
        // startSampleInFile - where to read in file
        // numSamples - how many samples to read

        while (numSamples > 0) {
            // update state
            int samplesToCopy = jmin(numSamples, samplesInBuffer - bufferPosition);
            if (samplesToCopy > 0) {
                numSamples -= samplesToCopy;
                bufferPosition += samplesToCopy;

                std::cout << "copy " << samplesToCopy << " samples from buffer" << std::endl;

                // always stereo
                float *pleft = buffer[0] + bufferPosition;
                float *pright = buffer[1] + bufferPosition + 1;
                float *outleft = (float*)(destSamples[0] + startOffsetInDestBuffer);
                float *outright = (float*)(destSamples[1] + startOffsetInDestBuffer);

                // copy from buffer
                while (samplesToCopy > 0) {
                    *outleft = *pleft;
                    *outright = *pright;
                    outleft++;
                    outright++;
                    pleft++;
                    pright++;
                    samplesToCopy--;
                }
            }

            // buffer is empty now - decode some more
            if (numSamples > 0) {
                if (av_read_frame(format, &packet) == 0) {
                    std::cout << "~ read frame" << std::endl;

                    int gotFrame = 0;
                    if (avcodec_decode_audio4(codec, frame, &gotFrame, &packet) == 0) {
                        std::cout << "~ decode audio" << std::endl;

                        if (gotFrame != 0) {
                            std::cout << "~ gotFrame " << frame->nb_samples << std::endl;

                            // allocate buffer for samples
                            int dstLinesize;
                            av_samples_alloc((uint8_t**)&buffer, &dstLinesize, 2, frame->nb_samples, AV_SAMPLE_FMT_FLTP, 0);

                            int frames = swr_convert(swr,
                                                     (uint8_t**)buffer,
                                                     frame->nb_samples,
                                                     (const uint8_t**)frame->data,
                                                     frame->nb_samples);
                            frames = frame->nb_samples;
                            bufferPosition = 0;
                            samplesInBuffer = frames;

                            std::cout << "read " << frames << " frames" << std::endl;
                        }
                    }
                }
            }
            // return false?
        }

        return true;

        // old method
        while (numSamples > 0)
        {
            const int numAvailable = (int) (reservoirStart + samplesInReservoir - startSampleInFile);

            if (startSampleInFile >= reservoirStart && numAvailable > 0)
            {
                // got a few samples overlapping, so use them before seeking..
                const int numToUse = jmin (numSamples, numAvailable);

                for (int i = jmin (numDestChannels, reservoir.getNumChannels()); --i >= 0;)
                    if (destSamples[i] != nullptr)
                        memcpy (destSamples[i] + startOffsetInDestBuffer,
                                reservoir.getReadPointer (i, (int) (startSampleInFile - reservoirStart)),
                                sizeof (float) * (size_t) numToUse);

                startSampleInFile += numToUse;
                numSamples -= numToUse;
                startOffsetInDestBuffer += numToUse;

                if (numSamples == 0)
                    break;
            }

            if (startSampleInFile < reservoirStart
                || startSampleInFile + numSamples > reservoirStart + samplesInReservoir)
            {
                // buffer miss, so refill the reservoir
                reservoirStart = jmax (0, (int) startSampleInFile);
                samplesInReservoir = reservoir.getNumSamples();


                /*
                int64_t tm = av_rescale_q((int64_t) (startSampleInFile / sampleRate) * AV_TIME_BASE,
                                        AV_TIME_BASE_Q,
                                        stream->time_base);

                if (av_seek_frame(format, audioIndex, tm, AVSEEK_FLAG_BACKWARD) != 0) {
                    std::cout << "~ seek frame" << std::endl;
                }
                else {
                    std::cout << "seek ok " << samplesInReservoir << std::endl;
                }
                */


                // if (reservoirStart != (int) WavpackGetSampleIndex (wvContext))
                //     WavpackSeekSample (wvContext, reservoirStart);

                int offset = 0;
                int numToRead = frame->nb_samples;

                while (numToRead > 0)
                {
                    // // initialize buffer
                    // if (sampleBuffer == nullptr)
                    // {
                    //     sampleBufferSize = numToRead * numChannels;
                    //     sampleBuffer = new int32_t[numToRead * numChannels];
                    // }

                    // // reallocate if buffer size is too small
                    // if (sampleBufferSize < numToRead * numChannels)
                    // {
                    //     sampleBufferSize = numToRead * numChannels;
                    //     delete []sampleBuffer;
                    //     sampleBuffer = new int32_t[sampleBufferSize];
                    // }

                    if (av_read_frame(format, &packet) != 0) {
                        std::cout << "~ read frame" << std::endl;
                        break;
                    }

                    int gotFrame = 0;
                    if (avcodec_decode_audio4(codec, frame, &gotFrame, &packet) != 0) {
                        std::cout << "~ decode audio" << std::endl;
                        break;
                    }

                    if (!gotFrame) {
                        std::cout << "~ !gotFrame" << std::endl;
                        continue;
                    }

                    // resample frames
                    //float *buffer;
                    int frames = swr_convert(swr, (uint8_t**)&buffer, frame->nb_samples, (const uint8_t**)frame->data, frame->nb_samples);
                    frames = frame->nb_samples;

                    // fill buffers
                    ///*data = (double*) realloc(*data, (*size + frame->nb_samples) * sizeof(double));
                    ///memcpy(*data + *size, buffer, frames * sizeof(double));
                    ///*size += frames;

                    auto p1 = reservoir.getWritePointer (0, offset);
                    auto p2 = reservoir.getWritePointer (1, offset);

                    float *fp1 = p1;
                    float *fp2 = p2;
                    float *in = nullptr;//buffer;

                    std::cout << "frames read " << frames << std::endl;

                    for (int i = 0; i < frames; ++i)
                    {
                        *fp1 = *in;
                        in++;
                        fp1++;

                        *fp2 = *in;
                        in++;
                        fp2++;
                    }

                    numToRead -= frames;
                    offset += frames;

                    // const long samps = WavpackUnpackSamples (wvContext, sampleBuffer, numToRead);

                    // if (samps <= 0)
                    //     break;

                    // jassert (samps <= numToRead);

                    // auto p1 = reservoir.getWritePointer (0, offset);
                    // auto p2 = reservoir.getWritePointer (1, offset);

                    // float *fp1 = p1;
                    // float *fp2 = p2;
                    // int32_t *in = sampleBuffer;

                    // float maxF = 1.0f;
                    // if (bitsPerSample == 16)
                    //     maxF = 32767.0f;
                    // else if (bitsPerSample == 24)
                    //     maxF = 8388607.0f;
                    // else if (bitsPerSample == 32)
                    //     maxF = 32768.0f * 65536.0f;

                    // if (WavpackGetMode(wvContext) & MODE_FLOAT)
                    //     maxF = 1.0f;

                    // for (int i = 0; i < samps; ++i)
                    // {
                    //     *fp1 = (float)*in / maxF;
                    //     in++;
                    //     fp1++;

                    //     *fp2 = (float)*in / maxF;
                    //     in++;
                    //     fp2++;
                    // }

                    // numToRead -= samps;
                    // offset += samps;
                }

                std::cout << "--" << frame->nb_samples << std::endl;

                if (numToRead > 0)
                    reservoir.clear (offset, numToRead);
            }
        }

        if (numSamples > 0)
        {
            for (int i = numDestChannels; --i >= 0;)
                if (destSamples[i] != nullptr)
                    zeromem (destSamples[i] + startOffsetInDestBuffer, sizeof (int) * (size_t) numSamples);
        }

        return true;
    }

    //==============================================================================
    // static int32_t wv_read_bytes (void *id, void *data, int32_t bcount)
    // {
    //     return (int32_t) (static_cast<InputStream*> (id)->read (data, (int) bcount));
    // }

    // static uint32_t wv_get_pos (void *id)
    // {
    //     InputStream* const in = static_cast<InputStream*> (id);
    //     return in->getPosition ();
    // }

    // static int wv_set_pos_abs (void *id, uint32_t pos)
    // {
    //     InputStream* const in = static_cast<InputStream*> (id);
    //     in->setPosition (pos);
    //     return 0;
    // }

    // static int wv_push_back_byte (void *id, int c)
    // {
    //     InputStream* const in = static_cast<InputStream*> (id);

    //     if (0 == in->setPosition (in->getPosition() - 1))
    //     {
    //         return EOF;
    //     }

    //     return c;
    // }

    // static int wv_set_pos_rel (void *id, int32_t delta, int mode)
    // {
    //     InputStream* const in = static_cast<InputStream*> (id);

    //     if (mode == SEEK_CUR)
    //         delta += in->getPosition();
    //     else if (mode == SEEK_END)
    //         delta += in->getTotalLength();

    //     in->setPosition (delta);
    //     return 0;
    // }

    // static uint32_t wv_get_length (void *id)
    // {
    //     return static_cast<InputStream*> (id)->getTotalLength ();
    // }

    // static int wv_can_seek (void *id)
    // {
    //     return 1;
    // }

private:
    // WavPackNamespace::WavpackStreamReader wvReader;
    // WavPackNamespace::WavpackContext* wvContext;
    FFmpegNamespace::customAVCOntext *ffmpegContext;
    FFmpegNamespace::AVCodecContext *codec;
    FFmpegNamespace::AVFormatContext *format;
    FFmpegNamespace::AVStream *stream;
    FFmpegNamespace::SwrContext *swr;
    FFmpegNamespace::AVPacket packet;
    FFmpegNamespace::AVFrame *frame;
    int audioIndex;

    char wvErrorBuffer[80];
    AudioSampleBuffer reservoir;
    int reservoirStart, samplesInReservoir;
    float **buffer;
    //int32_t *sampleBuffer;
    //size_t sampleBufferSize;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FFmpegReader)
};

//==============================================================================
FFmpegAudioFormat::FFmpegAudioFormat()  : AudioFormat (wavPackFormatName, ".ape") // TODO: formats
{
}

FFmpegAudioFormat::~FFmpegAudioFormat()
{
}

Array<int> FFmpegAudioFormat::getPossibleSampleRates()
{
    const int rates[] = { 8000, 11025, 12000, 16000, 22050, 32000,
                          44100, 48000, 88200, 96000, 176400, 192000 };

    return Array<int> (rates, numElementsInArray (rates));
}

Array<int> FFmpegAudioFormat::getPossibleBitDepths()
{
    const int depths[] = { 16, 24, 32 };

    return Array<int> (depths, numElementsInArray (depths));
}

bool FFmpegAudioFormat::canDoStereo()    { return true; }
bool FFmpegAudioFormat::canDoMono()      { return true; }
bool FFmpegAudioFormat::isCompressed()   { return true; }

AudioFormatReader* FFmpegAudioFormat::createReaderFor (InputStream* in, const bool deleteStreamIfOpeningFails)
{
    ScopedPointer<FFmpegReader> r (new FFmpegReader (in));

    if (r->sampleRate > 0)
    {
        return r.release();
    }

    if (! deleteStreamIfOpeningFails)
        r->input = nullptr;

    return nullptr;
}

AudioFormatWriter* FFmpegAudioFormat::createWriterFor (OutputStream* out,
                                                        double sampleRate,
                                                        unsigned int numChannels,
                                                        int bitsPerSample,
                                                        const StringPairArray& metadataValues,
                                                        int qualityOptionIndex)
{
    jassertfalse; // not yet implemented!
    return nullptr;
}

StringArray FFmpegAudioFormat::getQualityOptions()
{
    // TODO: ?
    static const char* options[] = { "fast", "high", "very high", 0 };
    return StringArray (options);
}

#endif
