q: `steady_clock` vs `system_clock` for timer
a: `steady_clock` system_clock is a wall-clock that can be adjusted by NTP, DST changes, or settimeofday(). A backward NTP jump can make the timer never fire; a forward jump can fire it prematurely
q: bounded vs unbounded q



