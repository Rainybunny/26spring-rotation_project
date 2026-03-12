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

    // Calculate padding lengths
    const int64_t stage_1_pad = WHISPER_SAMPLE_RATE * 30;
    const int64_t stage_2_pad = frame_size / 2;
    const int64_t total_pad = stage_1_pad + 2 * stage_2_pad;

    // Pre-allocate padded samples buffer
    static std::vector<float> samples_padded;
    samples_padded.resize(n_samples + total_pad);

    // Pad samples - optimized copy operations
    // 1. Reflective pad beginning
    std::reverse_copy(samples + 1, samples + 1 + stage_2_pad, samples_padded.begin());
    // 2. Copy main samples
    memcpy(samples_padded.data() + stage_2_pad, samples, n_samples * sizeof(float));
    // 3. Zero pad end
    memset(samples_padded.data() + n_samples + stage_2_pad, 0, (stage_1_pad + stage_2_pad) * sizeof(float));

    // Calculate dimensions
    mel.n_mel = n_mel;
    mel.n_len = (samples_padded.size() - frame_size) / frame_step;
    mel.n_len_org = 1 + (n_samples + stage_2_pad - frame_size) / frame_step;
    
    // Pre-allocate output buffer
    mel.data.resize(mel.n_mel * mel.n_len);

    // Process in parallel
    {
        std::vector<std::thread> workers;
        workers.reserve(n_threads - 1);
        
        for (int iw = 1; iw < n_threads; ++iw) {
            workers.emplace_back(
                log_mel_spectrogram_worker_thread, 
                iw, hann, std::cref(samples_padded),
                n_samples + stage_2_pad, frame_size, frame_step, n_threads,
                std::cref(filters), std::ref(mel));
        }

        // Main thread work
        log_mel_spectrogram_worker_thread(0, hann, samples_padded, 
            n_samples + stage_2_pad, frame_size, frame_step, 
            n_threads, filters, mel);

        // Join workers
        for (auto& worker : workers) {
            worker.join();
        }
    }

    // Normalization
    float mmax = -1e20f;
    for (float val : mel.data) {
        mmax = std::max(mmax, val);
    }
    mmax -= 8.0f;

    for (float& val : mel.data) {
        val = (std::max(val, mmax) + 4.0f) / 4.0f;
    }

    wstate.t_mel_us += ggml_time_us() - t_start_us;

    if (debug) {
        std::ofstream outFile("log_mel_spectrogram.json");
        outFile << "[";
        for (size_t i = 0; i < mel.data.size() - 1; i++) {
            outFile << mel.data[i] << ", ";
        }
        outFile << mel.data.back() << "]";
    }

    return true;
}