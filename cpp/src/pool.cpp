#include "aurora_media.h"
#include <stdlib.h>
#include <string.h>

struct PoolBlock {
    uint8_t *data;
    size_t capacity;
    int in_use;
};

struct PoolContext {
    PoolBlock *blocks;
    size_t block_count;
    size_t block_size;
};

AuroraFilterHandle aurora_pool_create(size_t block_size, size_t block_count) {
    auto pool = new PoolContext();
    pool->block_size = block_size;
    pool->block_count = block_count;
    pool->blocks = static_cast<PoolBlock*>(calloc(block_count, sizeof(PoolBlock)));

    for (size_t i = 0; i < block_count; i++) {
        pool->blocks[i].data = static_cast<uint8_t*>(malloc(block_size));
        pool->blocks[i].capacity = block_size;
        pool->blocks[i].in_use = 0;
    }

    return static_cast<AuroraFilterHandle>(pool);
}

uint8_t* aurora_pool_acquire(AuroraFilterHandle handle, size_t *out_size) {
    auto pool = static_cast<PoolContext*>(handle);
    if (!pool) return nullptr;

    for (size_t i = 0; i < pool->block_count; i++) {
        if (!pool->blocks[i].in_use) {
            pool->blocks[i].in_use = 1;
            *out_size = pool->blocks[i].capacity;
            return pool->blocks[i].data;
        }
    }
    return nullptr;
}

void aurora_pool_release(AuroraFilterHandle handle, uint8_t *ptr) {
    auto pool = static_cast<PoolContext*>(handle);
    if (!pool) return;

    for (size_t i = 0; i < pool->block_count; i++) {
        if (pool->blocks[i].data == ptr) {
            pool->blocks[i].in_use = 0;
            return;
        }
    }
}

void aurora_pool_destroy(AuroraFilterHandle handle) {
    auto pool = static_cast<PoolContext*>(handle);
    if (!pool) return;
    for (size_t i = 0; i < pool->block_count; i++) {
        free(pool->blocks[i].data);
    }
    free(pool->blocks);
    delete pool;
}
