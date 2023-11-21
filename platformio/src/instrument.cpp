/** \file instrument.cpp
*/
#include <ArduinoLog.h>
#undef CR
#include <SD.h>
#include "instrument.h"
static const int chipSelect = BUILTIN_SDCARD;

char filePosition[10];

CInstrument* CInstrument::singleton = nullptr;

CInstrument* CInstrument::getInstance()
{
    if (singleton == nullptr)
        singleton = new CInstrument();
    return singleton;
}

void CInstrument::updateFilePosition(File* file)
{
    snprintf(filePosition, 10, "%llu", file->position());
}

void CInstrument::dumpHexBytes(const uint8_t *bytes, size_t count)
{
    char row[256];
    size_t index = count - (count % 8);
    Log.verbose("Line %d bytes %x count %d index %d\n", __LINE__, bytes, count, index);
    size_t i = 0;
    for (; i < index; i = i + 8 )
    {
        snprintf(row, 256, "%lx %02x %02x %02x %02x %02x %02x %02x %02x\n",
            (long unsigned int)(bytes + i), bytes[i], bytes[i + 1], bytes[i + 2], bytes[i + 3],
            bytes[i + 4], bytes[i + 5], bytes[i + 6], bytes[i + 7]);
        Log.verbose("Line %d row %s",__LINE__, row);
    }

    if (index != count)
    {
        char * row_ptr = row;

        // Ordinary printf is safe here
        row_ptr = row_ptr + sprintf(row_ptr, "%lx ", (long unsigned int)(bytes + i));
        for (; i < count; i++)
        {
            row_ptr = row_ptr + sprintf(row_ptr, "%02x ", bytes[i]);
        }
        Log.verbose("Line %d row %s\n", __LINE__, row);
    }
}

void CInstrument::dumpHexBytes(const uint32_t *bytes, size_t count)
{
    char row[256];
    size_t index = count - (count % 8);
    Log.verbose("Line %d bytes %x count %d index %d\n", __LINE__, bytes, count, index);
    size_t i = 0;
    for (; i < index; i = i + 8 )
    {
        snprintf(row, 256, "%lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
            (long unsigned int)(bytes + i), bytes[i], bytes[i + 1], bytes[i + 2], bytes[i + 3],
            bytes[i + 4], bytes[i + 5], bytes[i + 6], bytes[i + 7]);
        Log.verbose("Line %d row %s",__LINE__, row);
    }

    if (index != count)
    {
        char * row_ptr = row;

        // Ordinary printf is safe here
        row_ptr = row_ptr + sprintf(row_ptr, "%lx ", (long unsigned int)(bytes + i));
        for (; i < count; i++)
        {
            row_ptr = row_ptr + sprintf(row_ptr, "%08lx ", bytes[i]);
        }
        Log.verbose("Line %d row %s\n", __LINE__, row);
    }
}

void CInstrument::dumpInstrumentData(AudioSynthWavetable::instrument_data *instrumentData)
{
        char buf[256];
        snprintf(buf, 256, "Instrument data\n"
                            "===============\n"
                            "Number of samples %d\n"
                            "Sample note ranges pointer %p\n",
                            instrumentData->sample_count,
                            instrumentData->sample_note_ranges
                            );
        Log.verbose("%s", buf);
}

void CInstrument::dumpSampleMetadata(struct AudioSynthWavetable::sample_data* metadata)
{
    char buf[2500];

	snprintf(buf, 2500,
            "sample %lx "
            "LOOP %d " 
            "INDEX_BITS %d "
            "PER_HERTZ_PHASE_INCREMENT %f "
            "MAX_PHASE %ld "
            "LOOP_PHASE_END %ld "
            "LOOP_PHASE_LENGTH %ld "
            "INITIAL_ATTENUATION_SCALAR %d\n"

	// VOLUME ENVELOPE VALUE
            "DELAY_COUNT %ld "
            "ATTACK_COUNT %ld "
            "HOLD_COUNT %ld "
            "DECAY_COUNT %ld "
            "RELEASE_COUNT %ld "
            "SUSTAIN_MULT %ld\n"

	// VIRBRATO VALUES
            "VIBRATO_DELAY %ld "
            "VIBRATO_INCREMENT %ld "
            "VIBRATO_PITCH_COEFFICIENT_INITIAL %f "
            "VIBRATO_PITCH_COEFFICIENT_SECOND %f\n"

	// MODULATION VALUES
            "MODULATION_DELAY %ld "
            "MODULATION_INCREMENT %ld "
            "MODULATION_PITCH_COEFFICIENT_INITIAL %f "
            "MODULATION_PITCH_COEFFICIENT_SECOND %f "
            "MODULATION_AMPLITUDE_INITIAL_GAIN %ld "
            "MODULATION_AMPLITUDE_SECOND_GAIN %ld\n",


            (unsigned long int)metadata->sample, metadata->LOOP, metadata->INDEX_BITS, 
            metadata->PER_HERTZ_PHASE_INCREMENT, metadata->MAX_PHASE, metadata->LOOP_PHASE_END,
            metadata->LOOP_PHASE_LENGTH, metadata->INITIAL_ATTENUATION_SCALAR, metadata->DELAY_COUNT,
            metadata->ATTACK_COUNT, metadata->HOLD_COUNT, metadata->DECAY_COUNT, metadata->RELEASE_COUNT,
            metadata->SUSTAIN_MULT, metadata->VIBRATO_DELAY, metadata->VIBRATO_INCREMENT,
            metadata->VIBRATO_PITCH_COEFFICIENT_INITIAL, metadata->VIBRATO_PITCH_COEFFICIENT_SECOND,
            metadata->MODULATION_DELAY, metadata->MODULATION_INCREMENT,
            metadata->MODULATION_PITCH_COEFFICIENT_INITIAL, metadata->MODULATION_PITCH_COEFFICIENT_SECOND,
            metadata->MODULATION_AMPLITUDE_INITIAL_GAIN, metadata->MODULATION_AMPLITUDE_SECOND_GAIN);
}

struct AudioSynthWavetable::instrument_data* CInstrument::load(const char *name)
{
    char tmp_buffer[128];

    if (!SD.begin(chipSelect))
    {
        Log.verbose("Line %d SD card intialisation failed\n", __LINE__);
        return nullptr;
    }

    if (!SD.exists(name))
    {
        Log.verbose("Line %d Cannot find file %s\n", __LINE__, name);
        return nullptr;
    }
    
    Log.verbose("Line %d Opening file %s\n", __LINE__, name);
    File data = SD.open(name);
    if (!data)
    {
        Log.verbose("Line %d Unable to open data file\n", __LINE__);
        return nullptr;
    }

    // 1. Read the number of samples (uint8_t)
    Log.verbose("Line %d sizeof(uint8_t) %d sizeof instrument_data.sample_count %d\n", __LINE__, sizeof(uint8_t), sizeof instrument_data.sample_count);
    if (data.read(&instrument_data.sample_count, sizeof instrument_data.sample_count) != sizeof instrument_data.sample_count)
    {
        Log.verbose("Failed to read number of samples\n", __LINE__);
        data.close();
        return nullptr;
    }
    Log.verbose("Line %d Number of samples %d\n", __LINE__, instrument_data.sample_count);
    updateFilePosition(&data);
    Log.verbose("Line %d file position before reading raw sample sizes %s\n", __LINE__, filePosition);

    // 2. Allocate memory to hold the raw sample sizes.
    if (nullptr != raw_sample_sizes)
    {
        free(raw_sample_sizes);
        raw_sample_sizes = nullptr;
    }
    
    size_t raw_sample_sizes_size = sizeof(uint16_t) * instrument_data.sample_count;
    Log.verbose("Line %d raw_sample_sizes %d\n", __LINE__, raw_sample_sizes_size);
    if (nullptr == (raw_sample_sizes = (uint16_t*)malloc(raw_sample_sizes_size)))
    {
        Log.verbose("Line %d Failed to allocate memory for raw_sample_sizes\n", __LINE__);
        data.close();
        return nullptr;
    }
    snprintf(tmp_buffer,128, "%lx",  (long unsigned int)raw_sample_sizes);
    Log.verbose("Line %d raw_sample_sizes data %s\n", __LINE__, tmp_buffer);

    // 3. Read the raw sample sizes.
    if (data.read(raw_sample_sizes, raw_sample_sizes_size) != raw_sample_sizes_size)
    {
        Log.verbose("Line %d Failed to read raw sample sizes\n", __LINE__);
        data.close();
        return nullptr;
    }

    updateFilePosition(&data);
    Log.verbose("Line %d file position before reading sample note ranges %s\n", __LINE__, filePosition);

    // 4. Allocate memory to hold the per sample note ranges.
    size_t sample_note_ranges_array_size = sizeof(uint8_t) * instrument_data.sample_count;
    Log.verbose("Line %d sample_note_ranges_array_size %d\n", __LINE__, sample_note_ranges_array_size);
    if (nullptr != sample_note_ranges_array)
    {
        free(sample_note_ranges_array);
        sample_note_ranges_array = nullptr;
    }

    if (nullptr == (sample_note_ranges_array = (uint8_t*)malloc(sample_note_ranges_array_size)))
    {
        Log.verbose("Line %d Failed to allocate memory for sample_note_ranges_array\n", __LINE__);
        data.close();
        return nullptr;
    }
    snprintf(tmp_buffer,128, "%lx",  (long unsigned int)sample_note_ranges_array);
    Log.verbose("Line %d sample_note_ranges_array address %s\n", __LINE__, tmp_buffer);
    instrument_data.sample_note_ranges = sample_note_ranges_array;

    // 5. Read the sample_note_ranges.
    if (data.read(sample_note_ranges_array, sample_note_ranges_array_size) != sample_note_ranges_array_size)
    {
        Log.verbose("Line %d Failed to read sample sample note ranges\n", __LINE__);
        data.close();
        return nullptr;
    }
    // 6. Allocate memory for the sample metadata.
    size_t samples_metadata_array_size = ((sizeof(AudioSynthWavetable::sample_data)) * instrument_data.sample_count);
    if (nullptr != samples_metadata_array)
    {
        free(samples_metadata_array);
        samples_metadata_array = nullptr;
    }

    if (nullptr == (samples_metadata_array = (AudioSynthWavetable::sample_data*)malloc(samples_metadata_array_size)))
    {
        Log.verbose("Line %d Failed to allocate memory for samples_metadata_array\n", __LINE__);
        free(sample_note_ranges_array);
        data.close();
        return nullptr;
    }
    snprintf(tmp_buffer,128, "%lx",  (long unsigned int)samples_metadata_array);
    Log.verbose("Line %d samples_metadata_array address %s\n", __LINE__, tmp_buffer);
    instrument_data.samples = samples_metadata_array;

    updateFilePosition(&data);
    Log.verbose("Line %d file position  before reading sample metadata array %s\n", __LINE__, filePosition);

    // 7. Read the sample metadata.
    if (data.read(samples_metadata_array, samples_metadata_array_size) != samples_metadata_array_size)
    {
        Log.verbose("Line %d Failed to read samples metadata\n", __LINE__);
        data.close();
        free(sample_note_ranges_array);
        free(samples_metadata_array);
        return nullptr;
    }

    // 8. For each sample metadata allocate memory for the raw sample data, fix the
    // pointer to the raw sample data in the metadata and
    // read the raw sample data into it.
    for (int i = 0; i < instrument_data.sample_count; i++)
    {
        dumpSampleMetadata(&samples_metadata_array[i]);
        dumpHexBytes(reinterpret_cast<uint8_t*>(samples_metadata_array + i), sizeof(AudioSynthWavetable::sample_data));
        snprintf(tmp_buffer,128, "%lx",  (long unsigned int)samples_metadata_array[i].sample);
        Log.verbose("Line %d samples_metadata_array[%d].sample before fixup %s\n", __LINE__, i, tmp_buffer);
        Log.verbose("Line %d samples_metadata_array[%d].number_of_raw_samples %d\n", __LINE__, i, raw_sample_sizes[i]);
        // Allocate memory for the raw samples array and fix the pointer
        int16_t raw_sample_size = raw_sample_sizes[i];
        int16_t raw_sample_size_in_bytes = raw_sample_size * sizeof(uint32_t);
        if (nullptr == (samples_metadata_array[i].sample = (int16_t*)malloc(raw_sample_size_in_bytes)))
        {
            Log.verbose("Line %d Failed to allocate memory for raw samples array\n", __LINE__);
            free(sample_note_ranges_array);
            free(samples_metadata_array);
            for (int j = i -1; j >= 0; j--)
                free(const_cast<int16_t*>(samples_metadata_array[j].sample));
            data.close();
            return nullptr;
        }
        updateFilePosition(&data);
        Log.verbose("Line %d file position before reading raw sample data %s\n", __LINE__, filePosition);
        Log.verbose("Line %d sample %d metadata\n", __LINE__, i);
        snprintf(tmp_buffer,128, "%lx",  (long unsigned int)samples_metadata_array[i].sample);
        Log.verbose("Line %d samples_metadata_array[%d].sample after fixup %s\n", __LINE__, i, tmp_buffer);
        Log.verbose("Line %d samples_metadata_array[%d].number_of_raw_samples %d\n", __LINE__, i, raw_sample_sizes[i]);
        // Read the raw sample data into the array
        if (data.read((void*)samples_metadata_array[i].sample, raw_sample_size_in_bytes) != (unsigned int)raw_sample_size_in_bytes)
        {
            Log.verbose("Line %d Failed to read raw sample data\n", __LINE__);
            data.close();
            
            free(sample_note_ranges_array);
            free(samples_metadata_array);
            return nullptr;
        }
        Log.verbose("Line %d sample %d data\n", __LINE__, i);
        dumpHexBytes((uint32_t*)samples_metadata_array[i].sample, raw_sample_size);
    }
    data.close();

    dumpInstrumentData(reinterpret_cast<AudioSynthWavetable::instrument_data*>(&instrument_data));
    
    return reinterpret_cast<AudioSynthWavetable::instrument_data*>(&instrument_data);   
}
