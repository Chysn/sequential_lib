/*                  Sequential Packing (sequential_packing.h)
 *                        Tenth Anniversary Edition
 *
 * Copyright (c) 2010, 2012, 2013, 2020 The Beige Maze Project
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
 *
 *******************************************************************************
 *
 * Sequential System Exclusive data is arranged in 8-byte packets.  The first
 * byte in each packet is a composite of the high bits of the next seven data
 * bytes.
 *
 * Four functions are provided in this header:
 *
 * Seq_unpack() converts a single Sequential system exclusive dump values 
 * into a series of data bytes, so that the values may be freely manipulated
 *
 * Seq_pack() converts an array of Sequential data into the packed format,
 * so that it may be transmitted via system exclusive to the appropriate
 * synth.
 *
 * Seq_set() is a shortcut for putting a value array into a SequentialData
 * struct, given its size and the value array.
 *
 * Seq_dump() sends the values to standard output.
 *
 */
#ifndef SEQUENTIAL_PACKING_H_
#include <stdio.h>
#define SEQUENTIAL_PACKING_H_
#define SEQUENTIAL_DATA_MAX 128000

/*
 * _SequentialData is a sort of generic data structure, containing a size, 
 * and a fixed-length array of data.  Since it's possible to use the same data 
 * structure to represent both packed and unpacked data, several typedefs are 
 * included to disambiguate the role of the data in your code.
 */
typedef struct _SequentialData {
    int size;
    unsigned int value[SEQUENTIAL_DATA_MAX];
} SequentialData, UnpackedData, PackedData;

/* Function declarations */
UnpackedData Seq_unpack(PackedData packed);
PackedData Seq_pack(UnpackedData unpacked);
void Seq_set(SequentialData *voice, int size, unsigned int values[]);
void Seq_dump(SequentialData voice);

/*
 * Given packed data (for example, the data that would come directly from a
 * Sequential instrument's system exclusive dump), unpack() returns an 
 * UnpackedData, whose data property contains a one-byte-per-parameter 
 * representation of the data.  The UnpackedData will be an easy way to examine, 
 * manipulate, and modify data.
 *
 * Example:
 *
 *   (Accept a system exclusive dump.  Process the sysex header yourself, and
 *    then put the values into value, an array of unsigned ints.  Store the size of 
 *    the value array in int size)
 *   PackedData packed_sysex;
 *   Seq_set(&packed_sysex, size, values);
 *   UnpackededData mopho_voice = Seq_unpack(packed_sysex);
 *   int cutoff_frequency = mopho_voice.value[20];
 */
UnpackedData Seq_unpack(PackedData packed)
{
    unsigned int values[SEQUENTIAL_DATA_MAX];
    int packbyte = 0;  /* Composite of high bits of next 7 bytes */
    int pos = 0;       /* Current position of 7 */
    int ixp;           /* Packed byte index */
    int size = 0;      /* Unpacked voice size */
    unsigned int c;    /* Current source byte */
    for (ixp = 0; ixp < packed.size; ixp++)
    {
        c = packed.value[ixp];
        if (pos == 0) {
            packbyte = c;
        } else {
            if (packbyte & (1 << (pos - 1))) {c |= 0x80;}
            values[size++] = c;
        }
        pos++;
        pos &= 0x07;
        if (size > SEQUENTIAL_DATA_MAX) break;
    }
    
    UnpackedData unpacked;
    Seq_set(&unpacked, size, values);
    return unpacked;
}


/*
 * Given unpacked data (for example, data that might be modified or created by
 * calling software), pack() returns a PackedData, whose data property contains 
 * a packed representation of the data.  The PackedData is suitable for sending 
 * back to a Sequential instrument.  To send the packed data back via a file or 
 * direct I/O call, see dump(); or roll your own I/O to MIDI.  Don't forget the
 * appropriate system exclusive header.
 * 
 * Example:
 *
 *   (Let's continue the example from unpack() above, by opening the filter all 
 *    the way. As you may recall, mopho_voice_data is an UnpackedData)
 *   mopho_voice.value[20] = 164;
 *   PackedData mopho_sysex = Seq_pack(mopho_voice);
 */
PackedData pack(UnpackedData unpacked)
{
    unsigned int values[SEQUENTIAL_DATA_MAX];
    int packbyte = 0;  /* Composite of high bits of next 7 bytes */
    int pos = 0;       /* Current position of 7 */
    int ixu;           /* Unpacked byte index */
    int size = 0;      /* Packed voice size */
    int packet[7];     /* Current packet */
    int i;             /* Packet output index */
    unsigned int c;    /* Current source byte */
    for (ixu = 0; ixu < unpacked.size; ixu++)
    {
        c = unpacked.value[ixu];
        if (pos == 7) {
            values[size++] = packbyte;
            for (i = 0; i < pos; i++) values[size++] = packet[i];
            packbyte = 0;
            pos = 0;
        }
        if (c & 0x80) {
            packbyte += (1 << pos);
            c &= 0x7f;
        }
        packet[pos++] = c;
        if ((size + 8) > SEQUENTIAL_DATA_MAX) break;
    }
    values[size++] = packbyte;
    for (i = 0; i < pos; i++) values[size++] = packet[i];
    PackedData packed;
    Seq_set(&packed, size, values);
    return packed;
}


/*
 * Updates an UnpackedDara or a PackedData with the size and data array.
 *
 * If this was object-oriented code, this would be part of the constructor.
 * But since this is C, and SequentialData is a struct instead of a class, 
 * populating it is a two-step process.  If you have a data array (an array of
 * unsigned ints representing the packed or unpacked data) called data, and 
 * know its size, you set up the struct like (for example) this:
 *
 *   UnpackedData voice;
 *   Seq_set(&voice, size, data);
 *
 * Now you can access the individual parameters with voice.data[index].  See 
 * dump() below for a really simple example of this.
 */
void Seq_set(SequentialData *data, int size, unsigned int values[])
{
    data->size = size;
    int i;
    for (i = 0; i <  size; i++) data->value[i] = values[i];
}


/*
 * Sends the data to standard output.  The data may be either packed or 
 * unpacked. If it's packed, it can be used to create a system exclusive file.  
 * Note that this is only the parameter data and not the system exclusive header
 */
void Seq_dump(SequentialData data)
{
    int i;
    for (i = 0; i < data.size; i++) putchar(data.value[i]);
}

#endif /* SEQUENTIAL_PACKING_H_ */
