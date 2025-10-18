use rumqttc::{AsyncClient, MqttOptions, QoS, Incoming};
use tokio::sync::mpsc::Receiver;
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

    pub async fn run(&mut self, shutdown: &mut tokio::sync::watch::Receiver<bool>) {
        // Parse server as host:port or tcp://host:port
        // For simplicity assume tcp://host:port
        let server = self.config.server.trim_start_matches("tcp://");
        let mut parts = server.split(':');
        let host = parts.next().unwrap_or("localhost");
        let port: u16 = parts.next().and_then(|p| p.parse().ok()).unwrap_or(1883);

        let mut mqttoptions = MqttOptions::new(&self.config.client_id, host, port);
        mqttoptions.set_keep_alive(std::time::Duration::from_secs(5));
        
        // Set credentials if provided
        if !self.config.username.is_empty() {
            mqttoptions.set_credentials(&self.config.username, &self.config.password);
        }

        loop {
            let (client, mut eventloop) = AsyncClient::new(mqttoptions.clone(), 10);
            // subscribe to HA status
            if let Err(e) = client.subscribe("homeassistant/status", QoS::AtLeastOnce).await {
                eprintln!("[MQTT] subscribe failed: {}", e);
            }

            // Immediately send discovery messages after connect
            let disco = self.mode.get_all_discovery_messages();
            for (topic, payload) in disco {
                if let Err(e) = client.publish(topic, QoS::AtLeastOnce, false, payload).await {
                    eprintln!("[MQTT] discovery publish failed: {}", e);
                }
            }

            // Combined loop: process events and publishes
            loop {
                if *shutdown.borrow() { println!("shutdown signalled, breaking mqtt loop"); break; }
                tokio::select! {
                    // process mqtt events
                    event = eventloop.poll() => {
                        match event {
                            Ok(notification) => {
                                if let rumqttc::Event::Incoming(Incoming::Publish(p)) = notification {
                                    if p.topic == "homeassistant/status" {
                                        if let Ok(payload) = String::from_utf8(p.payload.to_vec()) {
                                            if payload == "online" {
                                                println!("HA online, resending discovery");
                                                // publish discovery messages
                                                let msgs = self.mode.get_all_discovery_messages();
                                                for (topic, payload) in msgs {
                                                    if let Err(e) = client.publish(topic, QoS::AtLeastOnce, false, payload).await {
                                                        eprintln!("[MQTT] discovery publish failed: {}", e);
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            Err(e) => { eprintln!("[MQTT] eventloop error: {}", e); break; }
                        }
                    }

                    // process outgoing publishes
                    maybe = self.rx.recv() => {
                        match maybe {
                            Some((label, value)) => {
                                let meter = self.mode.get_meter_id();
                                if meter.is_empty() {
                                    println!("Skipped publish: meter_id not set for label {}", label);
                                    continue;
                                }
                                let topic = self.mode.get_mqtt_topic(&label);
                                if let Err(e) = client.publish(topic, QoS::AtLeastOnce, false, value).await {
                                    eprintln!("[MQTT] publish error: {}", e);
                                    break;
                                }
                            }
                            None => { println!("publisher channel closed"); break; }
                        }
                    }
                }
            }

            if *shutdown.borrow() { println!("shutdown signalled, exiting mqtt outer loop"); break; }
            tokio::time::sleep(tokio::time::Duration::from_secs(5)).await;
        }
    }
}
