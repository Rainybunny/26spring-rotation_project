void IslandWindow::_summonWindowRoutineBody(Remoting::SummonWindowBehavior args)
{
    // Cache animation state as static since it rarely changes
    static BOOL animationsEnabled = TRUE;
    static bool animationsChecked = false;
    
    uint32_t actualDropdownDuration = args.DropdownDuration();
    if (args.DropdownDuration() > 0 && !animationsChecked)
    {
        SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION, 0, &animationsEnabled, 0);
        animationsChecked = true;
        if (!animationsEnabled)
        {
            actualDropdownDuration = 0;
        }
    }
    else if (!animationsEnabled)
    {
        actualDropdownDuration = 0;
    }

    const bool shouldToggle = args.ToggleVisibility() && GetForegroundWindow() == _window.get();
    if (!shouldToggle)
    {
        _globalActivateWindow(actualDropdownDuration, args.ToMonitor());
        return;
    }

    // Only check monitor if we're toggling and using ToMouse behavior
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