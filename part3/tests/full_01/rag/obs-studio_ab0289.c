#include <immintrin.h>

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

	// Process samples in chunks of 8 using AVX
	size_t i = 0;
	if (count >= 8) {
		__m256 sum_vec = _mm256_setzero_ps();
		__m256 max_vec = _mm256_setzero_ps();
		
		for (; i <= count - 8; i += 8) {
			__m256 samples = _mm256_loadu_ps(&array[i]);
			__m256 squared = _mm256_mul_ps(samples, samples);
			
			sum_vec = _mm256_add_ps(sum_vec, squared);
			max_vec = _mm256_max_ps(max_vec, squared);
		}
		
		// Horizontal sum and max
		__m128 sum128 = _mm_add_ps(_mm256_extractf128_ps(sum_vec, 0),
		                          _mm256_extractf128_ps(sum_vec, 1));
		__m128 max128 = _mm_max_ps(_mm256_extractf128_ps(max_vec, 0),
		                          _mm256_extractf128_ps(max_vec, 1));
		
		sum128 = _mm_hadd_ps(sum128, sum128);
		sum128 = _mm_hadd_ps(sum128, sum128);
		sum_val = _mm_cvtss_f32(sum128);
		
		max128 = _mm_max_ps(max128, _mm_movehl_ps(max128, max128));
		max128 = _mm_max_ss(max128, _mm_movehdup_ps(max128));
		max_val = _mm_cvtss_f32(max128);
	}

	// Process remaining samples
	for (; i < count; i++) {
		float val = array[i];
		float val_pow2 = val * val;
		sum_val += val_pow2;
		max_val = fmaxf(max_val, val_pow2);
	}

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