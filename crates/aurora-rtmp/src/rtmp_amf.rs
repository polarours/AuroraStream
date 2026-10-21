use bytes::{BufMut, BytesMut};

/// Minimal AMF0 implementation, sufficient for the RTMP command messages the server
/// has to understand (`connect`, `createStream`, `publish`, `play`, ...) and to answer.
#[derive(Debug, Clone, PartialEq)]
pub enum AmfValue {
    Number(f64),
    Boolean(bool),
    String(String),
    Object(Vec<(String, AmfValue)>),
    Null,
    Undefined,
    EcmaArray(Vec<(String, AmfValue)>),
    StrictArray(Vec<AmfValue>),
    Date(f64),
    LongString(String),
}

impl AmfValue {
    pub fn as_str(&self) -> Option<&str> {
        match self {
            AmfValue::String(s) | AmfValue::LongString(s) => Some(s),
            _ => None,
        }
    }

    pub fn as_number(&self) -> Option<f64> {
        match self {
            AmfValue::Number(n) => Some(*n),
            _ => None,
        }
    }

    pub fn as_object(&self) -> Option<&[(String, AmfValue)]> {
        match self {
            AmfValue::Object(o) | AmfValue::EcmaArray(o) => Some(o),
            _ => None,
        }
    }

    /// Looks up a key inside an object or array value.
    pub fn get(&self, key: &str) -> Option<&AmfValue> {
        self.as_object()?
            .iter()
            .find(|(k, _)| k == key)
            .map(|(_, v)| v)
    }
}

#[derive(Debug)]
pub enum AmfError {
    Truncated,
    UnknownMarker(u8),
}

impl std::fmt::Display for AmfError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            AmfError::Truncated => write!(f, "truncated AMF value"),
            AmfError::UnknownMarker(m) => write!(f, "unsupported AMF marker 0x{:02x}", m),
        }
    }
}

/// Parses a single AMF0 value starting at `offset`, advancing it past the value.
pub fn parse(data: &[u8], offset: &mut usize) -> Result<AmfValue, AmfError> {
    let marker = *data.get(*offset).ok_or(AmfError::Truncated)?;
    *offset += 1;

    match marker {
        0x00 => {
            let bytes = data.get(*offset..*offset + 8).ok_or(AmfError::Truncated)?;
            *offset += 8;
            Ok(AmfValue::Number(f64::from_be_bytes(
                bytes.try_into().unwrap(),
            )))
        }
        0x01 => {
            let b = *data.get(*offset).ok_or(AmfError::Truncated)?;
            *offset += 1;
            Ok(AmfValue::Boolean(b != 0))
        }
        0x02 => {
            let len = read_u16(data, offset)?;
            let s = read_str(data, offset, len as usize)?;
            Ok(AmfValue::String(s))
        }
        0x03 => Ok(AmfValue::Object(parse_object_body(data, offset)?)),
        0x05 => Ok(AmfValue::Null),
        0x06 => Ok(AmfValue::Undefined),
        0x08 => {
            // ECMA array: 4 byte count followed by the same body as an object.
            if data.len() < *offset + 4 {
                return Err(AmfError::Truncated);
            }
            *offset += 4;
            Ok(AmfValue::EcmaArray(parse_object_body(data, offset)?))
        }
        0x0A => {
            let count = read_u32(data, offset)? as usize;
            let mut items = Vec::with_capacity(count.min(1024));
            for _ in 0..count {
                items.push(parse(data, offset)?);
            }
            Ok(AmfValue::StrictArray(items))
        }
        0x0B => {
            // Date: 8 byte timestamp plus a 2 byte timezone.
            let bytes = data.get(*offset..*offset + 10).ok_or(AmfError::Truncated)?;
            *offset += 10;
            Ok(AmfValue::Date(f64::from_be_bytes(
                bytes[..8].try_into().unwrap(),
            )))
        }
        0x0C => {
            let len = read_u32(data, offset)? as usize;
            let s = read_str(data, offset, len)?;
            Ok(AmfValue::LongString(s))
        }
        other => Err(AmfError::UnknownMarker(other)),
    }
}

/// Parses the name/value pairs of an object, stopping at the object end marker.
fn parse_object_body(data: &[u8], offset: &mut usize) -> Result<Vec<(String, AmfValue)>, AmfError> {
    let mut entries = Vec::new();
    loop {
        let key_len = read_u16(data, offset)?;
        // The object end marker is an empty key followed by 0x09.
        if key_len == 0 {
            let marker = *data.get(*offset).ok_or(AmfError::Truncated)?;
            *offset += 1;
            if marker != 0x09 {
                return Err(AmfError::UnknownMarker(marker));
            }
            return Ok(entries);
        }
        let key = read_str(data, offset, key_len as usize)?;
        let value = parse(data, offset)?;
        entries.push((key, value));
    }
}

/// Encodes an AMF0 value into `out`.
pub fn encode(value: &AmfValue, out: &mut BytesMut) {
    match value {
        AmfValue::Number(n) => {
            out.put_u8(0x00);
            out.put_u64(n.to_bits());
        }
        AmfValue::Boolean(b) => {
            out.put_u8(0x01);
            out.put_u8(if *b { 1 } else { 0 });
        }
        AmfValue::String(s) => {
            out.put_u8(0x02);
            out.put_u16(s.len() as u16);
            out.put_slice(s.as_bytes());
        }
        AmfValue::LongString(s) => {
            out.put_u8(0x0C);
            out.put_u32(s.len() as u32);
            out.put_slice(s.as_bytes());
        }
        AmfValue::Null => out.put_u8(0x05),
        AmfValue::Undefined => out.put_u8(0x06),
        AmfValue::Object(entries) => {
            out.put_u8(0x03);
            encode_object_body(entries, out);
        }
        AmfValue::EcmaArray(entries) => {
            out.put_u8(0x08);
            out.put_u32(entries.len() as u32);
            encode_object_body(entries, out);
        }
        AmfValue::StrictArray(items) => {
            out.put_u8(0x0A);
            out.put_u32(items.len() as u32);
            for item in items {
                encode(item, out);
            }
        }
        AmfValue::Date(t) => {
            out.put_u8(0x0B);
            out.put_u64(t.to_bits());
            out.put_u16(0);
        }
    }
}

fn encode_object_body(entries: &[(String, AmfValue)], out: &mut BytesMut) {
    for (key, value) in entries {
        out.put_u16(key.len() as u16);
        out.put_slice(key.as_bytes());
        encode(value, out);
    }
    // Object end marker: empty key + 0x09.
    out.put_u16(0);
    out.put_u8(0x09);
}

/// Builds an AMF0 command payload: command name, transaction id and optional arguments.
pub fn encode_command(name: &str, transaction_id: f64, args: &[AmfValue]) -> BytesMut {
    let mut out = BytesMut::new();
    encode(&AmfValue::String(name.to_string()), &mut out);
    encode(&AmfValue::Number(transaction_id), &mut out);
    for arg in args {
        encode(arg, &mut out);
    }
    out
}

/// Builds an AMF0 `onStatus` notification.
pub fn encode_status(
    transaction_id: f64,
    level: &str,
    code: &str,
    description: &str,
) -> BytesMut {
    let info = vec![
        (
            "level".to_string(),
            AmfValue::String(level.to_string()),
        ),
        (
            "code".to_string(),
            AmfValue::String(code.to_string()),
        ),
        (
            "description".to_string(),
            AmfValue::String(description.to_string()),
        ),
    ];

    let mut out = BytesMut::new();
    encode(&AmfValue::String("onStatus".to_string()), &mut out);
    encode(&AmfValue::Number(transaction_id), &mut out);
    encode(&AmfValue::Null, &mut out);
    encode(&AmfValue::Object(info), &mut out);
    out
}

fn read_u16(data: &[u8], offset: &mut usize) -> Result<u16, AmfError> {
    let bytes = data.get(*offset..*offset + 2).ok_or(AmfError::Truncated)?;
    *offset += 2;
    Ok(u16::from_be_bytes(bytes.try_into().unwrap()))
}

fn read_u32(data: &[u8], offset: &mut usize) -> Result<u32, AmfError> {
    let bytes = data.get(*offset..*offset + 4).ok_or(AmfError::Truncated)?;
    *offset += 4;
    Ok(u32::from_be_bytes(bytes.try_into().unwrap()))
}

fn read_str(data: &[u8], offset: &mut usize, len: usize) -> Result<String, AmfError> {
    let bytes = data.get(*offset..*offset + len).ok_or(AmfError::Truncated)?;
    *offset += len;
    Ok(String::from_utf8_lossy(bytes).into_owned())
}
