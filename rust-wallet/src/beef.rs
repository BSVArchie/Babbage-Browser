/// BRC-62 BEEF (Background Evaluation Extended Format) Parser
///
/// BEEF is a format for packaging Bitcoin transactions with their ancestry
/// and optional SPV proofs for verification without a full blockchain.
///
/// Format:
/// - version (1 byte)
/// - num_bumps (varint) - number of merkle proofs
/// - bumps (array of merkle proofs)
/// - num_txs (varint) - number of transactions
/// - transactions (array of raw transactions, parents first)

use std::io::{Cursor, Read};

/// BEEF format version
pub const BEEF_VERSION: u8 = 0x00;

/// Represents a parsed BEEF structure
#[derive(Debug, Clone)]
pub struct Beef {
    pub version: u8,
    pub bumps: Vec<MerkleProof>,
    pub transactions: Vec<Vec<u8>>, // Raw transaction bytes (parents first, main tx last)
}

/// Represents a merkle proof (BUMPs - Block Unspent Merkle Proofs)
#[derive(Debug, Clone)]
pub struct MerkleProof {
    pub block_height: u32,
    pub tree_height: u8,
    pub path: Vec<Vec<u8>>, // Merkle path hashes
}

impl Beef {
    /// Parse BEEF format from hex string
    pub fn from_hex(hex: &str) -> Result<Self, String> {
        let bytes = hex::decode(hex)
            .map_err(|e| format!("Invalid BEEF hex: {}", e))?;
        Self::from_bytes(&bytes)
    }

    /// Parse BEEF format from raw bytes
    pub fn from_bytes(bytes: &[u8]) -> Result<Self, String> {
        let mut cursor = Cursor::new(bytes);

        // Read version
        let version = read_u8(&mut cursor)?;
        if version != BEEF_VERSION {
            return Err(format!("Unsupported BEEF version: {}", version));
        }

        // Read number of BUMPs (merkle proofs)
        let num_bumps = read_varint(&mut cursor)?;

        // Read BUMPs
        let mut bumps = Vec::new();
        for _ in 0..num_bumps {
            bumps.push(read_bump(&mut cursor)?);
        }

        // Read number of transactions
        let num_txs = read_varint(&mut cursor)?;

        // Read transactions
        let mut transactions = Vec::new();
        for _ in 0..num_txs {
            let tx_bytes = read_transaction(&mut cursor)?;
            transactions.push(tx_bytes);
        }

        Ok(Beef {
            version,
            bumps,
            transactions,
        })
    }

    /// Get the main transaction (last in the array)
    pub fn main_transaction(&self) -> Option<&Vec<u8>> {
        self.transactions.last()
    }

    /// Get parent transactions (all except the last)
    pub fn parent_transactions(&self) -> &[Vec<u8>] {
        if self.transactions.len() > 1 {
            &self.transactions[..self.transactions.len() - 1]
        } else {
            &[]
        }
    }

    /// Check if BEEF has merkle proofs for SPV
    pub fn has_proofs(&self) -> bool {
        !self.bumps.is_empty()
    }
}

/// Read a single byte
fn read_u8(cursor: &mut Cursor<&[u8]>) -> Result<u8, String> {
    let mut buf = [0u8; 1];
    cursor.read_exact(&mut buf)
        .map_err(|e| format!("Failed to read byte: {}", e))?;
    Ok(buf[0])
}

/// Read a variable-length integer (Bitcoin varint format)
fn read_varint(cursor: &mut Cursor<&[u8]>) -> Result<u64, String> {
    let first = read_u8(cursor)?;

    match first {
        0..=0xfc => Ok(first as u64),
        0xfd => {
            let mut buf = [0u8; 2];
            cursor.read_exact(&mut buf)
                .map_err(|e| format!("Failed to read varint: {}", e))?;
            Ok(u16::from_le_bytes(buf) as u64)
        }
        0xfe => {
            let mut buf = [0u8; 4];
            cursor.read_exact(&mut buf)
                .map_err(|e| format!("Failed to read varint: {}", e))?;
            Ok(u32::from_le_bytes(buf) as u64)
        }
        0xff => {
            let mut buf = [0u8; 8];
            cursor.read_exact(&mut buf)
                .map_err(|e| format!("Failed to read varint: {}", e))?;
            Ok(u64::from_le_bytes(buf))
        }
    }
}

/// Read a merkle proof (BUMP)
fn read_bump(cursor: &mut Cursor<&[u8]>) -> Result<MerkleProof, String> {
    // Read block height
    let mut buf = [0u8; 4];
    cursor.read_exact(&mut buf)
        .map_err(|e| format!("Failed to read block height: {}", e))?;
    let block_height = u32::from_le_bytes(buf);

    // Read tree height
    let tree_height = read_u8(cursor)?;

    // Read merkle path
    let path_len = read_varint(cursor)?;
    let mut path = Vec::new();
    for _ in 0..path_len {
        let mut hash = vec![0u8; 32];
        cursor.read_exact(&mut hash)
            .map_err(|e| format!("Failed to read merkle hash: {}", e))?;
        path.push(hash);
    }

    Ok(MerkleProof {
        block_height,
        tree_height,
        path,
    })
}

/// Read a raw transaction
fn read_transaction(cursor: &mut Cursor<&[u8]>) -> Result<Vec<u8>, String> {
    let start_pos = cursor.position() as usize;
    let total_len = cursor.get_ref().len();

    // Read version (4 bytes)
    let mut version = [0u8; 4];
    cursor.read_exact(&mut version)
        .map_err(|e| format!("Failed to read tx version at pos {}/{}: {}", cursor.position(), total_len, e))?;

    // Read input count
    let input_count = read_varint(cursor)
        .map_err(|e| format!("Failed to read input count at pos {}/{}: {}", cursor.position(), total_len, e))?;

    // Read inputs
    for i in 0..input_count {
        // Previous output (32 bytes txid + 4 bytes vout)
        let mut prev_out = [0u8; 36];
        cursor.read_exact(&mut prev_out)
            .map_err(|e| format!("Failed to read input {} prev_out at pos {}/{}: {}", i, cursor.position(), total_len, e))?;

        // Script length and script
        let script_len = read_varint(cursor)
            .map_err(|e| format!("Failed to read input {} script length at pos {}/{}: {}", i, cursor.position(), total_len, e))?;
        let mut script = vec![0u8; script_len as usize];
        cursor.read_exact(&mut script)
            .map_err(|e| format!("Failed to read input {} script ({} bytes) at pos {}/{}: {}", i, script_len, cursor.position(), total_len, e))?;

        // Sequence (4 bytes)
        let mut sequence = [0u8; 4];
        cursor.read_exact(&mut sequence)
            .map_err(|e| format!("Failed to read input {} sequence at pos {}/{}: {}", i, cursor.position(), total_len, e))?;
    }

    // Read output count
    let output_count = read_varint(cursor)
        .map_err(|e| format!("Failed to read output count at pos {}/{}: {}", cursor.position(), total_len, e))?;

    // Read outputs
    for i in 0..output_count {
        // Value (8 bytes)
        let mut value = [0u8; 8];
        cursor.read_exact(&mut value)
            .map_err(|e| format!("Failed to read output {} value at pos {}/{}: {}", i, cursor.position(), total_len, e))?;

        // Script length and script
        let script_len = read_varint(cursor)
            .map_err(|e| format!("Failed to read output {} script length at pos {}/{}: {}", i, cursor.position(), total_len, e))?;
        let mut script = vec![0u8; script_len as usize];
        cursor.read_exact(&mut script)
            .map_err(|e| format!("Failed to read output {} script ({} bytes) at pos {}/{}: {}", i, script_len, cursor.position(), total_len, e))?;
    }

    // Read locktime (4 bytes)
    let mut locktime = [0u8; 4];
    cursor.read_exact(&mut locktime)
        .map_err(|e| format!("Failed to read locktime at pos {}/{}: {}", cursor.position(), total_len, e))?;

    // Extract the full transaction bytes
    let end_pos = cursor.position() as usize;
    let all_bytes = cursor.get_ref();
    let tx_bytes = all_bytes[start_pos..end_pos].to_vec();

    Ok(tx_bytes)
}

/// Parse a raw Bitcoin transaction to extract metadata
#[derive(Debug, Clone)]
pub struct ParsedTransaction {
    pub version: u32,
    pub inputs: Vec<ParsedInput>,
    pub outputs: Vec<ParsedOutput>,
    pub lock_time: u32,
}

#[derive(Debug, Clone)]
pub struct ParsedInput {
    pub prev_txid: String,
    pub prev_vout: u32,
    pub script: Vec<u8>,
    pub sequence: u32,
}

#[derive(Debug, Clone)]
pub struct ParsedOutput {
    pub value: i64,
    pub script: Vec<u8>,
}

impl ParsedTransaction {
    /// Parse a raw transaction from bytes
    pub fn from_bytes(bytes: &[u8]) -> Result<Self, String> {
        let mut cursor = Cursor::new(bytes);

        // Read version
        let mut version_buf = [0u8; 4];
        cursor.read_exact(&mut version_buf)
            .map_err(|e| format!("Failed to read version: {}", e))?;
        let version = u32::from_le_bytes(version_buf);

        // Read inputs
        let input_count = read_varint(&mut cursor)?;
        let mut inputs = Vec::new();
        for _ in 0..input_count {
            // Read previous output (txid + vout)
            let mut prev_txid_bytes = [0u8; 32];
            cursor.read_exact(&mut prev_txid_bytes)
                .map_err(|e| format!("Failed to read prev txid: {}", e))?;

            // Reverse for display (Bitcoin uses little-endian)
            let prev_txid = hex::encode(prev_txid_bytes.iter().rev().copied().collect::<Vec<u8>>());

            let mut prev_vout_buf = [0u8; 4];
            cursor.read_exact(&mut prev_vout_buf)
                .map_err(|e| format!("Failed to read prev vout: {}", e))?;
            let prev_vout = u32::from_le_bytes(prev_vout_buf);

            // Read script
            let script_len = read_varint(&mut cursor)?;
            let mut script = vec![0u8; script_len as usize];
            cursor.read_exact(&mut script)
                .map_err(|e| format!("Failed to read script: {}", e))?;

            // Read sequence
            let mut sequence_buf = [0u8; 4];
            cursor.read_exact(&mut sequence_buf)
                .map_err(|e| format!("Failed to read sequence: {}", e))?;
            let sequence = u32::from_le_bytes(sequence_buf);

            inputs.push(ParsedInput {
                prev_txid,
                prev_vout,
                script,
                sequence,
            });
        }

        // Read outputs
        let output_count = read_varint(&mut cursor)?;
        let mut outputs = Vec::new();
        for _ in 0..output_count {
            // Read value
            let mut value_buf = [0u8; 8];
            cursor.read_exact(&mut value_buf)
                .map_err(|e| format!("Failed to read value: {}", e))?;
            let value = i64::from_le_bytes(value_buf);

            // Read script
            let script_len = read_varint(&mut cursor)?;
            let mut script = vec![0u8; script_len as usize];
            cursor.read_exact(&mut script)
                .map_err(|e| format!("Failed to read script: {}", e))?;

            outputs.push(ParsedOutput {
                value,
                script,
            });
        }

        // Read locktime
        let mut locktime_buf = [0u8; 4];
        cursor.read_exact(&mut locktime_buf)
            .map_err(|e| format!("Failed to read locktime: {}", e))?;
        let lock_time = u32::from_le_bytes(locktime_buf);

        Ok(ParsedTransaction {
            version,
            inputs,
            outputs,
            lock_time,
        })
    }

    /// Parse from hex string
    pub fn from_hex(hex: &str) -> Result<Self, String> {
        let bytes = hex::decode(hex)
            .map_err(|e| format!("Invalid hex: {}", e))?;
        Self::from_bytes(&bytes)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_varint_parsing() {
        // Small value (< 0xfd)
        let bytes = vec![0x12];
        let mut cursor = Cursor::new(bytes.as_slice());
        assert_eq!(read_varint(&mut cursor).unwrap(), 0x12);

        // 2-byte value
        let bytes = vec![0xfd, 0x00, 0x01];
        let mut cursor = Cursor::new(bytes.as_slice());
        assert_eq!(read_varint(&mut cursor).unwrap(), 0x0100);
    }
}
