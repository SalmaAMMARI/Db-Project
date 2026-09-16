#include "minidb/buffer_manager/buffer_manager.hpp"

BufferManager::BufferManager(size_t pool_size_) : pool_size(pool_size_) {
}

void BufferManager::register_table(int table_id, HeapFile* heap_file) {
	table_files[table_id] = heap_file;
}

HeapFile* BufferManager::get_heap_file(int table_id) {
	auto it = table_files.find(table_id);
	if (it == table_files.end()) {
		return (nullptr);
	}
	return it->second;
}

Page& BufferManager::fetch_page(const PageId& page_id) {
	auto it = page_table.find(page_id);
	if (it != page_table.end()) {
		it->second.pin_count++;
		update_LRU(page_id); // TODO(lab1): set CLOCK ref_bit instead
		return it->second.page;
	}

	if (page_table.size() >= pool_size) {
		this->evict_page();
	}

	HeapFile* heap_file = table_files[page_id.table_id];
	Page	  page		= heap_file->read_page(page_id.page_number);

	BufferFrame frame;
	frame.page_id	= page_id;
	frame.page		= std::move(page);
	frame.pin_count = 1;
	frame.is_dirty	= false;
	// TODO(lab1): frame.ref_bit = true;

	page_table[page_id] = frame;
	this->update_LRU(page_id); // TODO(lab1): append to CLOCK frame ring

	return (page_table[page_id].page);
}

void BufferManager::unpin_page(const PageId& page_id, bool is_dirty) {
	auto it = page_table.find(page_id);

	if (it == page_table.end())
		return;
	if (it->second.pin_count == 0)
		return;
	it->second.pin_count--;
	if (is_dirty) {
		it->second.is_dirty = true;
	}
}

void BufferManager::flush_page(const PageId& page_id) {
	auto it = page_table.find(page_id);

	if (it == page_table.end())
		return;

	BufferFrame& frame = it->second;
	if (frame.is_dirty) {
		HeapFile* heap_file = table_files[frame.page_id.table_id];

		heap_file->write_page(frame.page_id.page_number, frame.page);
		frame.is_dirty = false;
	}
}

void BufferManager::flush_all() {
	for (const auto& p : page_table) {
		this->flush_page(p.first);
	}
}

void BufferManager::update_LRU(const PageId& page_id) {
	lrulist.remove(page_id);
	lrulist.push_front(page_id);
}

void BufferManager::evict_page() {
	// Baseline LRU victim selection. TODO(lab1): replace with CLOCK scan:
	// skip pinned frames; if ref_bit set, clear it and advance; else evict
	// (flush if dirty) and return.
	for (auto it = lrulist.rbegin(); it != lrulist.rend(); ++it) {
		const PageId& candidate_id = *it;
		BufferFrame&  frame		   = page_table[candidate_id];

		if (frame.pin_count != 0) {
			continue;
		}

		if (frame.is_dirty) {
			HeapFile* heap_file = table_files[candidate_id.table_id];
			heap_file->write_page(candidate_id.page_number, frame.page);
		}

		page_table.erase(candidate_id);
		lrulist.remove(candidate_id);
		return;
	}
}
