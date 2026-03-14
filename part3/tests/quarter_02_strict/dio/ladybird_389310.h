static size_t copy(T* destination, const T* source, size_t count)
    {
        if (count == 0)
            return 0;

        if constexpr (Traits<T>::is_trivial()) {
            __builtin_memmove(destination, source, count * sizeof(T));
            return count;
        }

        if (destination <= source) {
            for (size_t i = 0; i < count; ++i)
                new (&destination[i]) T(source[i]);
        } else {
            for (size_t i = 0; i < count; ++i) {
                size_t reverse_idx = count - i - 1;
                new (&destination[reverse_idx]) T(source[reverse_idx]);
            }
        }

        return count;
    }