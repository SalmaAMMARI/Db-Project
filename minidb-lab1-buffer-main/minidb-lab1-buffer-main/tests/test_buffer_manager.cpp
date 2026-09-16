#include "minidb/buffer_manager/buffer_manager.hpp"
#include "minidb/buffer_manager/heapfile.hpp"
#include "minidb/buffer_manager/page.hpp"

#include <cassert>
#include <cstdio>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

const std::string TEST_FILE = "test_buffer_manager.tbl";

void cleanup() {
	if (fs::exists(TEST_FILE)) {
		std::remove(TEST_FILE.c_str());
	}
}

void test_buffer_manager_basic_fetch_unpin_flush() {
	cleanup();

	HeapFile hf(TEST_FILE);
	int		 page_num = hf.allocate_page();
	{
		Page  p;
		auto& data = p.get_data();
		data[0]	   = 42;
		hf.write_page(page_num, p);
	}

	BufferManager bm(2);
	bm.register_table(1, &hf);

	PageId pid{1, page_num};
	Page&  fetched = bm.fetch_page(pid);
	assert(fetched.get_data()[0] == 42);

	fetched.get_data()[0] = 99;
	bm.unpin_page(pid, true);
	bm.flush_page(pid);

	Page verify = hf.read_page(page_num);
	assert(verify.get_data()[0] == 99);

	std::cout << "[PASS] Fetch → Modify → Unpin → Flush cycle.\n";
}

void test_eviction_writes_dirty_page() {
	cleanup();

	HeapFile hf(TEST_FILE);
	int		 pg0 = hf.allocate_page();
	int		 pg1 = hf.allocate_page();
	int		 pg2 = hf.allocate_page();

	BufferManager bm(2);
	bm.register_table(1, &hf);

	PageId pid0{1, pg0}, pid1{1, pg1}, pid2{1, pg2};

	Page& p0		 = bm.fetch_page(pid0);
	p0.get_data()[0] = 100;
	bm.unpin_page(pid0, true);

	Page& p1 = bm.fetch_page(pid1);
	bm.unpin_page(pid1, false);

	Page& p2 = bm.fetch_page(pid2);
	bm.unpin_page(pid2, false);

	Page reloaded = hf.read_page(pg0);
	assert(reloaded.get_data()[0] == 100);

	std::cout << "[PASS] Dirty page is flushed before eviction.\n";
}

void test_pinned_page_not_evicted() {
	cleanup();

	HeapFile hf(TEST_FILE);
	int		 pg0 = hf.allocate_page();
	int		 pg1 = hf.allocate_page();
	int		 pg2 = hf.allocate_page();

	{
		Page p;
		p.get_data()[0] = 7;
		hf.write_page(pg0, p);
	}

	BufferManager bm(2);
	bm.register_table(1, &hf);

	PageId pid0{1, pg0}, pid1{1, pg1}, pid2{1, pg2};

	Page& p0 = bm.fetch_page(pid0); // remains pinned
	assert(p0.get_data()[0] == 7);

	bm.fetch_page(pid1);
	bm.unpin_page(pid1, false);

	bm.fetch_page(pid2);
	bm.unpin_page(pid2, false);

	assert(bm.is_cached(pid0));
	assert(p0.get_data()[0] == 7);
	bm.unpin_page(pid0, false);

	std::cout << "[PASS] Pinned page is never evicted.\n";
}

/**
 * CLOCK second-chance with pool size 2:
 *   load A, load B, re-access A, load C
 *
 * Frames are [A, B] with the hand at A. One full scan clears both ref bits,
 * then A (ref=0) is evicted. B and C remain cached.
 *
 * Note: LRU would instead evict B (least recently used) and keep A — so this
 * test fails if students leave the provided LRU baseline unchanged.
 */
void test_clock_policy_second_chance() {
	cleanup();

	HeapFile hf(TEST_FILE);
	int		 pg0 = hf.allocate_page();
	int		 pg1 = hf.allocate_page();
	int		 pg2 = hf.allocate_page();

	{
		Page p;
		p.get_data()[0] = 11;
		hf.write_page(pg0, p);
		p.get_data()[0] = 22;
		hf.write_page(pg1, p);
		p.get_data()[0] = 33;
		hf.write_page(pg2, p);
	}

	BufferManager bm(2);
	bm.register_table(1, &hf);

	PageId pidA{1, pg0}, pidB{1, pg1}, pidC{1, pg2};

	bm.fetch_page(pidA);
	bm.unpin_page(pidA, false);
	bm.fetch_page(pidB);
	bm.unpin_page(pidB, false);

	bm.fetch_page(pidA);
	bm.unpin_page(pidA, false);

	bm.fetch_page(pidC);
	bm.unpin_page(pidC, false);

	assert(!bm.is_cached(pidA));
	assert(bm.is_cached(pidB));
	assert(bm.is_cached(pidC));

	std::cout << "[PASS] CLOCK second chance evicted A, kept B.\n";
}

int main() {
	test_buffer_manager_basic_fetch_unpin_flush();
	test_eviction_writes_dirty_page();
	test_pinned_page_not_evicted();
	test_clock_policy_second_chance();
	cleanup();

	std::cout << "[ALL TESTS PASSED]\n";
	return 0;
}
