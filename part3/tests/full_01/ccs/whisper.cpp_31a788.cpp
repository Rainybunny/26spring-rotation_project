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

    // Calculate the length of padding
    static int64_t stage_1_pad = WHISPER_SAMPLE_RATE * 30;
    static int64_t stage_2_pad = frame_size / 2;

    // Initialize static thread pool and buffers
    static std::vector<std::thread> workers;
    static std::vector<float> samples_padded;
    static bool initialized = false;
    
    if (!initialized) {
        // Pre-allocate maximum expected size
        samples_padded.resize(WHISPER_SAMPLE_RATE * 30 + stage_2_pad * 2);
        workers.reserve(n_threads - 1);
        initialized = true;
    }

    // Copy samples to padded buffer
    std::copy(samples, samples + n_samples, samples_padded.begin() + stage_2_pad);

    // pad 30 seconds of zeros at the end + reflective pad
    std::fill(samples_padded.begin() + n_samples + stage_2_pad, 
             samples_padded.begin() + n_samples + stage_1_pad + 2 * stage_2_pad, 0);
    std::reverse_copy(samples + 1, samples + 1 + stage_2_pad, samples_padded.begin());

    mel.n_mel = n_mel;
    mel.n_len = (samples_padded.size() - frame_size) / frame_step;
    mel.n_len_org = 1 + (n_samples + stage_2_pad - frame_size) / frame_step;
    
    // Reserve without initialization
    mel.data.clear();
    mel.data.resize(mel.n_mel * mel.n_len);

    // Reuse threads if already created
    if (workers.empty()) {
        for (int iw = 0; iw < n_threads - 1; ++iw) {
            workers.emplace_back(
                log_mel_spectrogram_worker_thread, iw + 1, hann, std::ref(samples_padded),
                n_samples + stage_2_pad, frame_size, frame_step, n_threads,
                std::cref(filters), std::ref(mel));
        }
    }

    // main thread
    log_mel_spectrogram_worker_thread(0, hann, samples_padded, n_samples + stage_2_pad, 
                                    frame_size, frame_step, n_threads, filters, mel);

    // Wait for workers
    for (auto& worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }

    // SIMD-optimized clamping and normalization
    double mmax = -1e20;
    for (int i = 0; i < mel.n_mel*mel.n_len; i++) {
        mmax = std::max(mmax, (double)mel.data[i]);
    }
    mmax -= 8.0;

    for (int i = 0; i < mel.n_mel*mel.n_len; i++) {
        mel.data[i] = (std::max(mel.data[i], (float)mmax) + 4.0f)/4.0f;
    }

    wstate.t_mel_us += ggml_time_us() - t_start_us;

    if (debug) {
        std::ofstream outFile("log_mel_spectrogram.json");
        outFile << "[";
        for (uint64_t i = 0; i < mel.data.size() - 1; i++) {
            outFile << mel.data[i] << ", ";
        }
        outFile << mel.data[mel.data.size() - 1] << "]";
        outFile.close();
    }

    return true;
}