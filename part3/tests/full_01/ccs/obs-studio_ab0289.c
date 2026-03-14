static void calc_volume_levels(struct obs_source *source, float *array,
		size_t frames, float volume)
{
	float sum_val = 0.0f;
	float max_val = 0.0f;
	float rms_val;
	
	audio_t *audio = obs_get_audio();
	const uint32_t sample_rate = audio_output_get_sample_rate(audio);
	const size_t channels = audio_output_get_channels(audio);
	const size_t count = frames * channels;
	const size_t vol_peak_delay = sample_rate * 3;
	const float alpha = 0.15f;
	const float inv_alpha = 1.0f - alpha;
	
	// Cache frequently accessed struct members
	float vol_max = source->vol_max;
	float vol_mag = source->vol_mag;
	size_t vol_update_count = source->vol_update_count;
	
	// Calculate sum and max
	for (size_t i = 0; i < count; i++) {
		float val = array[i];
		float val_pow2 = val * val;
		
		sum_val += val_pow2;
		if (val_pow2 > max_val)
			max_val = val_pow2;
	}

	UNUSED_PARAMETER(volume);

	// Convert to dB outside loop
	rms_val = to_db(sqrtf(sum_val / (float)count));
	max_val = to_db(sqrtf(max_val));

	// Update volume max with alpha blending
	vol_max = (max_val > vol_max) ? max_val : 
		(alpha * vol_max + inv_alpha * max_val);

	// Update peak and count
	if (vol_max > source->vol_peak || vol_update_count > vol_peak_delay) {
		source->vol_peak = vol_max;
		vol_update_count = 0;
	} else {
		vol_update_count += count;
	}

	// Update struct members
	source->vol_max = vol_max;
	source->vol_mag = alpha * rms_val + vol_mag * inv_alpha;
	source->vol_update_count = vol_update_count;
}