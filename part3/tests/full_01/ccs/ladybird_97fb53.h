void push_command(PaintingCommand command)
    {
        m_painting_commands.append({ state().scroll_frame_id, command });
    }

    RecordingPainter()
    {
        m_state_stack.append(State());
        m_painting_commands.reserve(256); // Pre-allocate capacity for typical command count
    }