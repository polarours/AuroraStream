use tokio::io::{AsyncReadExt, AsyncWriteExt};

use aurora_core::error::Result;
use aurora_core::plugin::PluginContext;

const RTMP_VERSION: u8 = 3;
const HANDSHAKE_SIZE: usize = 1536;

/// Performs the RTMP handshake.
///
/// The exchange is `C0 C1` -> `S0 S1 S2` -> `C2`. The server has to send `S2` together
/// with `S0`/`S1`; a client that only receives `S0`/`S1` waits for the remaining bytes
/// and aborts with "Cannot read RTMP handshake response".
pub async fn handshake(stream: &mut tokio::net::TcpStream, ctx: &PluginContext) -> Result<()> {
    // C0: protocol version.
    let mut c0 = [0u8; 1];
    stream.read_exact(&mut c0).await?;
    if c0[0] != RTMP_VERSION {
        ctx.warn(format!(
            "unexpected RTMP version: {}, expected {}",
            c0[0],
            RTMP_VERSION
        ));
    }

    // C1: 1536 bytes of timestamp plus random data.
    let mut c1 = [0u8; HANDSHAKE_SIZE];
    stream.read_exact(&mut c1).await?;

    // S0 (version) + S1 (timestamp + random) + S2 (echo of C1).
    stream.write_all(&[RTMP_VERSION]).await?;
    stream.write_all(&generate_s1()).await?;
    stream.write_all(&generate_s2(&c1)).await?;
    stream.flush().await?;

    // C2: the client echoes S1 back to us.
    let mut c2 = [0u8; HANDSHAKE_SIZE];
    stream.read_exact(&mut c2).await?;

    ctx.debug("RTMP handshake completed");
    Ok(())
}

fn generate_s1() -> [u8; HANDSHAKE_SIZE] {
    let mut s1 = [0u8; HANDSHAKE_SIZE];
    let time = std::time::SystemTime::now()
        .duration_since(std::time::UNIX_EPOCH)
        .unwrap_or_default()
        .as_secs() as u32;

    s1[0] = (time >> 24) as u8;
    s1[1] = (time >> 16) as u8;
    s1[2] = (time >> 8) as u8;
    s1[3] = time as u8;

    // Zero length field.
    s1[4] = 0;
    s1[5] = 0;
    s1[6] = 0;
    s1[7] = 0;

    for i in 8..HANDSHAKE_SIZE {
        s1[i] = (i * 73) as u8;
    }

    s1
}

fn generate_s2(c1: &[u8; HANDSHAKE_SIZE]) -> [u8; HANDSHAKE_SIZE] {
    let mut s2 = [0u8; HANDSHAKE_SIZE];
    s2.copy_from_slice(c1);
    s2
}
