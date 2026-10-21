use std::collections::VecDeque;

use parking_lot::Mutex;

pub struct MemoryBlock {
    data: Vec<u8>,
    capacity: usize,
}

impl MemoryBlock {
    fn new(capacity: usize) -> Self {
        Self {
            data: vec![0u8; capacity],
            capacity,
        }
    }

    pub fn as_slice(&self) -> &[u8] {
        &self.data[..self.capacity]
    }

    pub fn as_mut_slice(&mut self) -> &mut [u8] {
        &mut self.data[..self.capacity]
    }

    pub fn capacity(&self) -> usize {
        self.capacity
    }
}

pub struct MemoryPool {
    video_blocks: Mutex<VecDeque<MemoryBlock>>,
    audio_blocks: Mutex<VecDeque<MemoryBlock>>,
    video_block_size: usize,
    audio_block_size: usize,
}

impl MemoryPool {
    pub fn new(video_block_size: usize, audio_block_size: usize) -> Self {
        Self {
            video_blocks: Mutex::new(VecDeque::new()),
            audio_blocks: Mutex::new(VecDeque::new()),
            video_block_size,
            audio_block_size,
        }
    }

    pub fn acquire_video_block(&self) -> MemoryBlock {
        let mut blocks = self.video_blocks.lock();
        blocks
            .pop_front()
            .unwrap_or_else(|| MemoryBlock::new(self.video_block_size))
    }

    pub fn release_video_block(&self, block: MemoryBlock) {
        let mut blocks = self.video_blocks.lock();
        if blocks.len() < 64 {
            blocks.push_back(block);
        }
    }

    pub fn acquire_audio_block(&self) -> MemoryBlock {
        let mut blocks = self.audio_blocks.lock();
        blocks
            .pop_front()
            .unwrap_or_else(|| MemoryBlock::new(self.audio_block_size))
    }

    pub fn release_audio_block(&self, block: MemoryBlock) {
        let mut blocks = self.audio_blocks.lock();
        if blocks.len() < 128 {
            blocks.push_back(block);
        }
    }

    pub fn video_pool_size(&self) -> usize {
        self.video_blocks.lock().len()
    }

    pub fn audio_pool_size(&self) -> usize {
        self.audio_blocks.lock().len()
    }
}

impl Default for MemoryPool {
    fn default() -> Self {
        Self::new(2 * 1024 * 1024, 64 * 1024)
    }
}
