#ifndef MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_ALLOCATOR_GLOBAL_HEAP_H
#define MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_ALLOCATOR_GLOBAL_HEAP_H

#include <allocator_dbg_helper.h>
#include <logger.h>
#include <logger_guardant.h>
#include <pp_allocator.h>
#include <typename_holder.h>

class allocator_global_heap final:
        private allocator_dbg_helper,
        public smart_mem_resource,
        private logger_guardant,
        private typename_holder
{

private:

    logger *_logger;

    static constexpr const size_t size_t_size = sizeof(size_t);

public:

    explicit allocator_global_heap(
            logger *logger = nullptr);

    ~allocator_global_heap() override;

    allocator_global_heap(
            allocator_global_heap const &other);

    allocator_global_heap &operator=(
            allocator_global_heap const &other);

    allocator_global_heap(
            allocator_global_heap &&other) noexcept;

    allocator_global_heap &operator=(
            allocator_global_heap &&other) noexcept;

public:

    [[nodiscard]] void *do_allocate_sm(
            size_t size) override;

    void do_deallocate_sm(
            void *at) override;

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override;

private:

    inline logger *get_logger() const override;

private:

    inline std::string get_typename() const override;

public:

};

#endif //MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_ALLOCATOR_GLOBAL_HEAP_H