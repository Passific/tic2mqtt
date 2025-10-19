use std::io::{BufRead, BufReader};
use std::sync::mpsc::Sender;
use std::time::Duration;

pub struct SerialReader {
    device: Option<String>,
    tx: Sender<String>,
    baudrate: u32,
}

impl SerialReader {
    pub fn new(device: Option<String>, tx: Sender<String>) -> Self {
        SerialReader { device, tx, baudrate: 9600 }
    }

    pub fn with_baud(mut self, baud: u32) -> Self { self.baudrate = baud; self }

    /// Run serial read task. If device is None, fallback to stdin.
    pub fn run(&mut self, shutdown: &std::sync::Arc<std::sync::atomic::AtomicBool>) {
        if let Some(dev) = &self.device {
            // try to open serial port with serialport crate
            loop {
                if shutdown.load(std::sync::atomic::Ordering::SeqCst) { break; }
                match serialport::new(dev, self.baudrate)
                    .data_bits(serialport::DataBits::Seven)
                    .parity(serialport::Parity::Even)
                    .stop_bits(serialport::StopBits::One)
                    .timeout(Duration::from_secs(5))
                    .open()
                {
                    Ok(port) => {
                        let mut reader = BufReader::new(port);
                        let mut line = String::new();
                        loop {
                            line.clear();
                            match reader.read_line(&mut line) {
                                Ok(0) => break, // EOF
                                Ok(_) => {
                                    let line = line.trim_end_matches(['\r', '\n'].as_ref()).to_string();
                                    if !line.is_empty() {
                                        let _ = self.tx.send(line.clone());
                                    }
                                }
                                Err(e) => {
                                    eprintln!("[Serial] read error: {}. Reopening...", e);
                                    break;
                                }
                            }
                            if shutdown.load(std::sync::atomic::Ordering::SeqCst) { break; }
                        }
                    }
                    Err(e) => {
                        eprintln!("[Serial] open failed: {}. Retrying in 5s...", e);
                        std::thread::sleep(Duration::from_secs(5));
                        continue;
                    }
                }
            }
        } else {
            // stdin fallback
            let stdin = std::io::stdin();
            let mut reader = stdin.lock();
            let mut acc = String::new();
            loop {
                if shutdown.load(std::sync::atomic::Ordering::SeqCst) { break; }
                acc.clear();
                match reader.read_line(&mut acc) {
                    Ok(0) => break,
                    Ok(_) => {
                        let line = acc.trim_end_matches(['\r', '\n'].as_ref()).to_string();
                        if !line.is_empty() {
                            let _ = self.tx.send(line);
                        }
                    }
                    Err(_) => break,
                }
            }
        }
    }
}
