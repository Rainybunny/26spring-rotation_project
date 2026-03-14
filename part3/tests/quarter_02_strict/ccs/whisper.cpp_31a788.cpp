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
    int64_t stage_1_pad = WHISPER_SAMPLE_RATE * 30;
    int64_t stage_2_pad = frame_size / 2;

    // Use pre-allocated buffer from state
    std::vector<float>& samples_padded = wstate.inp_mel_padded;
    const size_t needed_size = n_samples + stage_1_pad + stage_2_pad * 2;
    if (samples_padded.capacity() < needed_size) {
        samples_padded.reserve(needed_size * 2); // Reserve extra to avoid frequent reallocs
    }
    samples_padded.resize(needed_size);
    
    // Copy data
    std::copy(samples, samples + n_samples, samples_padded.begin() + stage_2_pad);

    // pad 30 seconds of zeros at the end of audio + reflective pad
    std::fill(samples_padded.begin() + n_samples + stage_2_pad, 
             samples_padded.begin() + n_samples + stage_1_pad + 2 * stage_2_pad, 0);
    std::reverse_copy(samples + 1, samples + 1 + stage_2_pad, samples_padded.begin());

    mel.n_mel = n_mel;
    mel.n_len = (samples_padded.size() - frame_size) / frame_step;
    mel.n_len_org = 1 + (n_samples + stage_2_pad - frame_size) / frame_step;
    
    // Pre-allocate mel data buffer if needed
    const size_t mel_data_size = mel.n_mel * mel.n_len;
    if (mel.data.capacity() < mel_data_size) {
        mel.data.reserve(mel_data_size * 2); // Reserve extra
    }
    mel.data.resize(mel_data_size);

    // Rest of the function remains the same...
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

    // clamping and normalization
    double mmax = -1e20;
    for (int i = 0; i < mel_data_size; i++) {
        if (mel.data[i] > mmax) {
            mmax = mel.data[i];
        }
    }

    mmax -= 8.0;

    for (int i = 0; i < mel_data_size; i++) {
        if (mel.data[i] < mmax) {
            mel.data[i] = mmax;
        }
        mel.data[i] = (mel.data[i] + 4.0)/4.0;
    }

    wstate.t_mel_us += ggml_time_us() - t_start_us;

    if (debug) {
        std::ofstream outFile("log_mel_spectrogram.json");
        outFile << "[";
        for (uint64_t i = 0; i < mel.data.size() - 1; i++) {
            outFile << mel.data[i] << ", ";
        }
        outFile << mel.data.back() << "]";
        outFile.close();
    }

    return true;
}