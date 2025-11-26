use crate::tic::TicMode;
use std::collections::{HashSet, HashMap};

pub struct StandardTIC {
    meter_id: String,
    pub label_values: HashMap<String, String>,
}

impl StandardTIC {
    pub const BAUDRATE: u32 = 9600;
    pub fn new() -> Self { StandardTIC { meter_id: String::new(), label_values: HashMap::new() } }
}

impl TicMode for StandardTIC {
    fn baudrate(&self) -> u32 {
        Self::BAUDRATE
    }

    fn labels(&self) -> Vec<String> {
        let labels = vec![
            "ADSC", "CCAIN", "CCAIN-1", "CCASN", "CCASN-1", "DATE", "DPM1", "DPM2", "DPM3", "EAIT",
            "EASD01", "EASD02", "EASD03", "EASD04", "EASF01", "EASF02", "EASF03", "EASF04", "EASF05",
            "EASF06", "EASF07", "EASF08", "EASF09", "EASF10", "EAST", "ERQ1", "ERQ2", "ERQ3", "ERQ4",
            "FPM1", "FPM2", "FPM3", "IRMS1", "IRMS2", "IRMS3", "LTARF", "MSG1", "MSG2", "NGTF", "NJOURF",
            "NJOURF+1", "NTARF", "PCOUP", "PJOURF+1", "PPOINTE", "PREF", "PRM", "RELAIS", "SINSTI",
            "SINSTS", "SINSTS1", "SINSTS2", "SINSTS3", "SMAXIN", "SMAXIN-1", "SMAXSN", "SMAXSN-1",
            "SMAXSN1", "SMAXSN1-1", "SMAXSN2", "SMAXSN2-1", "SMAXSN3", "SMAXSN3-1", "STGE", "UMOY1",
            "UMOY2", "UMOY3", "URMS1", "URMS2", "URMS3", "VTIC",
        ];
        labels.into_iter().map(|s| s.to_string()).collect()
    }

    fn handle_label_value(&mut self, label: &str, value: &str) {
        if label == "ADSC" {
            // Publish previous frame if any (handled in main.rs)
            self.set_meter_id(value);
            self.label_values.clear();
        }
        self.label_values.insert(label.to_string(), value.to_string());
    }

    fn set_meter_id(&mut self, id: &str) {
        let sanitized: String = id.chars().map(|c| if c.is_ascii_alphanumeric() || c == '_' || c == '-' { c } else { '_' }).collect();
        self.meter_id = sanitized;
    }

    fn get_meter_id(&self) -> String { self.meter_id.clone() }

    fn get_ha_device_class(&self, label: &str) -> Option<&'static str> {
        let apparent_power: HashSet<&str> = [
            "SINSTS", "SINSTS1", "SINSTS2", "SINSTS3",
            "SMAXSN", "SMAXSN1", "SMAXSN2", "SMAXSN3", "SMAXSN-1", "SMAXSN1-1", "SMAXSN2-1", "SMAXSN3-1",
            "SINSTI", "SMAXIN", "SMAXIN-1",
        ].into_iter().collect();
        let current: HashSet<&str> = ["IRMS1", "IRMS2", "IRMS3"].into_iter().collect();
        let energy: HashSet<&str> = [
            "EAST", "EASF01", "EASF02", "EASF03", "EASF04", "EASF05", "EASF06", "EASF07", "EASF08", "EASF09", "EASF10", "EASD01", "EASD02", "EASD03", "EASD04", "EAIT"
        ].into_iter().collect();
        let voltage: HashSet<&str> = ["URMS1", "URMS2", "URMS3", "UMOY1", "UMOY2", "UMOY3"].into_iter().collect();
        let power: HashSet<&str> = ["CCASN", "CCASN-1", "CCAIN", "CCAIN-1"].into_iter().collect();
        let reactive_energy: HashSet<&str> = ["ERQ1", "ERQ2", "ERQ3", "ERQ4"].into_iter().collect();

        if apparent_power.contains(label) { return Some("apparent_power"); }
        if current.contains(label) { return Some("current"); }
        if energy.contains(label) { return Some("energy"); }
        if voltage.contains(label) { return Some("voltage"); }
        if power.contains(label) { return Some("power"); }
        if reactive_energy.contains(label) { return Some("reactive_energy"); }
        None
    }

    fn get_ha_state_class(&self, label: &str) -> Option<&'static str> {
        let total_increasing: HashSet<&str> = [
            "EAST", "EASF01", "EASF02", "EASF03", "EASF04", "EASF05", "EASF06", "EASF07", "EASF08", "EASF09", "EASF10", "EASD01", "EASD02", "EASD03", "EASD04", "EAIT"
        ].into_iter().collect();
        let measurement: HashSet<&str> = [
            "IRMS1", "IRMS2", "IRMS3", "URMS1", "URMS2", "URMS3", "UMOY1", "UMOY2", "UMOY3", "SINSTS", "SINSTS1", "SINSTS2", "SINSTS3", "SINSTI"
        ].into_iter().collect();
        if total_increasing.contains(label) { return Some("total_increasing"); }
        if measurement.contains(label) { return Some("measurement"); }
        None
    }

    fn get_ha_unit(&self, label: &str) -> Option<&'static str> {
        let ampere: HashSet<&str> = ["IRMS1", "IRMS2", "IRMS3"].into_iter().collect();
        let watt_hour: HashSet<&str> = [
            "EAST", "EASF01", "EASF02", "EASF03", "EASF04", "EASF05", "EASF06", "EASF07", "EASF08", "EASF09", "EASF10", "EASD01", "EASD02", "EASD03", "EASD04", "EAIT"
        ].into_iter().collect();
        let volt: HashSet<&str> = ["URMS1", "URMS2", "URMS3", "UMOY1", "UMOY2", "UMOY3"].into_iter().collect();
        let volt_ampere: HashSet<&str> = [
            "SINSTS", "SINSTS1", "SINSTS2", "SINSTS3",
            "SMAXSN", "SMAXSN1", "SMAXSN2", "SMAXSN3", "SMAXSN-1", "SMAXSN1-1", "SMAXSN2-1", "SMAXSN3-1",
            "SINSTI", "SMAXIN", "SMAXIN-1"
        ].into_iter().collect();
        let watt: HashSet<&str> = ["CCASN", "CCASN-1", "CCAIN", "CCAIN-1"].into_iter().collect();
        let varh: HashSet<&str> = ["ERQ1", "ERQ2", "ERQ3", "ERQ4"].into_iter().collect();

        if ampere.contains(label) { return Some("A"); }
        if watt_hour.contains(label) { return Some("Wh"); }
        if volt.contains(label) { return Some("V"); }
        if volt_ampere.contains(label) { return Some("VA"); }
        if watt.contains(label) { return Some("W"); }
        if varh.contains(label) { return Some("varh"); }
        None
    }
}
