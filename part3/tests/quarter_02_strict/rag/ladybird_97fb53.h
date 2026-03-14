void push_command(PaintingCommand command)
    {
        m_painting_commands.emplace_back(PaintingCommandWithScrollFrame{state().scroll_frame_id, std::move(command)});
    }