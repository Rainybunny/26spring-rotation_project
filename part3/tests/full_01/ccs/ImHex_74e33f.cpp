void Task::update(u64 value) {
        m_currValue = value;

        if (m_shouldInterrupt.load(std::memory_order_relaxed)) [[unlikely]]
            throw TaskInterruptor();
    }