#include <not_implemented.h>
#include <cstddef>
#include "../include/allocator_buddies_system.h"
#include <sstream>

using byte = unsigned char;

allocator_buddies_system::~allocator_buddies_system()
{
    if (_trusted_memory) {
        auto byte_ptr = reinterpret_cast<byte*>(_trusted_memory);
        auto mutex_ptr = reinterpret_cast<std::mutex*>(byte_ptr + sizeof(logger*) + sizeof(allocator_dbg_helper*) + sizeof(fit_mode) + sizeof(unsigned char));
        mutex_ptr->~mutex();
        ::operator delete(_trusted_memory);
    }
    _trusted_memory = nullptr;
    debug_with_guard("Destructor: allocator resources cleaned");
}

allocator_buddies_system::allocator_buddies_system(
        allocator_buddies_system &&other) noexcept : _trusted_memory(other._trusted_memory)
{
    other._trusted_memory = nullptr;
    debug_with_guard("Move constructor: resources transferred");
}

allocator_buddies_system &allocator_buddies_system::operator=(
        allocator_buddies_system &&other) noexcept
{
    if (this != &other) {
        std::swap(_trusted_memory, other._trusted_memory);
    }
    debug_with_guard("Move assignment: resources swapped");
    return *this;
}

allocator_buddies_system::allocator_buddies_system(
        size_t space_size_power_of_two,
        std::pmr::memory_resource *parent_allocator,
        logger *logger,
        allocator_with_fit_mode::fit_mode allocate_fit_mode)
{
    if (space_size_power_of_two < min_k) {
        error_with_guard("Constructor: requested size too small");
        throw std::logic_error("Requested size too small");
    }

    size_t real_size = (1 << space_size_power_of_two) + allocator_metadata_size;
    if (parent_allocator == nullptr) {
        try {
            _trusted_memory = ::operator new(real_size);
        } catch (std::bad_alloc& ex) {
            error_with_guard("Constructor: system allocation failed");
            throw;
        }
    } else {
        try {
            _trusted_memory = parent_allocator->allocate(real_size, 1);
        } catch (std::bad_alloc& ex) {
            error_with_guard("Constructor: parent allocator failed");
            throw;
        }
    }

    fill_allocator_fields(space_size_power_of_two, parent_allocator, logger, allocate_fit_mode);
    debug_with_guard(std::string("Constructor: allocator initialized with size 2^") + std::to_string(space_size_power_of_two));
}

void allocator_buddies_system::fill_allocator_fields(size_t space_size_power_of_two,
                                                     std::pmr::memory_resource *parent_allocator,
                                                     logger *logger,
                                                     allocator_with_fit_mode::fit_mode allocate_fit_mode)
{
    void* memory =_trusted_memory;
    *reinterpret_cast<class logger**>(memory) = logger;
    memory = reinterpret_cast<void*>(reinterpret_cast<byte*>(memory) + sizeof(class logger*));

    *reinterpret_cast<std::pmr::memory_resource**>(memory) = parent_allocator;
    memory = reinterpret_cast<void*>(reinterpret_cast<byte*>(memory) + sizeof(std::pmr::memory_resource*));

    *reinterpret_cast<allocator_with_fit_mode::fit_mode*>(memory) = allocate_fit_mode;
    memory = reinterpret_cast<void*>(reinterpret_cast<byte*>(memory) + sizeof(allocator_with_fit_mode::fit_mode));

    *reinterpret_cast<byte*>(memory) = space_size_power_of_two;
    memory = reinterpret_cast<void*>(reinterpret_cast<byte*>(memory) + sizeof(byte));

    auto mut = reinterpret_cast<std::mutex*>(memory);
    new (mut) :: std::mutex();
    memory = reinterpret_cast<void*>(reinterpret_cast<byte*>(memory) + sizeof(std::mutex));

    block_metadata* first_block = reinterpret_cast<block_metadata*>(memory);
    (*first_block).occupied = false;
    first_block->size = space_size_power_of_two;
    debug_with_guard(std::string("Initial block created: size=2^") + std::to_string(space_size_power_of_two));
}

std::string allocator_buddies_system::get_info_in_string(const std::vector<allocator_test_utils::block_info>& vec) noexcept
{
    std::ostringstream str;
    for (auto& it : vec)
    {
        if (it.is_block_occupied)
        {
            str << "<occupied>";
        } else {
            str << "<free>";
        }
        str << " <" + std::to_string(it.block_size) + "> | ";
    }
    return str.str();
}

std::mutex &allocator_buddies_system::get_mutex() const noexcept
{
    auto byte_ptr = reinterpret_cast<byte*>(_trusted_memory);
    return *reinterpret_cast<std::mutex*>(byte_ptr + sizeof(logger*) + sizeof(std::pmr::memory_resource*) + sizeof(fit_mode) + sizeof(unsigned char));
}

[[nodiscard]] void *allocator_buddies_system::do_allocate_sm(size_t size)
{
    std::lock_guard lock(get_mutex());
    debug_with_guard(std::string("Allocation started for ") + std::to_string(size) + " bytes");

    size_t real_size = size + occupied_block_metadata_size;
    information_with_guard(std::string("Current blocks state: ") + get_info_in_string(get_blocks_info()));

    void* free_block;

    switch (get_fit_mod()) {
        case allocator_with_fit_mode::fit_mode::first_fit:
            debug_with_guard("Using FIRST_FIT strategy");
            free_block = get_first(real_size);
            break;
        case allocator_with_fit_mode::fit_mode::the_best_fit:
            debug_with_guard("Using BEST_FIT strategy");
            free_block = get_best(real_size);
            break;
        case allocator_with_fit_mode::fit_mode::the_worst_fit:
            debug_with_guard("Using WORST_FIT strategy");
            free_block = get_worst(real_size);
            break;
        default:
            error_with_guard("Invalid allocation strategy");
            throw std::logic_error("Invalid allocation strategy");
    }

    if (free_block == nullptr) {
        error_with_guard("Allocation failed - no suitable block found");
        throw std::bad_alloc();
    }

    while (get_size_block(free_block) >= (real_size << 1)) {
        debug_with_guard(std::string("Splitting block of size 2^") + std::to_string(get_size_block(free_block)));

        auto first_twin = reinterpret_cast<block_metadata*>(free_block);
        --(first_twin->size);

        auto second_twin = reinterpret_cast<block_metadata*>(get_twin(free_block));
        second_twin->occupied = false;
        second_twin->size = first_twin->size;
    }

    auto find_twin = reinterpret_cast<block_metadata*>(free_block);
    find_twin->occupied = true;

    debug_with_guard(std::string("Successfully allocated block of size 2^") + std::to_string(find_twin->size));
    information_with_guard(std::string("Blocks state after allocation: ") + get_info_in_string(get_blocks_info()));

    return reinterpret_cast<void*>(reinterpret_cast<byte*>(free_block) + occupied_block_metadata_size);
}

void allocator_buddies_system::do_deallocate_sm(void *at)
{
    std::lock_guard lock(get_mutex());
    debug_with_guard("Deallocation started");

    void* current_block = reinterpret_cast<byte*>(at) - occupied_block_metadata_size;
    size_t current_block_size = get_size_block(current_block) - occupied_block_metadata_size;

    debug_with_guard("condition of block before deallocate: " + get_dump(reinterpret_cast<char*>(at), current_block_size));

    reinterpret_cast<block_metadata*>(current_block)->occupied = false;

    void* twin = get_twin(current_block);

    while (get_size_block(current_block) < get_size_full() &&
           get_size_block(current_block) == get_size_block(twin) &&
           !(reinterpret_cast<block_metadata*>(twin)->occupied))
    {
        debug_with_guard("Merging buddy blocks");

        void* left_twin = current_block < twin ? current_block : twin;
        auto current_meta = reinterpret_cast<block_metadata*>(left_twin);
        ++current_meta->size;

        current_block = left_twin;
        twin = get_twin(current_block);
    }

    debug_with_guard("Deallocation completed");
    information_with_guard(std::string("Blocks state after deallocation: ") + get_info_in_string(get_blocks_info()));
}

bool allocator_buddies_system::do_is_equal(const std::pmr::memory_resource &other) const noexcept
{
    if (typeid(other) != typeid(allocator_buddies_system)) {
        return false;
    }
    auto other_allocator = static_cast<const allocator_buddies_system*>(&other);
    return _trusted_memory == other_allocator->_trusted_memory;
}

inline void allocator_buddies_system::set_fit_mode(
        allocator_with_fit_mode::fit_mode mode)
{
    std::lock_guard lock(get_mutex());
    debug_with_guard(std::string("Setting fit mode to ") +
                     (mode == allocator_with_fit_mode::fit_mode::first_fit ? "FIRST_FIT" :
                      mode == allocator_with_fit_mode::fit_mode::the_best_fit ? "BEST_FIT" : "WORST_FIT"));

    auto byte_ptr = reinterpret_cast<byte*>(_trusted_memory);
    auto fit_mode_ptr = reinterpret_cast<allocator_with_fit_mode::fit_mode*>(
            byte_ptr + sizeof(logger*) + sizeof(allocator_dbg_helper*));
    *fit_mode_ptr = mode;
}

std::vector<allocator_test_utils::block_info> allocator_buddies_system::get_blocks_info() const noexcept
{
    std::vector<allocator_test_utils::block_info> blocks_info;
    if (!_trusted_memory) {
        return blocks_info;
    }

    for (auto it = begin(); it != end(); ++it) {
        if (*it == nullptr) {
            break;
        }
        blocks_info.push_back({it.size(), it.occupied()});
    }

    return blocks_info;
}

inline logger *allocator_buddies_system::get_logger() const
{
    if (_trusted_memory == nullptr) {
        return nullptr;
    }
    return *reinterpret_cast<logger**>(_trusted_memory);
}

inline std::string allocator_buddies_system::get_typename() const
{
    return "allocator_buddies_system";
}

allocator_with_fit_mode::fit_mode &allocator_buddies_system::get_fit_mod() const noexcept
{
    return *reinterpret_cast<fit_mode*>(reinterpret_cast<byte*>(_trusted_memory) +
                                        sizeof(logger*) + sizeof(allocator_dbg_helper*));
}

void *allocator_buddies_system::get_first(size_t size) const noexcept
{
    for(auto it = begin(), sent = end(); it != sent; ++it) {
        if (!it.occupied() && it.size() >= size) {
            return *it;
        }
    }
    return nullptr;
}

void *allocator_buddies_system::get_best(size_t size) const noexcept
{
    buddy_iterator res;
    for (auto it = begin(), sent = end(); it != sent; ++it) {
        if (!it.occupied() && it.size() >= size) {
            if (*res == nullptr || it.size() < res.size()) {
                res = it;
            }
        }
    }

    return *res;
}

void *allocator_buddies_system::get_worst(size_t size) const noexcept
{
    buddy_iterator res;
    for (auto it = begin(), sent = end(); it != sent; ++it) {
        if (!it.occupied() && it.size() >= size) {
            if (*res == nullptr || it.size() > res.size()) {
                res = it;
            }
        }
    }

    return *res;
}

std::vector<allocator_test_utils::block_info> allocator_buddies_system::get_blocks_info_inner() const
{
    return get_blocks_info();
}



inline size_t allocator_buddies_system::get_size_block(void* current_block) const noexcept
{
    auto metadata = reinterpret_cast<block_metadata*>(current_block);
    return static_cast<size_t>(1) << metadata->size;
}

allocator_buddies_system::buddy_iterator allocator_buddies_system::begin() const noexcept
{
    return buddy_iterator(reinterpret_cast<void *>(reinterpret_cast<byte*>(_trusted_memory) + allocator_metadata_size));
}

void* allocator_buddies_system::get_twin(void* current_block) noexcept
{
    size_t block_size = get_size_block(current_block);
    size_t offset_first_twin = reinterpret_cast<byte*>(current_block) -
                               (reinterpret_cast<byte*>(_trusted_memory) + allocator_metadata_size);
    return reinterpret_cast<byte*>(_trusted_memory) + allocator_metadata_size + (offset_first_twin ^ block_size);
}

allocator_buddies_system::buddy_iterator allocator_buddies_system::end() const noexcept
{
    byte* end_ptr = reinterpret_cast<byte*>(_trusted_memory) +
                    allocator_metadata_size +
                    (static_cast<size_t>(1) << *reinterpret_cast<byte*>(
                            reinterpret_cast<byte*>(_trusted_memory) +
                            sizeof(logger*) +
                            sizeof(std::pmr::memory_resource*) +
                            sizeof(fit_mode)));
    return buddy_iterator(end_ptr);
}

bool allocator_buddies_system::buddy_iterator::operator==(const buddy_iterator &other) const noexcept
{
    return _block == other._block;
}

bool allocator_buddies_system::buddy_iterator::operator!=(const buddy_iterator &other) const noexcept
{
    return !(*this == other);
}

allocator_buddies_system::buddy_iterator &allocator_buddies_system::buddy_iterator::operator++() & noexcept {
    if (!_block) return *this;

    auto meta = reinterpret_cast<block_metadata *>(_block);
    size_t block_size = 1 << meta->size;
    _block = reinterpret_cast<byte *>(_block) + block_size;

    return *this;
}

allocator_buddies_system::buddy_iterator allocator_buddies_system::buddy_iterator::operator++(int)
{
    auto temp = *this;
    ++(*this);
    return temp;
}

inline size_t allocator_buddies_system::get_size_full() const noexcept
{
    byte* ptr = reinterpret_cast<byte*>(_trusted_memory) +
                sizeof(logger*) +
                sizeof(std::pmr::memory_resource*) +
                sizeof(fit_mode);
    return 1 << (*reinterpret_cast<unsigned char*>(ptr));
}

size_t allocator_buddies_system::buddy_iterator::size() const noexcept
{
    auto metadata = reinterpret_cast<block_metadata*>(_block);
    return static_cast<size_t>(1) << metadata->size;
}

bool allocator_buddies_system::buddy_iterator::occupied() const noexcept
{
    auto metadata = reinterpret_cast<block_metadata*>(_block);
    return metadata->occupied;
}

void *allocator_buddies_system::buddy_iterator::operator*() const noexcept
{
    return _block;
}

allocator_buddies_system::buddy_iterator::buddy_iterator(void *start) : _block(start)
{
}

allocator_buddies_system::buddy_iterator::buddy_iterator() : _block(nullptr)
{
}

