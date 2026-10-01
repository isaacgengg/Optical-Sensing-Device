# Recorded sensor data

Nothing here yet. Once real hardware exists, put recorded sensor readings here
as CSV (one row per reading, one column per channel, plus a header naming the
strip, concentration, and conditions).

Tests can load these directly, or a small generator script can turn them into
`channel_reading_t` tables to replace the made-up values in
`firmware/adapters/mock_sensor/mock_data.c`.
