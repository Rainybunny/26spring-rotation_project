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
    const size_t padded_size = n_samples + stage_1_pad + stage_2_pad * 2;

    // Optimized padding initialization
    std::vector<float> samples_padded(padded_size, 0.0f);
    
    // Copy main samples with offset
    memcpy(samples_padded.data() + stage_2_pad, samples, n_samples * sizeof(float));

    // Reflective pad beginning
    for (int i = 0; i < stage_2_pad; ++i) {
        samples_padded[stage_2_pad - 1 - i] = samples[i + 1];
    }

    mel.n_mel = n_mel;
    mel.n_len = (padded_size - frame_size) / frame_step;
    mel.n_len_org = 1 + (n_samples + stage_2_pad - frame_size) / frame_step;
    mel.data.resize(mel.n_mel * mel.n_len);

    // Threaded processing (unchanged)
    {
        std::vector<std::thread> workers(n_threads - 1);
        for (int iw = 0; iw < n_threads - 1; ++iw) {
            workers[iw] = std::thread(
                    log_mel_spectrogram_worker_thread, iw + 1, hann, samples_padded,
                    n_samples + stage_2_pad, frame_size, frame_step, n_threads,
                    std::cref(filters), std::ref(mel));
        }

        log_mel_spectrogram_worker_thread(0, hann, samples_padded, n_samples + stage_2_pad, frame_size, frame_step, n_threads, filters, mel);

        for (int iw = 0; iw < n_threads - 1; ++iw) {
            workers[iw].join();
        }
    }

    // Normalization with early max reduction
    float mmax = -1e20f;
    const size_t data_size = mel.n_mel * mel.n_len;
    for (size_t i = 0; i < data_size; i++) {
        mmax = std::max(mmax, mel.data[i]);
    }

    const float norm_scale = mmax - 8.0f;
    for (size_t i = 0; i < data_size; i++) {
        mel.data[i] = (std::max(mel.data[i], norm_scale) + 4.0f) * 0.25f;
    }

    wstate.t_mel_us += ggml_time_us() - t_start_us;

    if (debug) {
        std::ofstream outFile("log_mel_spectrogram.json");
        outFile << "[";
        for (size_t i = 0; i < data_size - 1; i++) {
            outFile << mel.data[i] << ", ";
        }
        outFile << mel.data.back() << "]";
        outFile.close();
    }

    return true;
}