use anyhow::Result;
use structopt::StructOpt;
use tokio::sync::mpsc;

mod mqtt;
mod serial;
mod tic;
mod utils;

use mqtt::MqttPublisher;
use serial::SerialReader;
use tic::{TicModeEnum, TicModeHandle};

#[derive(StructOpt, Debug)]
struct Opt {
    /// Serial device path. If omitted, stdin is used (for testing).
    #[structopt(long)]
    serial: Option<String>,

    /// MQTT server (not used by stdout mock)
    #[structopt(long)]
    mqtt_server: Option<String>,

    /// MQTT username for authentication
    #[structopt(long)]
    mqtt_user: Option<String>,

    /// MQTT password for authentication
    #[structopt(long)]
    mqtt_pass: Option<String>,

    /// MQTT client ID. Default from env MQTT_CLIENT_ID or "tic2mqtt_client"
    #[structopt(long)]
    mqtt_client_id: Option<String>,

    /// TIC mode (standard or historique). Default from env TIC_MODE or standard.
    #[structopt(long)]
    mode: Option<String>,
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
async fn main() -> Result<()> {
    tracing_subscriber::fmt::init();

    let opt = Opt::from_args();

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
    let baudrate = tic_mode.inner.lock().unwrap().baudrate();

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
            tracing::error!("failed to listen for ctrl_c: {}", e);
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
            tracing::info!(%line, "invalid line");
        }
    }

    // wait for tasks to finish
    let _ = serial_handle.await;
    let _ = mqtt_handle.await;

    Ok(())
}
