use std::sync::mpsc;
use std::sync::{Arc, atomic::{AtomicBool, Ordering}};
use std::thread;

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

fn main() {
    let opt = parse_args();

    // Get configuration from CLI args or environment
    let mqtt_server = get_env_or(opt.mqtt_server, "MQTT_SERVER", "tcp://localhost:1883");
    let mqtt_user = get_env_or(opt.mqtt_user, "MQTT_USER", "");
    let mqtt_pass = get_env_or(opt.mqtt_pass, "MQTT_PASS", "");
    let mqtt_client_id = get_env_or(opt.mqtt_client_id, "MQTT_CLIENT_ID", "tic2mqtt_client");
    let serial_port = get_env_or(opt.serial, "SERIAL_PORT", "/dev/ttyUSB0");

    // Channels
    let (line_tx, line_rx) = mpsc::channel::<String>();
    let (publish_tx, publish_rx) = mpsc::channel::<(String, String)>();
    let shutdown = Arc::new(AtomicBool::new(false));

    // Initialize TIC mode
    let tic_mode = std::sync::Arc::new(TicModeHandle::new(get_tic_mode(opt.mode)));
    let baudrate = tic_mode.baudrate();

    // Start serial reader with configured port
    let serial_shutdown = shutdown.clone();
    let serial_handle = {
        let _tic_mode = tic_mode.clone();
        thread::spawn(move || {
            let mut serial = SerialReader::new(Some(serial_port), line_tx).with_baud(baudrate);
            serial.run(&serial_shutdown);
        })
    };

    // Start MQTT publisher with full configuration
    let mqtt_config = mqtt::MqttConfig {
        server: mqtt_server,
        client_id: mqtt_client_id,
        username: mqtt_user,
        password: mqtt_pass,
    };
    let mqtt_handle = {
        let tic_mode = tic_mode.clone();
        thread::spawn(move || {
            let mut mqtt = MqttPublisher::new(mqtt_config, publish_rx, (*tic_mode).clone());
            mqtt.run();
        })
    };

    // listen for ctrl-c and signal shutdown
    let shutdown_ctrlc = shutdown.clone();
    ctrlc::set_handler(move || {
        shutdown_ctrlc.store(true, Ordering::SeqCst);
    }).expect("Error setting Ctrl-C handler");

    // Main loop: parse lines into label/value and forward
    while let Ok(line) = line_rx.recv() {
        let line = line.trim().to_string();
        if line.is_empty() {
            continue;
        }
        if let Some((label, value)) = utils::parse_label_value(&line) {
            let is_frame_start = label == "ADCO" || label == "ADSC";
            if is_frame_start {
                // On frame start, publish previous frame if any
                let meter_id = tic_mode.get_meter_id();
                if !meter_id.is_empty() {
                    let label_values = tic_mode.get_label_values();
                    if !label_values.is_empty() {
                        // Manually build JSON: { "LABEL": { "raw": "value" }, ... }
                        let mut payload = String::from("{");
                        let mut first = true;
                        for (k, v) in label_values.iter() {
                            if !first { payload.push(','); } else { first = false; }
                            payload.push('"');
                            payload.push_str(&k.replace('"', "\""));
                            payload.push_str("\": {\"raw\": \"");
                            payload.push_str(&v.replace('"', "\""));
                            payload.push_str("\"}");
                        }
                        payload.push('}');
                        let topic = format!("tic2mqtt/{}", meter_id);
                        let _ = publish_tx.send((topic, payload));
                    }
                }
            }
            tic_mode.handle_label_value(&label, &value);
        } else {
            println!("invalid line: {}", line);
        }
        if shutdown.load(Ordering::SeqCst) {
            break;
        }
    }

    // wait for tasks to finish
    let _ = serial_handle.join();
    let _ = mqtt_handle.join();
}
