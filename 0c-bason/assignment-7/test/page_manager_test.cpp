#include "gtest/gtest.h"
#include "../src/page_manager.hpp"

using namespace basonlite;

class PageManagerTest : public ::testing::Test {
protected:
    const std::string db_path = "testdb.bin";
    const std::size_t page_size = 256;

    void TearDown() override {
        std::remove(db_path.c_str());
        std::remove((db_path + ".wal").c_str());
    }
};

TEST_F(PageManagerTest, InitializationCreatesHeader) {
    PageManager pm = PageManager::open(db_path, page_size);
    DatabaseHeader h = pm.header();
    EXPECT_EQ(h.version, 1);
    EXPECT_EQ(h.page_size, page_size);
    EXPECT_EQ(h.total_pages, 1);
    EXPECT_EQ(h.free_head, 0);
    EXPECT_EQ(h.root_page, 0);
}

TEST_F(PageManagerTest, ReadWritePage) {
    PageManager pm = PageManager::open(db_path, page_size);
    uint32_t page_no = pm.allocate_page();

    Page page(page_size);
    std::fill(page.bytes.begin(), page.bytes.end(), 0xAB);

    pm.write_page(page_no, page);

    Page read_back = pm.read_page(page_no);
    EXPECT_EQ(read_back.bytes, page.bytes);
}

TEST_F(PageManagerTest, AllocateAndFreePage) {
    PageManager pm = PageManager::open(db_path, page_size);

    uint32_t p1 = pm.allocate_page();
    uint32_t p2 = pm.allocate_page();

    pm.free_page(p1);
    uint32_t p3 = pm.allocate_page();

    EXPECT_EQ(p3, p1);
    EXPECT_NE(p2, p3);
}

TEST_F(PageManagerTest, HeaderUpdatedAfterAllocate) {
    PageManager pm = PageManager::open(db_path, page_size);
    uint32_t p1 = pm.allocate_page();
    uint32_t p2 = pm.allocate_page();

    DatabaseHeader h = pm.header();
    EXPECT_EQ(h.total_pages, 3);
}

TEST_F(PageManagerTest, ReadInvalidPageThrows) {
    PageManager pm = PageManager::open(db_path, page_size);
    EXPECT_THROW(pm.read_page(999), std::runtime_error);
}