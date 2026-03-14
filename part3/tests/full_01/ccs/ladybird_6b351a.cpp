void Shell::bring_cursor_to_beginning_of_a_line() const
{
    static String cached_eol_mark;
    static size_t cached_eol_mark_length = 0;
    
    struct winsize ws;
    if (m_editor) {
        ws = m_editor->terminal_size();
    } else {
        if (ioctl(STDERR_FILENO, TIOCGWINSZ, &ws) < 0) {
            // Very annoying assumptions.
            ws.ws_col = 80;
            ws.ws_row = 25;
        }
    }

    // Black with Cyan background.
    constexpr auto default_mark = "\e[30;46m%\e[0m";
    if (cached_eol_mark.is_null()) {
        cached_eol_mark = getenv("PROMPT_EOL_MARK");
        if (cached_eol_mark.is_null())
            cached_eol_mark = default_mark;
        cached_eol_mark_length = Line::Editor::actual_rendered_string_metrics(cached_eol_mark).line_metrics.last().total_length();
        if (cached_eol_mark_length >= ws.ws_col) {
            cached_eol_mark = default_mark;
            cached_eol_mark_length = 1;
        }
    }

    // Precompute the entire output string
    StringBuilder builder;
    builder.append(cached_eol_mark);
    for (auto i = cached_eol_mark_length; i < ws.ws_col; ++i)
        builder.append(' ');
    builder.append("\r");

    // Write everything in one operation
    auto output = builder.build();
    fwrite(output.characters(), 1, output.length(), stderr);
}