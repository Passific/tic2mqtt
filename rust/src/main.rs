use tokio::sync::mpsc;

mod mqtt;
mod serial;
mod tic;
mod utils;

use mqtt::MqttPublisher;
use serial::SerialReader;
use tic::{TicModeEnum, TicModeHandle};

struct Opt {
    serial: Option<String>,
    mqtt_server: Option<String>,
    mqtt_user: Option<String>,
    mqtt_pass: Option<String>,
    mqtt_client_id: Option<String>,
    mode: Option<String>,
}

fn parse_args() -> Opt {
    let mut opt = Opt {
        serial: None,
        mqtt_server: None,
        mqtt_user: None,
        mqtt_pass: None,
        mqtt_client_id: None,
        mode: None,
    };
    let args: Vec<String> = std::env::args().collect();
    let mut i = 1;
    while i < args.len() {
        match args[i].as_str() {
            "--serial" => { i += 1; if i < args.len() { opt.serial = Some(args[i].clone()); } },
            "--mqtt_server" => { i += 1; if i < args.len() { opt.mqtt_server = Some(args[i].clone()); } },
            "--mqtt_user" => { i += 1; if i < args.len() { opt.mqtt_user = Some(args[i].clone()); } },
            "--mqtt_pass" => { i += 1; if i < args.len() { opt.mqtt_pass = Some(args[i].clone()); } },
            "--mqtt_client_id" => { i += 1; if i < args.len() { opt.mqtt_client_id = Some(args[i].clone()); } },
            "--mode" => { i += 1; if i < args.len() { opt.mode = Some(args[i].clone()); } },
            _ => {},
        }
        i += 1;
    }
    opt
}

fn get_tic_mode(opt_mode: Option<String>) -> TicModeEnum {
    let mode_str = opt_mode
        .or_else(|| std::env::var("TIC_MODE").ok())
        .unwrap_or_else(|| "standard".into())
        .to_lowercase();

    match mode_str.as_str() {
        "historique" => TicModeEnum::Historique,
        _ => TicModeEnum::Standard,
    }
}

fn get_env_or(opt: Option<String>, var: &str, default: &str) -> String {
    opt.or_else(|| std::env::var(var).ok())
        .unwrap_or_else(|| default.into())
}

#[tokio::main]
async fn main() {
    let opt = parse_args();

    // Get configuration from CLI args or environment
    let mqtt_server = get_env_or(opt.mqtt_server, "MQTT_SERVER", "tcp://localhost:1883");
    let mqtt_user = get_env_or(opt.mqtt_user, "MQTT_USER", "");
    let mqtt_pass = get_env_or(opt.mqtt_pass, "MQTT_PASS", "");
    let mqtt_client_id = get_env_or(opt.mqtt_client_id, "MQTT_CLIENT_ID", "tic2mqtt_client");
    let serial_port = get_env_or(opt.serial, "SERIAL_PORT", "/dev/ttyUSB0");

    // Channels
    let (line_tx, mut line_rx) = mpsc::channel::<String>(256);
    let (publish_tx, publish_rx) = mpsc::channel::<(String, String)>(256);
    let (shutdown_tx, shutdown_rx) = tokio::sync::watch::channel(false);

    // Initialize TIC mode
    let tic_mode = TicModeHandle::new(get_tic_mode(opt.mode));
    let baudrate = tic_mode.baudrate();

    // Start serial reader with configured port
    let mut serial = SerialReader::new(Some(serial_port), line_tx).with_baud(baudrate);
    let mut serial_shutdown = shutdown_rx.clone();
    let serial_handle = tokio::spawn(async move { serial.run(&mut serial_shutdown).await });

    // Start MQTT publisher with full configuration
    let mqtt_config = mqtt::MqttConfig {
        server: mqtt_server,
        client_id: mqtt_client_id,
        username: mqtt_user,
        password: mqtt_pass,
    };
    let mut mqtt = MqttPublisher::new(mqtt_config, publish_rx, tic_mode.clone());
    let mut mqtt_shutdown = shutdown_rx.clone();
    let mqtt_handle = tokio::spawn(async move { mqtt.run(&mut mqtt_shutdown).await });

    // listen for ctrl-c and signal shutdown
    let shutdown_trigger = shutdown_tx.clone();
    tokio::spawn(async move {
        if let Err(e) = tokio::signal::ctrl_c().await {
            eprintln!("failed to listen for ctrl_c: {}", e);
        }
        let _ = shutdown_trigger.send(true);
    });

    // Main loop: parse lines into label/value and forward
    while let Some(line) = line_rx.recv().await {
        let line = line.trim().to_string();
        if line.is_empty() {
            continue;
        }
        // simple label/value split (first whitespace)
        if let Some((label, value)) = utils::parse_label_value(&line) {
            tic_mode.handle_label_value(&label, &value);
            let safe_value = utils::sanitize_value(&value);
            let _ = publish_tx.send((label.clone(), safe_value)).await;
        } else {
            println!("invalid line: {}", line);
        }
    }

    // wait for tasks to finish
    let _ = serial_handle.await;
    let _ = mqtt_handle.await;
}
