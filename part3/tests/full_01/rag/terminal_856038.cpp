void IslandWindow::_summonWindowRoutineBody(Remoting::SummonWindowBehavior args)
{
    uint32_t actualDropdownDuration = args.DropdownDuration();
    // If the user requested an animation, let's check if animations are enabled in the OS.
    if (args.DropdownDuration() > 0)
    {
        BOOL animationsEnabled = TRUE;
        SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION, 0, &animationsEnabled, 0);
        if (!animationsEnabled)
        {
            actualDropdownDuration = 0;
        }
    }

    // Cache foreground window check since it won't change during this function
    const bool isForeground = GetForegroundWindow() == _window.get();

    // * If the user doesn't want to toggleVisibility, then just always try to
    //   activate.
    // * If the user does want to toggleVisibility,
    //   - If we're the foreground window, ToMonitor == ToMouse, and the mouse is on the monitor we are
    //      - activate the window
    //   - else
    //      - dismiss the window
    if (args.ToggleVisibility() && isForeground)
    {
        bool handled = false;

        // They want to toggle the window when it is the FG window, and we are
        // the FG window. However, if we're on a different monitor than the
        // mouse, then we should move to that monitor instead of dismissing.
        if (args.ToMonitor() == Remoting::MonitorBehavior::ToMouse)
        {
            const til::rectangle cursorMonitorRect{ _getMonitorForCursor().rcMonitor };
            const til::rectangle currentMonitorRect{ _getMonitorForWindow(GetHandle()).rcMonitor };
            if (cursorMonitorRect != currentMonitorRect)
            {
                _globalActivateWindow(actualDropdownDuration, args.ToMonitor());
                handled = true;
            }
        }

        if (!handled)
        {
            _globalDismissWindow(actualDropdownDuration);
        }
    }
    else
    {
        _globalActivateWindow(actualDropdownDuration, args.ToMonitor());
    }
}