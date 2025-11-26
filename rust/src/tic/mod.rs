use std::sync::{Arc, Mutex};
use crate::utils::{MQTT_ID_BASE, MQTT_TOPIC_BASE, sanitize_label};

pub mod standard;
pub mod historique;

#[derive(Clone)]
pub enum TicModeEnum {
    Standard,
    Historique,
}

#[derive(Clone)]
pub struct TicModeHandle {
    inner: Arc<Mutex<Box<dyn TicMode + Send>>>,
}

impl TicModeHandle {
    pub fn baudrate(&self) -> u32 {
        if let Ok(lock) = self.inner.lock() { lock.baudrate() } else { 9600 }
    }
    pub fn new(mode: TicModeEnum) -> Self {
        let boxed: Box<dyn TicMode + Send> = match mode {
            TicModeEnum::Standard => Box::new(standard::StandardTIC::new()),
            TicModeEnum::Historique => Box::new(historique::HistoriqueTIC::new()),
        };
        TicModeHandle { inner: Arc::new(Mutex::new(boxed)) }
    }

    pub fn handle_label_value(&self, label: &str, value: &str) {
        if let Ok(mut lock) = self.inner.lock() { lock.handle_label_value(label, value); }
    }

    pub fn get_meter_id(&self) -> String {
        if let Ok(lock) = self.inner.lock() { lock.get_meter_id() } else { String::new() }
    }

    pub fn get_mqtt_topic(&self, label: &str) -> String {
        if let Ok(lock) = self.inner.lock() { lock.get_mqtt_topic(label) } else { String::new() }
    }

    pub fn get_all_discovery_messages(&self) -> Vec<(String, String)> {
        if let Ok(lock) = self.inner.lock() { lock.get_all_discovery_messages() } else { Vec::new() }
    }
}

pub trait TicMode {
    fn labels(&self) -> Vec<String> { Vec::new() }
    fn handle_label_value(&mut self, _label: &str, _value: &str);
    fn set_meter_id(&mut self, id: &str);
    fn get_meter_id(&self) -> String;
    fn baudrate(&self) -> u32;

    fn get_object_id(&self, label: &str) -> String {
        let safe_label = sanitize_label(label);
        format!("{}_{}", MQTT_ID_BASE, safe_label)
    }

    fn get_mqtt_topic(&self, label: &str) -> String {
        let id = self.get_meter_id();
        if id.is_empty() { return String::new(); }
        format!("{}/{}/{}/state", MQTT_TOPIC_BASE, id, self.get_object_id(label))
    }

    fn get_mqtt_config_topic(&self, label: &str) -> String {
        let id = self.get_meter_id();
        if id.is_empty() { return String::new(); }
        format!("{}/{}/{}/config", MQTT_TOPIC_BASE, id, self.get_object_id(label))
    }

    fn get_ha_device_class(&self, _label: &str) -> Option<&'static str> { None }
    fn get_ha_state_class(&self, _label: &str) -> Option<&'static str> { None }
    fn get_ha_unit(&self, _label: &str) -> Option<&'static str> { None }

    fn get_all_discovery_messages(&self) -> Vec<(String, String)> {
        let mut msgs = Vec::new();
        let meter = self.get_meter_id();
        if meter.is_empty() { return msgs; }
        for label in self.labels() {
            let safe_label = sanitize_label(&label);
            let object_id = self.get_object_id(&label);
            let config_topic = self.get_mqtt_config_topic(&label);
            let state_topic = self.get_mqtt_topic(&label);
            let device_class = self.get_ha_device_class(&label);
            let state_class = self.get_ha_state_class(&label);
            let unit = self.get_ha_unit(&label);

            let mut payload = format!(
                "{{\"name\":\"TIC {}\",\"state_topic\":\"{}\",\"unique_id\":\"{}\"",
                safe_label, state_topic, object_id
            );
            if let Some(dc) = device_class {
                payload.push_str(&format!(",\"device_class\":\"{}\"", dc));
            }
            if let Some(sc) = state_class {
                payload.push_str(&format!(",\"state_class\":\"{}\"", sc));
            }
            if let Some(u) = unit {
                payload.push_str(&format!(",\"unit_of_measurement\":\"{}\"", u));
            }
            payload.push_str("}");
            msgs.push((config_topic, payload));
        }
        msgs
    }
}
