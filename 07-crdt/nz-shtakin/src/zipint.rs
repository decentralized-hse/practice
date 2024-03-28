use std::io::{self, Error};

const BYTES4: u64 = 0xffffffff;
const BYTES2: u64 = 0xffff;
const BYTES1: u64 = 0xff;

fn byte_len(n: u64) -> u8 {
    if n <= BYTES1 {
        if n == 0 {
            return 0;
        }
        return 1;
    }
    if n <= BYTES2 {
        return 2;
    }
    if n <= BYTES4 {
        return 4;
    }
    return 8;
}

pub fn zip_uint64_pair(big: u64, lil: u64) -> Vec<u8> {
    let mut ret: [u8; 16] = [0; 16];
    let pat: u8 = (byte_len(big) << 4) | byte_len(lil);
    // println!("{:08b}", pat);
    match pat {
        0x00 => {
            return Vec::<u8>::new();
        },
        0x10 => {
            ret[0] = big as u8;
            return ret[0..1].to_vec();
        },
        0x01 | 0x11 => {
            ret[0] = big as u8;
            ret[1] = lil as u8;
            return ret[0..2].to_vec();
        },
        0x20 | 0x21 => {
            ret[0..2].copy_from_slice(&big.to_le_bytes()[0..2]);
            ret[2] = lil as u8;
            return ret[0..3].to_vec();
        },
        0x02 | 0x12 | 0x22 => {
            ret[0..2].copy_from_slice(&big.to_le_bytes()[0..2]);
            ret[2..4].copy_from_slice(&lil.to_le_bytes()[0..2]);
            return ret[0..4].to_vec();
        },
        0x40 | 0x41 => {
            ret[0..4].copy_from_slice(&big.to_le_bytes()[0..4]);
            ret[4] = lil as u8;
            return ret[0..5].to_vec();
        },
        0x42 => {
            ret[0..4].copy_from_slice(&big.to_le_bytes()[0..4]);
            ret[4..6].copy_from_slice(&lil.to_le_bytes()[0..2]);
            return ret[0..6].to_vec();
        },
        0x04 | 0x14 | 0x24 | 0x44 => {
            ret[0..4].copy_from_slice(&big.to_le_bytes()[0..4]);
            ret[4..8].copy_from_slice(&lil.to_le_bytes()[0..4]);
            return ret[0..8].to_vec();
        },
        0x80 | 0x81 => {
            ret[0..8].copy_from_slice(&big.to_le_bytes()[0..8]);
            ret[8] = lil as u8;
            return ret[0..9].to_vec();
        },
        0x82 => {
            ret[0..8].copy_from_slice(&big.to_le_bytes()[0..8]);
            ret[8..10].copy_from_slice(&lil.to_le_bytes()[0..2]);
            return ret[0..10].to_vec();
        },
        0x84 => {
            ret[0..8].copy_from_slice(&big.to_le_bytes()[0..8]);
            ret[8..12].copy_from_slice(&lil.to_le_bytes()[0..4]);
            return ret[0..12].to_vec();
        },
        0x08 | 0x18 | 0x28 | 0x48 | 0x88 => {
            ret[0..8].copy_from_slice(&big.to_le_bytes()[0..8]);
            ret[8..16].copy_from_slice(&lil.to_le_bytes()[0..8]);
            return ret[0..16].to_vec();
        },
        _ => {
            return Vec::<u8>::new()
        }
    }
}

pub fn unzip_uint64_pair(buf: &[u8]) -> Result<(u64, u64), Error> {
    let big : u64;
    let lil : u64;
	match buf.len() {
        0 => {
            return Ok((0, 0));
        },
        1 => {
            return Ok((buf[0] as u64, 0));
        },
        2 => {
            return Ok((buf[0] as u64, buf[1] as u64))
        },
        3 => {
            big = u16::from_le_bytes(buf[0..2].try_into().unwrap()) as u64;
            lil = buf[2] as u64;
        },
        4 => {
            big = u16::from_le_bytes(buf[0..2].try_into().unwrap()) as u64;
            lil = u16::from_le_bytes(buf[2..4].try_into().unwrap()) as u64;
        },
        5 => {
            big = u32::from_le_bytes(buf[0..4].try_into().unwrap()) as u64;
            lil = buf[4] as u64;
        },
        6 => {
            big = u32::from_le_bytes(buf[0..4].try_into().unwrap()) as u64;
            lil = u16::from_le_bytes(buf[4..6].try_into().unwrap()) as u64;
        },
        8 => {
            big = u32::from_le_bytes(buf[0..4].try_into().unwrap()) as u64;
            lil = u32::from_le_bytes(buf[4..8].try_into().unwrap()) as u64;
        },
        9 => {
            big = u64::from_le_bytes(buf[0..8].try_into().unwrap());
            lil = buf[8] as u64;
        },
        10 => {
            big = u64::from_le_bytes(buf[0..8].try_into().unwrap());
            lil = u16::from_le_bytes(buf[8..10].try_into().unwrap()) as u64;
        },
        12 => {
            big = u64::from_le_bytes(buf[0..8].try_into().unwrap());
            lil = u32::from_le_bytes(buf[8..12].try_into().unwrap()) as u64;
        },
        16 => {
            big = u64::from_le_bytes(buf[0..8].try_into().unwrap());
            lil = u64::from_le_bytes(buf[8..16].try_into().unwrap());
        },
        _ => {
            return Err(io::Error::new(io::ErrorKind::InvalidData,format!("invalid uint64_pair data")));
        }
    }
	return Ok((big, lil));
}

pub fn zip_uint64(mut v: u64) -> Vec<u8> {
    let mut buf: [u8; 8] = [0; 8];
	let mut i = 0;
	while v > 0 {
		buf[i] = v as u8;
		v >>= 8;
		i += 1
	}
	return buf[0..i].to_vec();
}

pub fn unzip_uint64(zip: &[u8]) -> u64 {
    let mut v : u64 = 0;
	for i in zip.len()..0 {
		v <<= 8;
		v |= zip[i] as u64
	}
	return v;
}
