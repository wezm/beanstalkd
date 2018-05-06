use std::time::{SystemTime, UNIX_EPOCH};

/// Returns the number of nanoseconds since UNIX_EPOCH
#[no_mangle]
pub extern fn nanoseconds() -> i64 {
    match SystemTime::now().duration_since(UNIX_EPOCH) {
        Ok(tv) => tv.as_secs() as i64 * 1000000000 + tv.subsec_nanos() as i64,
        Err(_) => -1,
    }
}
