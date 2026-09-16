
function(add_minidb_test name source)
	add_executable(${name} ${source})
	target_include_directories(${name} PRIVATE ${PROJECT_SOURCE_DIR}/include)
	target_link_libraries(${name} PRIVATE minidb)
	add_test(NAME ${name} COMMAND ${name})
endfunction()

add_minidb_test(test_page test_page.cpp)
add_minidb_test(test_heapfile test_heapfile.cpp)
add_minidb_test(test_buffer_manager test_buffer_manager.cpp)
add_minidb_test(test_table_manager test_table_manager.cpp)
add_minidb_test(test_catalog_manager test_catalog.cpp)
add_minidb_test(test_key_serialize_deserialize test_keys.cpp)
add_minidb_test(test_bptree_node test_bptree_node.cpp)
add_minidb_test(test_bptree test_bptree.cpp)
add_minidb_test(test_index_manager test_index_manager.cpp)
add_minidb_test(test_id_generator test_id_generator.cpp)

