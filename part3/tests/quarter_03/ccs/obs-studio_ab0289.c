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
	const float    inv_count      = 1.0f / (float)count;

	// Vectorized processing of sum and max calculations
	#pragma omp simd reduction(+:sum_val) reduction(max:max_val)
	for (size_t i = 0; i < count; i++) {
		float val      = array[i];
		float val_pow2 = val * val;

		sum_val += val_pow2;
		max_val  = fmaxf(max_val, val_pow2);
	}

	/*
	  We want the volume meters scale linearly in respect to current
	  volume, so, no need to apply volume here.
	*/

	UNUSED_PARAMETER(volume);

	// Optimized RMS and max calculations
	rms_val = to_db(sqrtf(sum_val * inv_count));
	max_val = to_db(sqrtf(max_val));

	// Optimized alpha blending
	if (max_val > source->vol_max) {
		source->vol_max = max_val;
	} else {
		source->vol_max = source->vol_max + alpha * (max_val - source->vol_max);
	}

	if (source->vol_max > source->vol_peak ||
	    source->vol_update_count > vol_peak_delay) {
		source->vol_peak         = source->vol_max;
		source->vol_update_count = 0;
	} else {
		source->vol_update_count += count;
	}

	// Optimized alpha blending for vol_mag
	source->vol_mag += alpha * (rms_val - source->vol_mag);
}