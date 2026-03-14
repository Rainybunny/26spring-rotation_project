void IslandWindow::_summonWindowRoutineBody(Remoting::SummonWindowBehavior args)
{
    uint32_t actualDropdownDuration = args.DropdownDuration();
    // If the user requested an animation, let's check if animations are enabled in the OS.
    if (args.DropdownDuration() > 0) [[unlikely]]
    {
        BOOL animationsEnabled = TRUE;
        SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION, 0, &animationsEnabled, 0);
        if (!animationsEnabled) [[unlikely]]
        {
            // The OS has animations disabled - we should respect that and
            // disable the animation here.
            actualDropdownDuration = 0;
        }
    }

    if (args.ToggleVisibility() && GetForegroundWindow() == _window.get()) [[unlikely]]
    {
        bool handled = false;

        // They want to toggle the window when it is the FG window, and we are
        // the FG window. However, if we're on a different monitor than the
        // mouse, then we should move to that monitor instead of dismissing.
        if (args.ToMonitor() == Remoting::MonitorBehavior::ToMouse) [[unlikely]]
        {
            const til::rectangle cursorMonitorRect{ _getMonitorForCursor().rcMonitor };
            const til::rectangle currentMonitorRect{ _getMonitorForWindow(GetHandle()).rcMonitor };
            if (cursorMonitorRect != currentMonitorRect) [[unlikely]]
            {
                // We're not on the same monitor as the mouse. Go to that monitor.
                _globalActivateWindow(actualDropdownDuration, args.ToMonitor());
                handled = true;
            }
        }

        if (!handled) [[unlikely]]
        {
            _globalDismissWindow(actualDropdownDuration);
        }
    }
    else
    {
        _globalActivateWindow(actualDropdownDuration, args.ToMonitor());
    }
}