void Shell::bring_cursor_to_beginning_of_a_line() const
{
    // Cache terminal size and EOL mark since they rarely change
    static struct winsize cached_ws = [] {
        struct winsize ws;
        if (ioctl(STDERR_FILENO, TIOCGWINSZ, &ws) < 0) {
            ws.ws_col = 80;
            ws.ws_row = 25;
        }
        return ws;
    }();

    static const String cached_eol_mark = [] {
        constexpr auto default_mark = "\e[30;46m%\e[0m";
        auto mark = getenv("PROMPT_EOL_MARK");
        return mark ? String(mark) : String(default_mark);
    }();

    struct winsize ws = m_editor ? m_editor->terminal_size() : cached_ws;
    String eol_mark = cached_eol_mark;

    size_t eol_mark_length = Line::Editor::actual_rendered_string_metrics(eol_mark).line_metrics.last().total_length();
    if (eol_mark_length >= ws.ws_col) {
        eol_mark = "\e[30;46m%\e[0m";
        eol_mark_length = 1;
    }

    fputs(eol_mark.characters(), stderr);

    // Fill remaining space more efficiently
    if (ws.ws_col > eol_mark_length) {
        String spaces(ws.ws_col - eol_mark_length, ' ');
        fputs(spaces.characters(), stderr);
    }

    putc('\r', stderr);
}