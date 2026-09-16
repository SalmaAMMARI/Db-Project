#ifndef __BUFFER_MANAGER_HPP__
#define __BUFFER_MANAGER_HPP__

#include "heapfile.hpp"
#include "page.hpp"

#include <list>
#include <unordered_map>
#include <vector>

/**
 * @struct BufferFrame
 * @brief Represents a frame in the buffer pool that caches a page.
 *
 * Stores the page data, page ID, dirty flag, and pin count.
 * Lab 1: add a reference bit (or equivalent) for CLOCK.
 */
struct BufferFrame {
	PageId page_id;
	Page   page;
	bool   is_dirty	 = false;
	int	   pin_count = 0;
	// TODO(lab1): add CLOCK metadata (e.g. bool ref_bit = false;)
};

/**
 * @class BufferManager
 * @brief Manages an in-memory buffer pool for caching pages from multiple tables.
 *
 * Handout baseline uses LRU so the DB runs and most tests pass.
 * Lab 1: replace LRU with CLOCK (second-chance). See LAB1.md.
 */
class BufferManager {
  private:
	size_t									pool_size;
	std::unordered_map<int, HeapFile*>		table_files;
	std::unordered_map<PageId, BufferFrame> page_table;

	/* Baseline LRU — replace with CLOCK state (frame ring + clock hand). */
	std::list<PageId> lrulist;
	void			  update_LRU(const PageId& page_id);
	// TODO(lab1): std::vector<PageId> frame_list; size_t clock_hand = 0;

	void evict_page();

  public:
	BufferManager(size_t pool_size);
	void	  register_table(int tableId, HeapFile* heap_file);
	HeapFile* get_heap_file(int table_id);

	Page& fetch_page(const PageId& page_id);
	void  unpin_page(const PageId& page_id, bool is_dirty);
	void  flush_page(const PageId& page_id);
	void  flush_all();

	/** True if the page currently resides in the buffer pool (for tests / debugging). */
	bool is_cached(const PageId& page_id) const {
		return page_table.find(page_id) != page_table.end();
	}

	int allocate_page(int table_id) {
		HeapFile* file = this->get_heap_file(table_id);
		return (file->allocate_page());
	}
};

#endif
