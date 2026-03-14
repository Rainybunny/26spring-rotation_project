void keyboard_task(void) {
    static matrix_row_t matrix_prev[MATRIX_ROWS];
    static uint8_t      led_status    = 0;
#ifdef QMK_KEYS_PER_SCAN
    uint8_t keys_processed = 0;
#endif

#if defined(OLED_DRIVER_ENABLE) && !defined(OLED_DISABLE_TIMEOUT)
    uint8_t ret = matrix_scan();
#else
    matrix_scan();
#endif

    if (is_keyboard_master()) {
        for (uint8_t r = 0; r < MATRIX_ROWS; r++) {
            matrix_row_t matrix_row = matrix_get_row(r);
            matrix_row_t matrix_change = matrix_row ^ matrix_prev[r];
            
            if (matrix_change) {
#ifdef MATRIX_HAS_GHOST
                if (has_ghost_in_row(r, matrix_row)) {
                    continue;
                }
#endif
                if (debug_matrix) matrix_print();
                
                matrix_row_t changed_bits = matrix_change;
                while (changed_bits) {
                    uint8_t c = __builtin_ctz(changed_bits); // Count trailing zeros to find first set bit
                    changed_bits &= changed_bits - 1; // Clear the least significant bit set
                    
                    action_exec((keyevent_t){
                        .key = (keypos_t){.row = r, .col = c}, 
                        .pressed = (matrix_row & ((matrix_row_t)1 << c)), 
                        .time = (timer_read() | 1)
                    });
                    
                    matrix_prev[r] ^= ((matrix_row_t)1 << c);
#ifdef QMK_KEYS_PER_SCAN
                    if (++keys_processed >= QMK_KEYS_PER_SCAN)
#endif
                        goto MATRIX_LOOP_END;
                }
            }
        }
    }

#ifdef QMK_KEYS_PER_SCAN
    if (!keys_processed)
#endif
        action_exec(TICK);

MATRIX_LOOP_END:

#ifdef DEBUG_MATRIX_SCAN_RATE
    matrix_scan_perf_task();
#endif

#ifdef QWIIC_ENABLE
    qwiic_task();
#endif

#ifdef OLED_DRIVER_ENABLE
    oled_task();
#    ifndef OLED_DISABLE_TIMEOUT
    if (ret) oled_on();
#    endif
#endif

#ifdef MOUSEKEY_ENABLE
    mousekey_task();
#endif

#ifdef PS2_MOUSE_ENABLE
    ps2_mouse_task();
#endif

#ifdef SERIAL_MOUSE_ENABLE
    serial_mouse_task();
#endif

#ifdef ADB_MOUSE_ENABLE
    adb_mouse_task();
#endif

#ifdef SERIAL_LINK_ENABLE
    serial_link_update();
#endif

#ifdef VISUALIZER_ENABLE
    visualizer_update(default_layer_state, layer_state, visualizer_get_mods(), host_keyboard_leds());
#endif

#ifdef POINTING_DEVICE_ENABLE
    pointing_device_task();
#endif

#ifdef MIDI_ENABLE
    midi_task();
#endif

#ifdef VELOCIKEY_ENABLE
    if (velocikey_enabled()) {
        velocikey_decelerate();
    }
#endif

    if (led_status != host_keyboard_leds()) {
        led_status = host_keyboard_leds();
        keyboard_set_leds(led_status);
    }
}