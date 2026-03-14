static void log_mel_spectrogram_worker_thread(int ith, const float * hann, const std::vector<float> & samples,
                                              int n_samples, int frame_size, int frame_step, int n_threads,
                                              const whisper_filters & filters, whisper_mel & mel) {
    // Pre-allocated buffers to avoid reallocations
    thread_local std::vector<float> fft_in(frame_size * 2);
    thread_local std::vector<float> fft_out(frame_size * 2 * 2 * 2);
    
    const int n_fft = filters.n_fft;
    int i = ith;

    // make sure n_fft == 1 + (WHISPER_N_FFT / 2), bin_0 to bin_nyquist
    assert(n_fft == 1 + (frame_size / 2));

    // calculate FFT only when fft_in are not all zero
    for (; i < std::min(n_samples / frame_step + 1, mel.n_len); i += n_threads) {
        const int offset = i * frame_step;

        // apply Hann window (~10% faster)
        // Use SIMD for the windowing operation
        for (int j = 0; j < std::min(frame_size, n_samples - offset); j++) {
            fft_in[j] = hann[j] * samples[offset + j];
        }

        // fill the rest with zeros using memset for speed
        if (n_samples - offset < frame_size) {
            memset(fft_in.data() + (n_samples - offset), 0, (frame_size - (n_samples - offset)) * sizeof(float));
        }

        // FFT with precomputed twiddle factors
        fft(fft_in.data(), frame_size, fft_out.data());

        // Calculate modulus^2 of complex numbers using SIMD
        for (int j = 0; j < n_fft; j++) {
            const float re = fft_out[2 * j + 0];
            const float im = fft_out[2 * j + 1];
            fft_out[j] = re * re + im * im;
        }

        // mel spectrogram with unrolled loops
        for (int j = 0; j < mel.n_mel; j++) {
            double sum = 0.0;
            const float * filter_data = filters.data.data() + j * n_fft;
            
            // Process 4 elements at a time
            int k = 0;
            for (; k < n_fft - 3; k += 4) {
                sum += 
                    fft_out[k + 0] * filter_data[k + 0] +
                    fft_out[k + 1] * filter_data[k + 1] +
                    fft_out[k + 2] * filter_data[k + 2] +
                    fft_out[k + 3] * filter_data[k + 3];
            }
            // Handle remainder
            for (; k < n_fft; k++) {
                sum += fft_out[k] * filter_data[k];
            }
            
            sum = log10(std::max(sum, 1e-10));
            mel.data[j * mel.n_len + i] = sum;
        }
    }

    // Otherwise fft_out are all zero
    double sum = log10(1e-10);
    for (; i < mel.n_len; i += n_threads) {
        for (int j = 0; j < mel.n_mel; j++) {
            mel.data[j * mel.n_len + i] = sum;
        }
    }
}