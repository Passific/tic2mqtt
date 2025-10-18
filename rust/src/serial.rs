use tokio::io::{AsyncReadExt};
use tokio_serial::SerialPortBuilderExt;
use tokio::sync::mpsc::Sender;

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
    pub async fn run(&mut self, shutdown: &mut tokio::sync::watch::Receiver<bool>) {
        if let Some(dev) = &self.device {
            // try to open serial port with tokio-serial
            loop {
                if *shutdown.borrow() { break; }
                match tokio_serial::new(dev, self.baudrate)
                    .data_bits(tokio_serial::DataBits::Seven)
                    .parity(tokio_serial::Parity::Even)
                    .stop_bits(tokio_serial::StopBits::One)
                    .open_native_async()
                {
                    Ok(mut port) => {
                        let mut buf = [0u8; 1];
                        let mut line = String::new();
                        loop {
                            match port.read_exact(&mut buf).await {
                                Ok(_) => {
                                    let ch = buf[0] as char;
                                    if ch == '\n' {
                                        if !line.is_empty() {
                                            let _ = self.tx.send(line.clone()).await;
                                            line.clear();
                                        }
                                    } else if ch != '\r' {
                                        line.push(ch);
                                    }
                                }
                                Err(e) => {
                                    eprintln!("[Serial] read error: {}. Reopening...", e);
                                    break;
                                }
                            }
                            if *shutdown.borrow() { break; }
                        }
                    }
                    Err(e) => {
                        eprintln!("[Serial] open failed: {}. Retrying in 5s...", e);
                        tokio::time::sleep(tokio::time::Duration::from_secs(5)).await;
                        continue;
                    }
                }
            }
        } else {
            // stdin fallback
            let mut stdin = tokio::io::stdin();
            let mut buf = [0u8; 1024];
            let mut acc = String::new();
            loop {
                if *shutdown.borrow() { break; }
                match stdin.read(&mut buf).await {
                    Ok(0) => break,
                    Ok(n) => {
                        for &b in &buf[..n] {
                            let ch = b as char;
                            if ch == '\n' {
                                if !acc.is_empty() {
                                    let s = acc.clone();
                                    let _ = self.tx.send(s).await;
                                    acc.clear();
                                }
                            } else if ch != '\r' {
                                acc.push(ch);
                            }
                        }
                    }
                    Err(_) => break,
                }
            }
        }
    }
}
