#include "../include/allocator_global_heap.h"

#include <not_implemented.h>

#include <sstream>

[[nodiscard]] void *allocator_global_heap::do_allocate_sm(size_t size) {
    void *ptr;
    debug_with_guard("Allocation of size " + std::to_string(size) + " started");
    try {
        ptr = ::operator new(size);
    } catch (std::bad_alloc &e) {
        error_with_guard("Failed to allocate memory of size " + std::to_string(size));
        throw;
    }

    debug_with_guard("Successfully allocated memory at 0x" +
                     (std::ostringstream{} << std::hex << reinterpret_cast<std::uintptr_t>(ptr)).str() +
                     " of size " + std::to_string(size));
    return ptr;
}

void allocator_global_heap::do_deallocate_sm(void *at) {
    if (!at) {
        error_with_guard("Nothing to deallocate");
        return;
    }

    std::ostringstream oss;
    oss << std::hex << reinterpret_cast<std::uintptr_t>(at);
    debug_with_guard("Deallocation of memory at 0x" + oss.str() + " started");
    ::operator delete(at);
    debug_with_guard("Successfully deallocated memory at 0x" + oss.str() + " finished");
}

inline logger *allocator_global_heap::get_logger() const {
    return _logger;
}

inline std::string allocator_global_heap::get_typename() const {
    return "allocator_global_heap";
}

allocator_global_heap::allocator_global_heap(logger *logger) : _logger(logger) {
    trace_with_guard("Constructor of allocator_global_heap started");
    trace_with_guard("Constructor of allocator_global_heap finished");
}

allocator_global_heap::~allocator_global_heap() {
    trace_with_guard("Destructor of allocator_global_heap started");
    trace_with_guard("Destructor of allocator_global_heap finished");
}

allocator_global_heap::allocator_global_heap(const allocator_global_heap &other) : _logger(other._logger) {
    trace_with_guard("Copy constructor of allocator_global_heap started");
    trace_with_guard("Copy constructor of allocator_global_heap finished");
}

allocator_global_heap &allocator_global_heap::operator=(const allocator_global_heap &other) {
    if (this != &other) {
        trace_with_guard("Assignment constructor of allocator_global_heap started");
        _logger = other._logger;
        trace_with_guard("Assignment constructor of allocator_global_heap finished");
    }

    return *this;
}

bool allocator_global_heap::do_is_equal(const std::pmr::memory_resource &other) const noexcept {
    return dynamic_cast<const allocator_global_heap *>(&other) != nullptr;
}

allocator_global_heap::allocator_global_heap(allocator_global_heap &&other) noexcept : _logger(other._logger) {
    trace_with_guard("Move constructor of allocator_global_heap started");
    trace_with_guard("Move constructor of allocator_global_heap finished");
}

allocator_global_heap &allocator_global_heap::operator=(allocator_global_heap &&other) noexcept {
    if (this != &other) {
        trace_with_guard("Move assignment constructor of allocator_global_heap started");
        _logger = other._logger;
        trace_with_guard("Move assignment constructor of allocator_global_heap finished");
    }

    return *this;
}