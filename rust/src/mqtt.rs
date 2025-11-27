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

        // Immediately send discovery messages after connect, with retain flag
        let disco = self.mode.get_all_discovery_messages();
        for (topic, payload) in disco {
            let mut msg = mqtt::Message::new(topic, payload, 1);
            msg.set_retained(true);
            if let Err(e) = cli.publish(msg) {
                eprintln!("[MQTT] discovery publish failed: {}", e);
            }
        }

        loop {
            // Process outgoing publishes (now topic is full frame topic, value is JSON)
            match self.rx.try_recv() {
                Ok((topic, payload)) => {
                    let msg = mqtt::Message::new(topic, payload, 1);
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
