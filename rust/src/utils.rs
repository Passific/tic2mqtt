pub const MQTT_TOPIC_BASE: &str = "homeassistant/sensor/tic2mqtt/";

pub fn sanitize_ascii_printable(val: &str) -> String {
    val.chars().filter(|&c| (c as u32) >= 32 && (c as u32) <= 126).collect()
}

pub fn sanitize_label(label: &str) -> String {
    label
        .chars()
        .map(|c| if c.is_ascii_alphanumeric() || c == '_' || c == '-' { c } else { '_' })
        .collect()
}

pub fn sanitize_value(value: &str) -> String {
    sanitize_ascii_printable(value)
}

/// Parse label and value from a line split by whitespace
pub fn parse_label_value(line: &str) -> Option<(String, String)> {
    let mut parts = line.split_whitespace();
    let label = parts.next()?.to_string();
    let value = parts.next()?.to_string();
    Some((label, value))
}
