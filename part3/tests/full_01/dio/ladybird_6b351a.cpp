void Shell::bring_cursor_to_beginning_of_a_line() const
{
    // Cache terminal size since it rarely changes
    static struct winsize cached_ws;
    static bool ws_cached = false;
    
    if (!ws_cached || !m_editor) {
        if (m_editor) {
            cached_ws = m_editor->terminal_size();
        } else {
            if (ioctl(STDERR_FILENO, TIOCGWINSZ, &cached_ws) < 0) {
                // Very annoying assumptions.
                cached_ws.ws_col = 80;
                cached_ws.ws_row = 25;
            }
        }
        ws_cached = true;
    }

    // Black with Cyan background.
    constexpr auto default_mark = "\e[30;46m%\e[0m";
    String eol_mark = getenv("PROMPT_EOL_MARK");
    if (eol_mark.is_null())
        eol_mark = default_mark;
    size_t eol_mark_length = Line::Editor::actual_rendered_string_metrics(eol_mark).line_metrics.last().total_length();
    if (eol_mark_length >= cached_ws.ws_col) {
        eol_mark = default_mark;
        eol_mark_length = 1;
    }

    // Batch output operations
    StringBuilder output;
    output.append(eol_mark);
    for (auto i = eol_mark_length; i < cached_ws.ws_col; ++i)
        output.append(' ');
    output.append('\r');
    fputs(output.string_view().characters(), stderr);
}