/* pro_wavetable.h
 * (c) 2020, The Beige Maze Project
 * 
 * pro3_wavetable.h provides functions for generating wavetable system
 * exclusive data for the Sequential Pro 3 synthesizer.
 * 
s */
#include "sequential_packing.h"
#include "pcm_proc.h"
#define PCM_MAX 176000
#define PRO3_SAMPLE_SIZE 1024
#define PRO3_WAVES 16

/* A Wavetable is a set of 16 reference waveforms represented as PCM */
typedef struct _Wavetable {
    pcm_sample_t ref[PRO3_WAVES][PRO3_SAMPLE_SIZE];
    int isset[PRO3_WAVES];
} Wavetable;

/* Function declarations */
Wavetable new_Wavetable();
void set_reference(Wavetable *table, PCMData *reference, int num);
void wavetable_fill(Wavetable *table);
void wavetable_sysex_dump(Wavetable *table, int num, char *name);
void wavetable_pcm_dump(Wavetable *table);

/* Create a new empty PRO 3 wavetable */
Wavetable new_Wavetable() 
{
    Wavetable table;
    int i;
    for (i = 0; i < PRO3_WAVES; i++) 
    {
        table.isset[i] = 0;
        int j;
        for (j = 0; j < PRO3_SAMPLE_SIZE; j++) table.ref[i][j] = 0;
    }
    return table;
}

/* Insert a PCM waveform into a Wavetable 
 * Prior to insertion, set waveform to 16-bit, 1024 samples
 */
void set_reference(Wavetable *table, PCMData *reference, int num)
{
    PCMData clone = pcm_clone(reference);
    if (clone.size != PRO3_SAMPLE_SIZE) pcm_change_size(&clone, PRO3_SAMPLE_SIZE);
    if (clone.resolution != 16) pcm_change_resolution(&clone, 16);
    int i;
    for (i = 0; i < PRO3_SAMPLE_SIZE; i++)
    {
        table->ref[num][i] = clone.data[i];
    }
    table->isset[num] = 1;
    return;
}

/* Fill in empty reference waveforms by morphing with linear interpolation.
 * At least one reference waveform must be set in 0
 */
void wavetable_fill(Wavetable *table)
{
    /* If the first waveform isn't set, return return without doing anything */
    if (table->isset[0] == 0) return;

    /* If the last waveform isn't set, fill the last position with the first wave */
    if (table->isset[PRO3_WAVES - 1] == 0) {
        int i;
        for (i  = 0; i < PRO3_SAMPLE_SIZE; i++)
        {
            table->ref[PRO3_WAVES - 1][i] = table->ref[0][i];
        }
        table->isset[PRO3_WAVES - 1] = 1;
    }

    int c;
    for (c = 1; c < 16; c++) /* c = current index */
    {
        if (table->isset[c] == 0) {
            /* There's no waveform here, so it needs to be filled */
            int n;
            for (n = c; n < 16; n++) /* n = next index */
            {
                if (table->isset[n] == 1) {
                    /* This is the next set waveform with data, so fill every waveform
                     * between the current one and this one */
                    int t;
                    for (t = c; t < n; t++) /* t = target index */
                    {
                        float div = n - c; /* Number of reference waves in range */
                        float ix = t - c + 1; /* Index of target within the range */
                        float scale = ix / div;
                        int i;
                        for (i = 0; i < PRO3_SAMPLE_SIZE; i++)
                        {
                            float pcm_diff = table->ref[n][i] - table->ref[c-1][i];
                            float v = table->ref[c-1][i] + (pcm_diff * scale);
                            table->ref[t][i] = (pcm_sample_t) v;
                        }
                    }
                    break;
                }   
            }
        }
    }

    return;
}

void wavetable_sysex_dump(Wavetable *table, int num, char *name)
{
    /* PCM data is signed, while the dsi_packing tools require data to be unsigned. So,
     * conversion must be done. First, I cast each sample to an int16_t to guarantee that
     * it's a 16-bit signed integer. Then, I divide the 16-bit word into bytes and
     * place them, big-endian, into an pro3 data array.
     */
    unsigned int pro3_data[PCM_MAX];
    unsigned long int i; /* i is the index within the PCM data */
    unsigned long int dx = 0; /* dx is the index within the Pro 3 data */    
    uint16_t checksum = 0;
    
    int k;
    for (k = 0; k < 16; k++)
    {
	    PCMData pcm = new_PCMData();
        set_pcm_data(&pcm, PRO3_SAMPLE_SIZE, table->ref[k]);
	    
	    for (i = 0; i < pcm.size; i++)
	    {
	    	int16_t sample = (int16_t)pcm.data[i];
	
	        /* Convert the signed 16-bit sample into a high and low byte */
	        pro3_data[dx++] = (sample) >> 8; /* High byte */
	        pro3_data[dx++] = (sample) & 0xff; /* Low byte */
	        uint16_t check = (((uint16_t) sample >> 8) | ((uint16_t) sample << 8));
	        checksum += check;
	    }	    
	    
	    /* Downsample to a 512-word waveform */
	    pcm_change_size(&pcm, PRO3_SAMPLE_SIZE / 2);
	    for (i = 0; i < pcm.size; i++)
	    {
	    	int16_t sample = (int16_t)pcm.data[i];
	        pro3_data[dx++] = (sample) >> 8; /* High byte */
	        pro3_data[dx++] = (sample) & 0xff; /* Low byte */
	        uint16_t check = (((uint16_t) sample >> 8) | ((uint16_t) sample << 8));
	        checksum += check;
	    }
	    
	    /* Downsample to a 256-word waveform and add it twice */
	    pcm_change_size(&pcm, PRO3_SAMPLE_SIZE / 4);
	    int j;
	    for (j = 0; j < 2; j++) 
	    {
		    for (i = 0; i < pcm.size; i++)
		    {
		    	int16_t sample = (int16_t)pcm.data[i];
		        pro3_data[dx++] = (sample) >> 8; /* High byte */
		        pro3_data[dx++] = (sample) & 0xff; /* Low byte */
	            uint16_t check = (((uint16_t) sample >> 8) | ((uint16_t) sample << 8));
	            checksum += check;
		    }
		}
	    
	    /* Downsample to a 128-word waveform and add it eight times */
	    pcm_change_size(&pcm, PRO3_SAMPLE_SIZE / 8);
	    for (j = 0; j < 8; j++) 
	    {
		    for (i = 0; i < pcm.size; i++)
		    {
		    	int16_t sample = (int16_t)pcm.data[i];
		        pro3_data[dx++] = (sample) >> 8; /* High byte */
		        pro3_data[dx++] = (sample) & 0xff; /* Low byte */
    	        uint16_t check = (((uint16_t) sample >> 8) | ((uint16_t) sample << 8));
	            checksum += check;
		    }
		}		
	}
	
    /* Use dsi_packing tools to convert the sample data into DSI's packed format */
    UnpackedData pro3_wavetable;
    Seq_set(&pro3_wavetable, dx, pro3_data);
    PackedData pro3_sysex = Seq_pack(pro3_wavetable);
  
    /* Send the actual SysEx to standard output */
    putchar(0xf0); /* Start SysEx */
    putchar(0x01); /* DSI */
    putchar(0x31); /* Pro3 */
    putchar(0x6a);
    putchar(0x6c);
    putchar(0x01);
    putchar(0x6b);
    putchar(num);
    while (strlen(name) < 8) strcat(name, " "); /* Enforce 8 characters */
    printf("%.8s", name);
    putchar(0x00);
    Seq_dump(pro3_sysex);
    putchar((char) (checksum & 0x7f));
    putchar((char) (checksum >> 8) & 0x7f);
    putchar(0xf7); /* End SysEx */
}

void wavetable_pcm_dump(Wavetable *table)
{
    int r;
    for (r = 0; r < PRO3_WAVES; r++)
    {
        int s;
        for (s = 0; s < PRO3_SAMPLE_SIZE; s++)
        {
            pcm_sample_t sample = table->ref[r][s];

            /* Convert the signed 16-bit sample into a high and low byte */
            char hi = (sample) >> 8; /* High byte */
            char lo = (sample) & 0xff; /* Low byte */
            putchar(hi);
            putchar(lo);
        }
    }
}

/*
 * Copyright (c) 2020 The Beige Maze Project
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
