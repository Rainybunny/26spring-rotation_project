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

    // Initialize padded samples with OpenMP parallelization
    std::vector<float> samples_padded(n_samples + stage_1_pad + stage_2_pad * 2);
    
    // Parallel copy and padding
    #pragma omp parallel for schedule(static) num_threads(n_threads)
    for (int i = 0; i < n_samples + stage_1_pad + 2*stage_2_pad; ++i) {
        if (i < stage_2_pad) {
            // Reflective pad beginning
            samples_padded[i] = samples[stage_2_pad - i];
        } else if (i < n_samples + stage_2_pad) {
            // Main samples
            samples_padded[i] = samples[i - stage_2_pad];
        } else {
            // Zero pad end
            samples_padded[i] = 0.0f;
        }
    }

    mel.n_mel = n_mel;
    mel.n_len = (samples_padded.size() - frame_size) / frame_step;
    mel.n_len_org = 1 + (n_samples + stage_2_pad - frame_size) / frame_step;
    mel.data.resize(mel.n_mel * mel.n_len);

    // Process frames in parallel with OpenMP
    #pragma omp parallel for schedule(dynamic) num_threads(n_threads)
    for (int i = 0; i < mel.n_len; i++) {
        const int offset = i * frame_step;
        
        std::vector<float> fft_in(frame_size * 2, 0.0f);
        std::vector<float> fft_out(frame_size * 2 * 2 * 2);

        // Apply Hann window
        for (int j = 0; j < std::min(frame_size, (int)samples_padded.size() - offset); j++) {
            fft_in[j] = hann[j] * samples_padded[offset + j];
        }

        // FFT
        fft(fft_in.data(), frame_size, fft_out.data());

        // Calculate modulus^2
        const int n_fft = 1 + (frame_size / 2);
        for (int j = 0; j < n_fft; j++) {
            fft_out[j] = (fft_out[2*j + 0] * fft_out[2*j + 0] + fft_out[2*j + 1] * fft_out[2*j + 1]);
        }

        // Mel spectrogram
        for (int j = 0; j < mel.n_mel; j++) {
            double sum = 0.0;
            int k = 0;
            for (k = 0; k < n_fft - 3; k += 4) {
                sum += fft_out[k + 0] * filters.data[j * n_fft + k + 0] +
                       fft_out[k + 1] * filters.data[j * n_fft + k + 1] +
                       fft_out[k + 2] * filters.data[j * n_fft + k + 2] +
                       fft_out[k + 3] * filters.data[j * n_fft + k + 3];
            }
            for (; k < n_fft; k++) {
                sum += fft_out[k] * filters.data[j * n_fft + k];
            }
            sum = log10(std::max(sum, 1e-10));
            mel.data[j * mel.n_len + i] = sum;
        }
    }

    // Normalization (serial as it's not computationally intensive)
    double mmax = -1e20;
    for (int i = 0; i < mel.n_mel*mel.n_len; i++) {
        if (mel.data[i] > mmax) mmax = mel.data[i];
    }
    mmax -= 8.0;

    #pragma omp parallel for schedule(static) num_threads(n_threads)
    for (int i = 0; i < mel.n_mel*mel.n_len; i++) {
        if (mel.data[i] < mmax) mel.data[i] = mmax;
        mel.data[i] = (mel.data[i] + 4.0)/4.0;
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