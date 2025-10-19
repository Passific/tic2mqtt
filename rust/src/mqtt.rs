use paho_mqtt as mqtt;
use std::sync::mpsc::Receiver;
use crate::tic::TicModeHandle;

pub struct MqttConfig {
    pub server: String,
    pub client_id: String,
    pub username: String,
    pub password: String,
}

pub struct MqttPublisher {
    config: MqttConfig,
    rx: Receiver<(String, String)>,
    mode: TicModeHandle,
}

impl MqttPublisher {
    pub fn new(config: MqttConfig, rx: Receiver<(String, String)>, mode: TicModeHandle) -> Self {
        MqttPublisher { config, rx, mode }
    }

    pub fn run(&mut self) {
        // Parse server as host:port or tcp://host:port
        let server = self.config.server.trim_start_matches("tcp://");
        let mut parts = server.split(':');
        let host = parts.next().unwrap_or("localhost");
        let port: u16 = parts.next().and_then(|p| p.parse().ok()).unwrap_or(1883);

        let create_opts = mqtt::CreateOptionsBuilder::new()
            .server_uri(format!("tcp://{}:{}", host, port))
            .client_id(&self.config.client_id)
            .finalize();

        let cli = mqtt::Client::new(create_opts).expect("Failed to create MQTT client");

        // Set credentials if provided
        let mut conn_opts_builder = mqtt::ConnectOptionsBuilder::new();
        if !self.config.username.is_empty() {
            conn_opts_builder.user_name(&self.config.username).password(&self.config.password);
        }
        let conn_opts = conn_opts_builder.keep_alive_interval(std::time::Duration::from_secs(5)).finalize();

        cli.connect(conn_opts).expect("Failed to connect to MQTT broker");

        // Subscribe to HA status
        cli.subscribe("homeassistant/status", 1).expect("Failed to subscribe");

        // Immediately send discovery messages after connect
        let disco = self.mode.get_all_discovery_messages();
        for (topic, payload) in disco {
            let msg = mqtt::Message::new(topic, payload, 1);
            if let Err(e) = cli.publish(msg) {
                eprintln!("[MQTT] discovery publish failed: {}", e);
            }
        }

        let rx = cli.start_consuming();

        loop {
            // Check for incoming MQTT messages
            if let Some(msg_opt) = rx.recv_timeout(std::time::Duration::from_millis(100)).ok() {
                if let Some(msg) = msg_opt {
                    if msg.topic() == "homeassistant/status" && msg.payload_str() == "online" {
                        println!("HA online, resending discovery");
                        let msgs = self.mode.get_all_discovery_messages();
                        for (topic, payload) in msgs {
                            let msg = mqtt::Message::new(topic, payload, 1);
                            if let Err(e) = cli.publish(msg) {
                                eprintln!("[MQTT] discovery publish failed: {}", e);
                            }
                        }
                    }
                }
            }

            // Process outgoing publishes
            match self.rx.try_recv() {
                Ok((label, value)) => {
                    let meter = self.mode.get_meter_id();
                    if meter.is_empty() {
                        println!("Skipped publish: meter_id not set for label {}", label);
                        continue;
                    }
                    let topic = self.mode.get_mqtt_topic(&label);
                    let msg = mqtt::Message::new(topic, value, 1);
                    if let Err(e) = cli.publish(msg) {
                        eprintln!("[MQTT] publish error: {}", e);
                        break;
                    }
                }
                Err(std::sync::mpsc::TryRecvError::Empty) => {},
                Err(std::sync::mpsc::TryRecvError::Disconnected) => {
                    println!("publisher channel closed");
                    break;
                }
            }
        }
        cli.disconnect(None).ok();
    }
}
