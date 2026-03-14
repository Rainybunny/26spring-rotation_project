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

    // Initialize a vector and copy data from C array to it.
    std::vector<float> samples_padded;
    samples_padded.resize(n_samples + stage_1_pad + stage_2_pad * 2);
    std::copy(samples, samples + n_samples, samples_padded.begin() + stage_2_pad);

    // pad 30 seconds of zeros at the end of audio (480,000 samples) + reflective pad 200 samples at the end of audio
    std::fill(samples_padded.begin() + n_samples + stage_2_pad, samples_padded.begin() + n_samples + stage_1_pad + 2 * stage_2_pad, 0);

    // reflective pad 200 samples at the beginning of audio
    std::reverse_copy(samples + 1, samples + 1 + stage_2_pad, samples_padded.begin());

    mel.n_mel     = n_mel;
    mel.n_len     = (samples_padded.size() - frame_size) / frame_step;
    mel.n_len_org = 1 + (n_samples + stage_2_pad - frame_size) / frame_step;
    mel.data.resize(mel.n_mel * mel.n_len);

    {
        std::vector<std::thread> workers(n_threads - 1);
        for (int iw = 0; iw < n_threads - 1; ++iw) {
            workers[iw] = std::thread(
                    log_mel_spectrogram_worker_thread, iw + 1, hann, samples_padded,
                    n_samples + stage_2_pad, frame_size, frame_step, n_threads,
                    std::cref(filters), std::ref(mel));
        }

        // main thread
        log_mel_spectrogram_worker_thread(0, hann, samples_padded, n_samples + stage_2_pad, frame_size, frame_step, n_threads, filters, mel);

        for (int iw = 0; iw < n_threads - 1; ++iw) {
            workers[iw].join();
        }
    }

    // Parallel clamping and normalization
    const double mmax = [&]() {
        double max_val = -1e20;
        const int chunk_size = (mel.n_mel * mel.n_len + n_threads - 1) / n_threads;
        
        std::vector<double> thread_max(n_threads, -1e20);
        std::vector<std::thread> workers;
        
        for (int i = 0; i < n_threads; ++i) {
            workers.emplace_back([&, i] {
                const int start = i * chunk_size;
                const int end = std::min(start + chunk_size, mel.n_mel * mel.n_len);
                double& local_max = thread_max[i];
                
                for (int j = start; j < end; ++j) {
                    if (mel.data[j] > local_max) {
                        local_max = mel.data[j];
                    }
                }
            });
        }
        
        for (auto& worker : workers) {
            worker.join();
        }
        
        for (const auto val : thread_max) {
            if (val > max_val) {
                max_val = val;
            }
        }
        return max_val - 8.0;
    }();

    // Parallel normalization
    {
        const int chunk_size = (mel.n_mel * mel.n_len + n_threads - 1) / n_threads;
        std::vector<std::thread> workers;
        
        for (int i = 0; i < n_threads; ++i) {
            workers.emplace_back([&, i] {
                const int start = i * chunk_size;
                const int end = std::min(start + chunk_size, mel.n_mel * mel.n_len);
                
                for (int j = start; j < end; ++j) {
                    mel.data[j] = (std::max(mel.data[j], mmax) + 4.0) / 4.0;
                }
            });
        }
        
        for (auto& worker : workers) {
            worker.join();
        }
    }

    wstate.t_mel_us += ggml_time_us() - t_start_us;

    // Dump log_mel_spectrogram
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