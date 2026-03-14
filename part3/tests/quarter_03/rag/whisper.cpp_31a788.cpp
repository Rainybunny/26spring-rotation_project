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

    // Pre-allocate and pad samples in one go
    std::vector<float> samples_padded(n_samples + total_pad);
    
    // Reflective pad at beginning
    for (int64_t i = 0; i < stage_2_pad; ++i) {
        samples_padded[i] = samples[stage_2_pad - i];
    }
    
    // Copy main samples
    memcpy(samples_padded.data() + stage_2_pad, samples, n_samples * sizeof(float));
    
    // Zero pad at end
    memset(samples_padded.data() + n_samples + stage_2_pad, 0, (stage_1_pad + stage_2_pad) * sizeof(float));

    mel.n_mel = n_mel;
    mel.n_len = (samples_padded.size() - frame_size) / frame_step;
    mel.n_len_org = 1 + (n_samples + stage_2_pad - frame_size) / frame_step;
    mel.data.resize(mel.n_mel * mel.n_len);

    {
        std::vector<std::thread> workers;
        workers.reserve(n_threads - 1);
        for (int iw = 0; iw < n_threads - 1; ++iw) {
            workers.emplace_back(
                log_mel_spectrogram_worker_thread, iw + 1, hann, samples_padded,
                n_samples + stage_2_pad, frame_size, frame_step, n_threads,
                std::cref(filters), std::ref(mel));
        }

        // main thread
        log_mel_spectrogram_worker_thread(0, hann, samples_padded, n_samples + stage_2_pad, frame_size, frame_step, n_threads, filters, mel);

        for (auto& worker : workers) {
            worker.join();
        }
    }

    // Find max value for normalization
    float mmax = -1e20f;
    for (const auto& val : mel.data) {
        mmax = std::max(mmax, val);
    }
    mmax -= 8.0f;

    // Normalize with clamping
    for (auto& val : mel.data) {
        val = (std::max(val, mmax) + 4.0f) / 4.0f;
    }

    wstate.t_mel_us += ggml_time_us() - t_start_us;

    if (debug) {
        std::ofstream outFile("log_mel_spectrogram.json");
        outFile << "[";
        if (!mel.data.empty()) {
            outFile << mel.data[0];
            for (size_t i = 1; i < mel.data.size(); ++i) {
                outFile << ", " << mel.data[i];
            }
        }
        outFile << "]";
    }

    return true;
}