#include <xmmintrin.h>

static void calc_volume_levels(struct obs_source *source, float *array,
		size_t frames, float volume)
{
	float sum_val = 0.0f;
	float max_val = 0.0f;
	float rms_val = 0.0f;

	audio_t        *audio          = obs_get_audio();
	const uint32_t sample_rate    = audio_output_get_sample_rate(audio);
	const size_t   channels       = audio_output_get_channels(audio);
	const size_t   count          = frames * channels;
	const size_t   vol_peak_delay = sample_rate * 3;
	const float    alpha          = 0.15f;

	// Vectorized processing
	size_t i = 0;
	__m128 sum_vec = _mm_setzero_ps();
	__m128 max_vec = _mm_setzero_ps();
	
	// Process 4 samples at a time
	for (; i + 3 < count; i += 4) {
		__m128 val = _mm_loadu_ps(&array[i]);
		__m128 val_pow2 = _mm_mul_ps(val, val);
		
		sum_vec = _mm_add_ps(sum_vec, val_pow2);
		max_vec = _mm_max_ps(max_vec, val_pow2);
	}
	
	// Horizontal sum and max
	float sum_arr[4], max_arr[4];
	_mm_storeu_ps(sum_arr, sum_vec);
	_mm_storeu_ps(max_arr, max_vec);
	
	sum_val = sum_arr[0] + sum_arr[1] + sum_arr[2] + sum_arr[3];
	max_val = fmaxf(fmaxf(max_arr[0], max_arr[1]), 
	               fmaxf(max_arr[2], max_arr[3]));
	
	// Process remaining samples
	for (; i < count; i++) {
		float val = array[i];
		float val_pow2 = val * val;
		
		sum_val += val_pow2;
		max_val = fmaxf(max_val, val_pow2);
	}

	UNUSED_PARAMETER(volume);

	rms_val = to_db(sqrtf(sum_val / (float)count));
	max_val = to_db(sqrtf(max_val));

	if (max_val > source->vol_max)
		source->vol_max = max_val;
	else
		source->vol_max = alpha * source->vol_max +
			(1.0f - alpha) * max_val;

	if (source->vol_max > source->vol_peak ||
	    source->vol_update_count > vol_peak_delay) {
		source->vol_peak         = source->vol_max;
		source->vol_update_count = 0;
	} else {
		source->vol_update_count += count;
	}

	source->vol_mag = alpha * rms_val + source->vol_mag * (1.0f - alpha);
}