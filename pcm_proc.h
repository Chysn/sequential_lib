    /* PCM Processor (pcm_proc.h)
     *
     * The library allows parsing of WAV files into raw PCM data, and some types of manipulation
     * of PCM data.
     *
     * new_PCMData() creates a PCMData struct and initializes the data to default values.
     *
     * get_wav_meta() parses PCM data and metadata from a WAV file and returns a WAVData struct.
     * 
     * wav_to_pcm() parses PCM data and metadata from a WAV file and returns a PCMData struct.
     *
     * wav_extract() extracts data from a portion of a WAV file and returns a PCMData struct.
     * 
     * pcm_change_resolution() changes the resolution of PCM data (resolutions up to 32-bit signed
     *     PCM are supported).
     *
     * pcm_change_size() changes the size of PCM data.
     *
     * pcm_normalize() changes the amplitude of PCM data to a specified proportion of the maximum.
     *
     * pcm_from_channel() extracts a channel from PCM data and returns a new PCMData struct.
     *
     * pcm_trim() extracts a subset of data and returns a new PCMData struct.
     *
     * pcm_clone() copies the PCMData struct and returns the copy.
     *
     * set_pcm_data() is a shortcut for putting a data array into a PCMData struct, given its size
     *     per channel and the data array.
     * 
     * pcm_morph() generates a waveform that's a point in the interpolation from one waveform to
     *     another.
     * 
     * pcm_gen_saw() generates a 1024-sample sawtooth waveform
     * 
     * pcm_gen_square() generates a 1024-sample square waveform
     * 
     * Please see the bottom for boring license information.
     */

    #ifndef PCM_PROC_H_
    #include <stdlib.h>
    #define PCM_PROC_H_
    #define PCM_PROC_MAX 131072
    #define PCM_PROC_CHANNEL_LEFT 0
    #define PCM_PROC_CHANNEL_RIGHT 1

    /* Offsets for channel and resolution bytes in format chunk */
    #define WAV_CHANNEL_OFFSET 7
    #define WAV_RESOLUTION_OFFSET 19

    /* Frequently-used types in this library */
    typedef unsigned long int pcm_index_t;
    typedef unsigned long int pcm_size_t;
    typedef signed int pcm_sample_t;

    /* PCMData is the primary operating structure for this library. It contains PCM data, and
     * keeps track of the resolution (16-bit, 24-bit, 32-bit) and number of channels. Note that
     * size is the number of samples per channel rather than the total size.
     */
    typedef struct _PCM_DATA {
       pcm_size_t size; /* Number of samples per channel */
       pcm_sample_t data[PCM_PROC_MAX];
       int channels;
       int resolution;
    } PCMData;

    typedef struct _WAV_META {
        pcm_size_t data_start;
        pcm_size_t data_end;
        pcm_size_t samples;
        int channels;
        int resolution;
    } WAVMeta;

    /* Function declarations */
    PCMData new_PCMData();
    WAVMeta get_wav_meta(pcm_size_t size, pcm_sample_t data[]);
    PCMData wav_to_pcm(pcm_size_t size, pcm_sample_t data[]);
    PCMData wav_extract(WAVMeta meta, pcm_sample_t data[], pcm_size_t start, pcm_size_t size);
    void pcm_change_resolution(PCMData *pcm, int new_resolution);
    void pcm_change_size(PCMData *pcm, pcm_size_t new_size);
    void pcm_normalize(PCMData *pcm, float new_amplitude);
    PCMData pcm_from_channel(PCMData *pcm, int channel);
    PCMData pcm_trim(PCMData *pcm, pcm_index_t start, pcm_size_t size);
    PCMData pcm_clone(PCMData *pcm);
    void set_pcm_data(PCMData *pcm, pcm_size_t size, pcm_sample_t data[]);
    PCMData pcm_morph(PCMData *low, PCMData *high, float scale);
    PCMData pcm_gen_saw();
    PCMData pcm_gen_square();

    /* Sets initial data for a PCMData struct */
    PCMData new_PCMData()
    {
        PCMData pcm;
        pcm.resolution = 16;
        pcm.channels = 1;
        pcm.size = 0;
        return pcm;
    }

    /* Parses a WAV file header to get the data, size, resolution, and number of channels */
    WAVMeta get_wav_meta(pcm_size_t size, pcm_sample_t data[])
    {
        WAVMeta meta;
        if (size > 44) {
            /* Get selected metadata (number of channels and resolution) from a WAV file */
            int channels = 0;
            int resolution = 0;

            char dcid[] = "data"; /* Data chunk ID */
            int dcx = 0;

            char fcid[] = "fmt "; /* Format chunk ID */
            int fcx = 0;
           
            pcm_index_t d_st = 0; /* Index for beginning of "data" chunk */
            pcm_index_t d_end = 0; /* Index for end of "data" chunk */

            pcm_index_t i;
            for (i = 0; i < size; i++)
            {
                char b = data[i];
               
                /* Find the "fmt " chunk and determine the number of channels and resolution */
                if (b == fcid[fcx] && channels == 0) {
                    if (fcx == 3) {
                        /* This index i is the end of the format chunk identifier. */
                        if (i + WAV_CHANNEL_OFFSET < size) channels = data[i + WAV_CHANNEL_OFFSET];
                        if (i + WAV_RESOLUTION_OFFSET < size) resolution = data[i + WAV_RESOLUTION_OFFSET];
                    }
                    fcx++;
                } else {
                    fcx = 0; /* The byte didn't match the next character in the chunk ID, so reset */
                }
               
                /* Find the "data" chunk and determine the beginning and end of the PCM data */
                if (b == dcid[dcx] && d_end == 0) {
                    if (dcx == 3) {
                        /* This index i is the end of the data chunk identifier. The data begins
                         * at this index + 5. The size is a 32-bit value in the four bytes from
                         * i+1 to i+4.
                         */
                        d_st = i + 5;
                        d_end = d_st + data[i+1] + (data[i+2] << 8) + (data[i+3] << 16) + (data[i+4] << 24);
                    }
                    dcx++;
                } else {
                    dcx = 0; /* The byte didn't match the next character in the chunk ID, so reset */
                }
               
                /* If we have all the data we need, get out of here */
                if (d_st && d_end && channels && resolution) break;
            }

            meta.data_start = d_st;
            meta.data_end = d_end;
            meta.channels = channels;
            meta.resolution = resolution;
            meta.samples = (d_end - d_st) / (resolution / 8);
        }

        return meta;
    }

    PCMData wav_to_pcm(pcm_size_t size, pcm_sample_t data[])
    {
        WAVMeta meta = get_wav_meta(size, data);
        PCMData pcm = wav_extract(meta, data, 0, meta.samples);
        return pcm;
    }

    PCMData wav_extract(WAVMeta meta, pcm_sample_t data[], pcm_size_t start, pcm_size_t samples)
    {
        pcm_sample_t pcm_data[PCM_PROC_MAX];
        pcm_index_t ix = 0; // Index within new PCM data
        pcm_index_t dx = meta.data_start + (start * (meta.resolution / 8)); // Index within WAV data
        int i;
        for (i = 0; i < samples; i++)
        {
            int ch;
            for (ch = 0; ch < meta.channels; ch++)
            {
                int bn;
                pcm_sample_t sample = 0;
                for (bn = 0; bn < (meta.resolution / 8); bn++)
                {
                    if (dx <= meta.data_end) { // Avoid buffer overread
                        char b = data[dx++];
                        sample += (b << (bn * 8));
                    }
                }
                pcm_data[ix++] = sample;
            }
        }

        PCMData pcm = new_PCMData();
        pcm_change_resolution(&pcm, meta.resolution);
        pcm.channels = meta.channels;
        set_pcm_data(&pcm, ix, pcm_data);
        return pcm;
    }

    /* Changes the resolution of the PCMData passed by reference. Resolution may be 8, 16, 24, or 32. */
    void pcm_change_resolution(PCMData *pcm, int new_resolution)
    {
        if (new_resolution > 32 || new_resolution < 8) new_resolution = pcm->resolution;

        if (pcm->resolution != new_resolution) {
            pcm_sample_t data[PCM_PROC_MAX];
            pcm_index_t i; /* Index within PCM data */
            for (i = 0; i < (pcm->size * pcm->channels); i++)
            {
                pcm_sample_t sample = pcm->data[i];
                if (pcm->resolution == 8) {
                    /* 8-bit WAV files are unsigned data, rather than signed. So I need to convert
                       the value to a two's complement 8-bit signed value. */
                    sample -= 128;
                }
                if (new_resolution < pcm->resolution) {
                    /* The target resolution is lower; shift bits right to downsample, but use
                       the most significant shifted-away bit to determine whether this sample
                       should be rounded up (if positive) or down (if negative). */
                    
                    /* Find maximum and minimum values (for range-checking) */
                    pcm_sample_t max = (1 << new_resolution) - 1;
                    pcm_sample_t min = -1 - max;
                    
                    /* Shift away unused bits, keeping the round bit to determine rounding direction */
                    sample >>= (pcm->resolution - new_resolution - 1);
                    int round_up = ((sample & 1) && (sample >= 0));
                    int round_dn = (!(sample & 1) && (sample < 0));

                    /* Shift away the round bit, and perform any necessary rounding up or down */
                    sample >>= 1;
                    if (round_up && sample < max) sample += 1;
                    if (round_dn && sample > min) sample -= 1;
                }
                if (new_resolution > pcm->resolution) {
                    /* The target resolution is highter; shift bits left */
                    sample <<= (new_resolution - pcm->resolution);
                }
                if (new_resolution == 8) {
                    /* See above. If the new PCM is 8 bits, it will be unsigned */
                    sample += 128;
                    sample &= 0xff;
                }

                data[i] = sample;
            }
           
            pcm->resolution = new_resolution;
            set_pcm_data(pcm, pcm->size * pcm->channels, data);
        }
        return;
    }

    /* Changes the size of the PCMData passed by reference.
     *
     * The PCM is expanded to fit (by linear interpolation) if the new size is larger than the
     * current size. The PCM is collapased to fit if the new size is smaller than the current size.
     */
    void pcm_change_size(PCMData *pcm, pcm_size_t new_size)
    {
        if (new_size > (PCM_PROC_MAX / pcm->channels)) new_size = PCM_PROC_MAX / pcm->channels;

        /* At least two samples per channel are required for interpolation to work */
        if (pcm->size > 1) {
            pcm_index_t i; /* General use index iterator */
           
            pcm_sample_t data[PCM_PROC_MAX];
            pcm_size_t data_size = pcm->size;
            for (i = 0; i < pcm->size; i++) data[i] = pcm->data[i];
           
            /* If the target PCM size is greater than the source size, interpolation is necessary
             * to fill in gaps created by expansion. This processor uses super-simple linear
             * interpolation; create one gap between each two samples and fill in these gaps with
             * the mean value of the two adjacent samples. Do this until the size is as big as
             * or greater than the target size.
             */
            while (data_size < new_size)
            {
                pcm_sample_t expanded_data[PCM_PROC_MAX];
                pcm_index_t ex = 0; /* Index within the expanded PCM */
                for (i = 0; i < data_size ; i++)
                {
                    int ch;
                    for (ch = 0; ch < pcm->channels; ch++)
                    {
                        pcm_index_t dx = (i * pcm->channels + ch); /* Index within data */
                        pcm_index_t nx = (dx + pcm->channels); /* Next index for same channel */
                        expanded_data[ex++] = data[dx];
                        if (nx < (data_size * pcm->channels)) {
                            int16_t dx_v = (int16_t) data[dx];
                            int16_t nx_v = (int16_t) data[nx];
                            expanded_data[ex++] = (dx_v / 2) + (nx_v / 2);
                            //printf("%lu: %d -- %d = %d\n", i, data[dx], data[nx], expanded_data[ex-1]);
                        }
                    }
                }
           
                /* Adjust the size, then copy the expanded pcm to pcm, potentially for another iteration */
                data_size = ex / pcm->channels;
                for (i = 0; i < ex; i++) data[i] = expanded_data[i];
            }
       
            /* Collapse the pcm data into {size} equally-spaced samples */
            float inc = (float) data_size / new_size; /* Increment, the size of each slice */
            pcm_index_t dx = 0; /* Index within the target data */
            pcm_sample_t resized_data[PCM_PROC_MAX];
            for (i = 0; i < new_size; i++)
            {
                int ch;
                for (ch = 0; ch < pcm->channels; ch++)
                {
                    pcm_index_t ix = (i * inc * pcm->channels) + ch; /* Index for channel within PCM data */
                   
                    /* Avoid buffer overread: */
                    if (ix >= (data_size * pcm->channels)) ix = data_size * pcm->channels;

                    resized_data[dx++] = data[ix];
                }
            }
           
            set_pcm_data(pcm, dx, resized_data);
        }
       
        return;
    }

    /*
     * Normalizes the PCM amplitude to a proportion of the maximum possible amplitude,
     * based on the PCMData's resolution.
     */
    void pcm_normalize(PCMData *pcm, float new_amplitude)
    {
        int max = ((0x01 << pcm->resolution) / 2) - 1; /* Largest sample value based on resolution */
       
        /* Locate the peak sample in the PCM, among all channels. */
        int peak = 0;
        pcm_index_t i;
        for (i = 0; i < (pcm->size * pcm->channels); i++)
        {
            int pos_sample = abs(pcm->data[i]);
            if (pos_sample > peak) peak = pos_sample;
        }
       
        /* Calculate the coeffecient required to get the peak to the specified amplitude */
        float coeff = peak ? (max * new_amplitude) / peak  : 1;

        /* Normalize using the coeffecient */
        pcm_sample_t data[PCM_PROC_MAX];
        for (i = 0; i < (pcm->size * pcm->channels); i++) data[i] = pcm->data[i] * coeff;
        set_pcm_data(pcm, pcm->size, data);
       
        return;
    }

    /*
     * Extracts data for a single channel from the PCMData passed by reference, and returns a new
     * PCMData.
     *
     * channel may be an integer, or PCM_PROC_CHANNEL_LEFT or PCM_PROC_CHANNEL_RIGHT. If the channel
     * provided is out-of-range, the left channel (channel 0) is extracted.
     */
    PCMData pcm_from_channel(PCMData *pcm, int channel)
    {
        if (channel > (pcm->channels - 1) || channel < 0) channel = PCM_PROC_CHANNEL_LEFT;

        pcm_sample_t data[PCM_PROC_MAX];
        pcm_index_t ic; /* Index within selected channel's PCM data */
        for (ic = 0; ic < pcm->size; ic++)
        {
            int ch;
            for (ch = 0; ch < pcm->channels; ch++)
            {
                if (ch == channel) {
                    /* Get the sample for only the specfied channel */
                    data[ic] = pcm->data[ic * pcm->channels + ch];
                    break;
                }
            }
        }
       
        PCMData converted = new_PCMData();
        converted.resolution = pcm->resolution;
        set_pcm_data(&converted, pcm->size, data);
       
        return converted;
    }

    /*
     * Extracts data for a specific number of samples per channel from the PCMData passed by reference,
     * and returns a new PCMData.
     */
    PCMData pcm_trim(PCMData *pcm, pcm_index_t start, pcm_size_t size)
    {
        PCMData trimmed = new_PCMData();
       
        if (pcm->size >= (start + size)) {
            pcm_sample_t data[PCM_PROC_MAX];
            pcm_index_t offset = start * pcm->channels;
           
            pcm_index_t i;
            pcm_index_t new_ix = 0; /* Index within the new PCM */
            for (i = 0; i < (size * pcm->channels); i++) data[new_ix++] = pcm->data[i + offset];
           
            trimmed.channels = pcm->channels;
            trimmed.resolution = pcm->resolution;
            set_pcm_data(&trimmed, size * pcm->channels, data);
        }
       
        return trimmed;
    }

    /* Returns a clone of a PCMData, passed by reference. */
    PCMData pcm_clone(PCMData *pcm)
    {
        PCMData clone = new_PCMData();
        clone.size = pcm->size;
        clone.resolution = pcm->resolution;
        clone.channels = pcm->channels;
        set_pcm_data(&clone, pcm->size * pcm->channels, pcm->data);
        return clone;
    }

    /*
     * Updates a PCMData with size and data array.
     *
     * Note that the size overall array size rather than the per-channel size.
     */
    void set_pcm_data(PCMData *pcm, pcm_size_t size, pcm_sample_t data[])
    {
       pcm->size = size / pcm->channels;
       pcm_index_t i;
       for (i = 0; i < size; i++) pcm->data[i] = data[i];
    }

    /* Returns a PCM waveform that is a partial morph between the start PCM
     * wave and the end PCM wave, at the specified scale from >0 to <1
     */
    PCMData pcm_morph(PCMData *start, PCMData *end, float scale)
    {
        PCMData morphed = new_PCMData();

        pcm_sample_t pcm_data[PCM_PROC_MAX];
        pcm_index_t i;
        for (i = 0; i < start->size; i++)
        {
            float diff = end->data[i] - start->data[i]; /* PCM steps from start to end points */
            float v = start->data[i] + (diff * scale);
            pcm_data[i] = (pcm_sample_t) v; /* Cast to sample and set data point */
        }
        set_pcm_data(&morphed, start->size, pcm_data);

        return morphed;
    }

    PCMData pcm_gen_saw()
    {
        pcm_sample_t data[PCM_PROC_MAX];

        pcm_index_t i;
        for (i = 0; i < 512; i++)
        {
            data[i] = (pcm_sample_t) (i * 64);
            data[i+512] = (pcm_sample_t) (i * 64 - 32768);
        }

        PCMData saw = new_PCMData();
        set_pcm_data(&saw, 1024, data);

        return saw;
    }

    PCMData pcm_gen_square()
    {
        pcm_sample_t data[PCM_PROC_MAX];

        pcm_index_t i;
        for (i = 0; i < 512; i++)
        {
            data[i] = (pcm_sample_t) 32767;
            data[i+512] = (pcm_sample_t) -32768;
        }

        PCMData saw = new_PCMData();
        set_pcm_data(&saw, 1024, data);

        return saw;
    }

    #endif /* PCM_PROC_H_ */

/*
 * Copyright (c) 2014, 2020 The Beige Maze Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
