static bool log_mel_spectrogram(
              whisper_state & wstate,
              const float * samples,
              const int   n_samples,
              const int   /*sample_rate*/,
              const int   frame_size,
              const int   frame_step,
              const int   n_mel,
              const int   n_threads,
              const whisper_filters & filters,
              const bool   debug,
              whisper_mel & mel) {
    const int64_t t_start_us = ggml_time_us();

    // Hann window
    WHISPER_ASSERT(frame_size == WHISPER_N_FFT && "Unsupported frame_size");
    const float * hann = global_cache.hann_window;

    // Calculate padding sizes
    const int64_t stage_1_pad = WHISPER_SAMPLE_RATE * 30;
    const int64_t stage_2_pad = frame_size / 2;
    const int64_t total_pad = stage_1_pad + 2 * stage_2_pad;
    
    // Pre-allocate and pad samples
    std::vector<float> samples_padded;
    samples_padded.resize(n_samples + total_pad);
    
    // Reflective pad beginning
    for (int64_t i = 0; i < stage_2_pad; ++i) {
        samples_padded[i] = samples[stage_2_pad - i];
    }
    
    // Copy main samples
    memcpy(samples_padded.data() + stage_2_pad, samples, n_samples * sizeof(float));
    
    // Pad end with zeros
    memset(samples_padded.data() + n_samples + stage_2_pad, 0, (stage_1_pad + stage_2_pad) * sizeof(float));

    // Calculate dimensions
    mel.n_mel = n_mel;
    mel.n_len = (samples_padded.size() - frame_size) / frame_step;
    mel.n_len_org = 1 + (n_samples + stage_2_pad - frame_size) / frame_step;
    
    // Pre-allocate output
    mel.data.resize(mel.n_mel * mel.n_len);

    // Process in parallel
    {
        std::vector<std::thread> workers;
        workers.reserve(n_threads - 1);
        
        for (int iw = 1; iw < n_threads; ++iw) {
            workers.emplace_back(
                log_mel_spectrogram_worker_thread, 
                iw, hann, samples_padded,
                n_samples + stage_2_pad, frame_size, frame_step, n_threads,
                std::cref(filters), std::ref(mel));
        }

        // Main thread work
        log_mel_spectrogram_worker_thread(0, hann, samples_padded, 
            n_samples + stage_2_pad, frame_size, frame_step, n_threads, 
            filters, mel);

        for (auto& worker : workers) {
            worker.join();
        }
    }

    // Normalize - use direct pointer access and unroll loop
    float max_val = -1e20f;
    float* mel_data = mel.data.data();
    const size_t mel_size = mel.n_mel * mel.n_len;
    
    // Find max
    for (size_t i = 0; i < mel_size; ++i) {
        max_val = std::max(max_val, mel_data[i]);
    }
    
    const float norm_shift = max_val - 8.0f;
    
    // Normalize all values
    for (size_t i = 0; i < mel_size; ++i) {
        float val = mel_data[i];
        val = val < norm_shift ? norm_shift : val;
        mel_data[i] = (val + 4.0f) * 0.25f;
    }

    wstate.t_mel_us += ggml_time_us() - t_start_us;

    // Debug output
    if (debug) {
        std::ofstream outFile("log_mel_spectrogram.json");
        outFile << "[";
        for (size_t i = 0; i < mel_size - 1; ++i) {
            outFile << mel_data[i] << ", ";
        }
        outFile << mel_data[mel_size - 1] << "]";
    }

    return true;
}