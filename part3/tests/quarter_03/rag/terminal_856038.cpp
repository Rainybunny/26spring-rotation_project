void IslandWindow::_summonWindowRoutineBody(Remoting::SummonWindowBehavior args)
{
    // Early check for animation settings
    uint32_t actualDropdownDuration = args.DropdownDuration();
    if (actualDropdownDuration > 0)
    {
        BOOL animationsEnabled = TRUE;
        SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION, 0, &animationsEnabled, 0);
        if (!animationsEnabled)
        {
            actualDropdownDuration = 0;
        }
    }

    // Cache foreground window check
    const bool isForeground = (GetForegroundWindow() == _window.get());

    if (args.ToggleVisibility() && isForeground)
    {
        // Check monitor position if needed
        if (args.ToMonitor() == Remoting::MonitorBehavior::ToMouse)
        {
            const til::rectangle cursorMonitorRect{ _getMonitorForCursor().rcMonitor };
            const til::rectangle currentMonitorRect{ _getMonitorForWindow(GetHandle()).rcMonitor };
            if (cursorMonitorRect != currentMonitorRect)
            {
                _globalActivateWindow(actualDropdownDuration, args.ToMonitor());
                return;
            }
        }
        _globalDismissWindow(actualDropdownDuration);
    }
    else
    {
        _globalActivateWindow(actualDropdownDuration, args.ToMonitor());
    }
}