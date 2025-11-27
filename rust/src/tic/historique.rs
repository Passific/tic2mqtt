use std::any::Any;
use crate::tic::TicMode;
use std::collections::{HashSet, HashMap};

use crate::tic::LabelValue;
pub struct HistoriqueTIC {
    meter_id: String,
    pub label_values: HashMap<String, LabelValue>,
}

impl HistoriqueTIC {
    pub const BAUDRATE: u32 = 1200;
    pub fn new() -> Self { HistoriqueTIC { meter_id: String::new(), label_values: HashMap::new() } }
}

impl TicMode for HistoriqueTIC {
    fn get_mode_name(&self) -> &'static str { "historique" }
    fn as_any(&self) -> &dyn Any { self }
    fn baudrate(&self) -> u32 {
        Self::BAUDRATE
    }

    fn labels(&self) -> Vec<String> {
        let labels = vec![
            "ADCO", "ADIR1", "ADIR2", "ADIR3", "ADPS", "BASE", "BBRHCJB", "BBRHCJR", "BBRHCJW",
            "BBRHPJB", "BBRHPJR", "BBRHPJW", "DEMAIN", "EJPHN", "EJPHPM", "HCHC", "HCHP", "HHPHC",
            "IINST", "IINST1", "IINST2", "IINST3", "IMAX", "IMAX1", "IMAX2", "IMAX3", "ISOUSC",
            "MOTDETAT", "OPTARIF", "PAPP", "PEJP", "PMAX", "PPOT", "PTEC",
        ];
        labels.into_iter().map(|s| s.to_string()).collect()
    }

    fn handle_label_value(&mut self, label: &str, value: &str) {
        if label == "ADCO" {
            // Publish previous frame if any (handled in main.rs)
            self.set_meter_id(value);
            // Do not clear label_values, just update values in place
        }
        use std::collections::hash_map::Entry;
        match self.label_values.entry(label.to_string()) {
            Entry::Occupied(mut entry) => {
                entry.get_mut().value = value.to_string();
                // Optionally update timestamp here if needed
            },
            Entry::Vacant(entry) => {
                entry.insert(LabelValue { value: value.to_string(), timestamp: None });
            }
        }
    }

    fn set_meter_id(&mut self, id: &str) {
        let sanitized: String = id.chars().map(|c| if c.is_ascii_alphanumeric() || c == '_' || c == '-' { c } else { '_' }).collect();
        self.meter_id = sanitized;
    }

    fn get_meter_id(&self) -> String { self.meter_id.clone() }

    fn get_ha_device_class(&self, label: &str) -> Option<&'static str> {
        let apparent_power: HashSet<&str> = ["PAPP", "PREF", "PCOUP", "SINSTS", "SINSTS1", "SINSTS2", "SINSTS3"].into_iter().collect();
        let current: HashSet<&str> = ["ADIR1", "ADIR2", "ADIR3", "ADPS", "IINST", "IINST1", "IINST2", "IINST3", "IMAX", "IMAX1", "IMAX2", "IMAX3", "ISOUSC"].into_iter().collect();
        let energy: HashSet<&str> = ["BASE", "BBRHCJB", "BBRHCJR", "BBRHCJW", "BBRHPJB", "BBRHPJR", "BBRHPJW", "EJPHN", "EJPHPM", "HCHC", "HCHP"].into_iter().collect();
        if apparent_power.contains(label) { return Some("apparent_power"); }
        if current.contains(label) { return Some("current"); }
        if energy.contains(label) { return Some("energy"); }
        None
    }

    fn get_ha_state_class(&self, label: &str) -> Option<&'static str> {
        let total_increasing: HashSet<&str> = ["BASE", "BBRHCJB", "BBRHCJR", "BBRHCJW", "BBRHPJB", "BBRHPJR", "BBRHPJW", "EJPHN", "EJPHPM", "HCHC", "HCHP"].into_iter().collect();
        let measurement: HashSet<&str> = ["IINST", "IINST1", "IINST2", "IINST3", "PAPP"].into_iter().collect();
        if total_increasing.contains(label) { return Some("total_increasing"); }
        if measurement.contains(label) { return Some("measurement"); }
        None
    }

    fn get_ha_unit(&self, label: &str) -> Option<&'static str> {
        let ampere: HashSet<&str> = ["ADPS", "ADIR1", "ADIR2", "ADIR3", "IINST", "IINST1", "IINST2", "IINST3", "IMAX", "IMAX1", "IMAX2", "IMAX3", "ISOUSC"].into_iter().collect();
        let watt_hour: HashSet<&str> = ["BASE", "BBRHCJB", "BBRHCJR", "BBRHCJW", "BBRHPJB", "BBRHPJR", "BBRHPJW", "EJPHN", "EJPHPM", "HCHC", "HCHP"].into_iter().collect();
        let volt_ampere: HashSet<&str> = ["PAPP", "PREF", "PCOUP"].into_iter().collect();
        let watt: HashSet<&str> = ["PMAX"].into_iter().collect();
        if ampere.contains(label) { return Some("A"); }
        if watt_hour.contains(label) { return Some("Wh"); }
        if volt_ampere.contains(label) { return Some("VA"); }
        if watt.contains(label) { return Some("W"); }
        None
    }
}
